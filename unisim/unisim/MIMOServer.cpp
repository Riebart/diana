#include "MIMOServer.hpp"

#include <stdio.h>

// We can actually use Berkeley style sockets everywhere, just need to include the right stuff
// http://en.wikipedia.org/wiki/Berkeley_sockets

#ifdef WIN32
    #include <winsock2.h>
    // http://stackoverflow.com/questions/15660203/inet-pton-identifier-not-found
    // Make sure to link with ws2_32.lib
    #include <ws2tcpip.h>
    #define SHUT_RDWR SD_BOTH
    #define GET_ERROR WSAGetLastError()
    #define CLOSESOCKET closesocket

    typedef struct
    {
        u_int fd_count;
        SOCKET fd_array[FD_SETSIZE];
    } fd_setc;
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <string.h>
    #include <netinet/in.h>
    
    #define GET_ERROR errno
    #define SOCKET_ERROR -1
    #define INVALID_SOCKET -1
    #define CLOSESOCKET close

    typedef struct
    {
        socklen_t fd_count;
        int fd_array[FD_SETSIZE];
    } fd_setc;
#endif

/// @todo Don't need to pass pointers, just the server which contains the other information.
/// @todo Potentially move message parsing into here, because message handling will cause the socket to block.
class SocketThread
{
    friend class MIMOServer;
    friend void serve_SocketThread(SocketThread* sock);
    friend void serve_MIMOServer(MIMOServer* server);

public:
    SocketThread(int32_t c, void (*handler)(int32_t), MIMOServer* server);
    SocketThread(int32_t c, void (*handler)(int32_t, void*), void* handler_context, MIMOServer* server);
    ~SocketThread();

private:
    int32_t c;
    void (*handler)(int32_t);
    void (*handler2)(int32_t, void*);
    void* handler_context;
    bool running;
    std::thread t;
    MIMOServer* server;

    void start();
    void stop();
};

void serve_SocketThread(SocketThread* sock)
{
    while (sock->running)
    {
        if (!sock->running)
        {
            break;
        }

        char b;
        int32_t ret = recv(sock->c, &b, 1, MSG_PEEK);
        // Ignore errors if we're no longer running.
        if (!sock->running)
        {
            break;
        }

        if (ret == SOCKET_ERROR)
        {
            ret = GET_ERROR;
            fprintf(stderr, "Unable to peek at client %d (%d)\n", sock->c, ret);
            break;
        }

        if (ret == 0)
        {
            break;
        }

        // Handle any data we got.
        if (sock->handler != NULL)
        {
            sock->handler(sock->c);
        }
        else
        {
            sock->handler2(sock->c, sock->handler_context);
        }
    }

    if (sock->running)
    {
        sock->server->on_hangup(sock->c);
        sock->running = false;
    }
}

SocketThread::SocketThread(int32_t c, void (*handler)(int32_t), MIMOServer* server)
{
    this->c = c;
    this->handler = handler;
    this->handler2 = NULL;
    this->handler_context = NULL;
    this->server = server;
    running = false;
}

SocketThread::SocketThread(int32_t c, void (*handler)(int32_t, void*), void* handler_context, MIMOServer* server)
{
    this->c = c;
    this->handler = NULL;
    this->handler2 = handler;
    this->handler_context = handler_context;
    this->server = server;
    running = false;
}

SocketThread::~SocketThread()
{
    stop();
}

void SocketThread::start()
{
    if (!running)
    {
        running = true;
        t = std::thread(serve_SocketThread, this);
    }
}

void SocketThread::stop()
{
    if (running)
    {
        running = false;
        int32_t ret = shutdown(c, SHUT_RDWR);
        if (ret == SOCKET_ERROR)
        {
            ret = GET_ERROR;
            fprintf(stderr, "Error shutting down socket %d (%d)\n", c, ret);
        }

        ret = CLOSESOCKET(c);
        if (ret == SOCKET_ERROR)
        {
            ret = GET_ERROR;
            fprintf(stderr, "Error closing socket %d (%d)\n", c, ret);
        }

        if (t.joinable())
        {
            t.join();
        }
    }
}

// More on non-blocking IO: https://publib.boulder.ibm.com/infocenter/iseries/v5r3/index.jsp?topic=%2Frzab6%2Frzab6xnonblock.htm
void serve_MIMOServer(MIMOServer* server)
{
    fd_setc fds;
#ifdef WIN32
    const timeval timeout = { 0, 10000 };
#else
    timeval timeout = { 0, 10000 };
#endif

    int32_t nready;

    while (server->running)
    {
        fds.fd_count = 2;
        fds.fd_array[0] = server->server4;
        fds.fd_array[1] = server->server6;

        nready = select(2, (fd_set*)&fds, NULL, NULL, &timeout);
        if (nready == SOCKET_ERROR)
        {
            nready = GET_ERROR;
            fprintf(stderr, "Error when selecting sockets (%d)\n", nready);
            server->stop();
            break;
        }

        // When we stop the server, we hang up the sockets, and so select returns.
        // Break if we're stopping
        if (!server->running)
        {
            break;
        }

        // Service hangups
        if (server->hangups.size() > 0)
        {
            server->hangup_lock.lock();
            int32_t pre_hangup = server->inputs.size();

            for (int32_t i = 0 ; i < server->hangups.size() ; i++)
            {
                server->hangup(server->hangups[i]);
            }

            server->hangups.clear();
            server->hangup_lock.unlock();
            continue;
        }

        // The only ones we're looking for are 
        if (nready > 0)
        {
            struct sockaddr_storage addr = { 0 };
#ifdef WIN32
            int32_t addrlen = sizeof(struct sockaddr_storage);
#else
            socklen_t addrlen = sizeof(struct sockaddr_storage);
#endif

            for (int32_t i = 0 ; i < fds.fd_count ; i++)
            {
                // First check to see if the socket is ready for a connection, or if we're shutting them down
                int32_t err = -12345;
#ifdef WIN32
                int32_t sockoptslen = 4;
#else
                socklen_t sockoptslen = 4;
#endif
                nready = getsockopt(fds.fd_array[i], SOL_SOCKET, SO_ERROR, (char*)&err, &sockoptslen);
                //fprintf(stderr, "%d\n", err);
                // I don't think we are ready to accept this connection maybe?
                if (err != 0)
                {
                    continue;
                }

                int32_t c = accept(fds.fd_array[i], (struct sockaddr*)&addr, &addrlen);
                if (c == INVALID_SOCKET)
                {
                    nready = GET_ERROR;
                    fprintf(stderr, "Unable to accept connection on socket %d (%d)\n", fds.fd_array[i], nready);
                    continue;
                }

                if (fds.fd_array[i] == server->server4)
                {
                    char buf[16];
                    struct sockaddr_in* addr4 = (struct sockaddr_in*)&addr;
                    inet_ntop(AF_INET, &addr4->sin_addr, buf, 16);
                    fprintf(stderr, "Received connection from %s:%d\n", buf, ntohs(addr4->sin_port));
                }
                else // if (fds.fd_array[i] == server->server6)
                {
                    char buf[40];
                    struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&addr;
                    inet_ntop(AF_INET6, &addr6->sin6_addr, buf, 40);
                    fprintf(stderr, "Received connection from %s:%d\n", buf, ntohs(addr6->sin6_port));
                }

                SocketThread* st;
                if (server->data_callback2 == NULL)
                {
                    st = new SocketThread(c, server->data_callback, server);
                }
                else
                {
                    st = new SocketThread(c, server->data_callback2, server->data_context, server);
                }

                server->threadmap[c] = st;
                server->inputs.push_back(c);
                st->start();
            }
        }
    }
}

MIMOServer::MIMOServer(void (*data_callback)(int32_t), void (*hangup_callback)(int32_t), int32_t port, int32_t backlog)
{
    this->data_callback = data_callback;
    this->data_callback2 = NULL;
    this->data_context = NULL;
    this->hangup_callback = hangup_callback;
    this->hangup_callback2 = NULL;
    this->hangup_context = NULL;

    if ((port < 0) || (port > 65535))
    {
        fprintf(stderr, "Port %d is out of acceptable range.\n", port);
        exit(EXIT_FAILURE);
    }
    this->port = port;
    this->backlog = backlog;

    running = false;
}

MIMOServer::MIMOServer(void (*data_callback)(int32_t, void*), void* data_context, void (*hangup_callback)(int32_t, void*), void* hangup_context, int32_t port, int32_t backlog)
{
    this->data_callback = NULL;
    this->data_callback2 = data_callback;
    this->data_context = data_context;
    this->hangup_callback2 = NULL;
    this->hangup_callback2 = hangup_callback;
    this->hangup_context = hangup_context;

    if ((port < 0) || (port > 65535))
    {
        fprintf(stderr, "Port %d is out of acceptable range.\n", port);
        exit(EXIT_FAILURE);
    }
    this->port = port;
    this->backlog = backlog;

    running = false;
}

MIMOServer::~MIMOServer()
{
    stop();

    // Double-check that the threadmap is empty
    if (threadmap.size() > 0)
    {
        fprintf(stderr, "The threadmap still has %lu things at the end of the destructor!\n", threadmap.size());
        std::map<int32_t, struct SocketThread*>::iterator it;
        for (it = threadmap.begin() ; it != threadmap.end() ; ++it)
        {
            it->second->stop();
            delete it->second;
        }
    }
}

int32_t listen(int32_t port, int32_t backlog, int32_t family, uint32_t addr4, in6_addr addr6)
{
    int32_t server = socket(family, SOCK_STREAM, IPPROTO_TCP);
    if(server == INVALID_SOCKET)
    {
        perror("Can not create MIMOServer socket");
        exit(EXIT_FAILURE);
    }

    int32_t ret;
    uint32_t sockopts = 1;
    ret = setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&sockopts, 1);
    if (ret == SOCKET_ERROR)
    {
        ret = GET_ERROR;
        fprintf(stderr, "Can not set MIMOServer socket options (%d)\n", ret);
        exit(EXIT_FAILURE);
    }

    switch(family)
    {
    case AF_INET:
        {
            struct sockaddr_in stSockAddr;
            memset(&stSockAddr, 0, sizeof(stSockAddr));
            stSockAddr.sin_family = AF_INET;
            stSockAddr.sin_port = htons(port);
            stSockAddr.sin_addr.s_addr = htonl(addr4);
            ret = bind(server, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr));
            break;
        }
    case AF_INET6:
        {
            struct sockaddr_in6 stSockAddr;
            memset(&stSockAddr, 0, sizeof(stSockAddr));
            stSockAddr.sin6_family = AF_INET6;
            stSockAddr.sin6_port = htons(port);
            stSockAddr.sin6_addr = addr6;
            ret = bind(server, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr));
            break;
        }
    default:
        {
            ret = GET_ERROR;
            fprintf(stderr, "Invalid address family for socket (%d)\n", ret);
            ret = CLOSESOCKET(server);
            if (ret == SOCKET_ERROR)
            {
                ret = GET_ERROR;
                fprintf(stderr, "Error closing socket %d (%d)\n", server, ret);
            }
            exit(EXIT_FAILURE);
        }
    }

    if (ret == SOCKET_ERROR)
    {
        ret = GET_ERROR;
        fprintf(stderr, "Can not bind MIMOServer socket (%d)\n", ret);
        ret = CLOSESOCKET(server);
        if (ret == SOCKET_ERROR)
        {
            ret = GET_ERROR;
            fprintf(stderr, "Error closing socket %d (%d)\n", server, ret);
        }
        exit(EXIT_FAILURE);
    }

    ret = listen(server, backlog);
    if (ret == SOCKET_ERROR)
    {
        ret = GET_ERROR;
        fprintf(stderr, "Can not put MIMOServer socket into listen state (%d)\n", ret);
        ret = CLOSESOCKET(server);
        if (ret == SOCKET_ERROR)
        {
            ret = GET_ERROR;
            fprintf(stderr, "Error closing socket %d (%d)\n", server, ret);
        }
        exit(EXIT_FAILURE);
    }

    return server;
}

int32_t listen4(int32_t port, int32_t backlog, uint32_t addr)
{
    return listen(port, backlog, AF_INET, addr, in6addr_any);
}

int32_t listen6(int32_t port, int32_t backlog, in6_addr addr)
{
    return listen(port, backlog, AF_INET6, INADDR_ANY, addr);
}

void MIMOServer::start()
{
    if (!running)
    {
#ifdef WIN32
        WSADATA wsad;
        WSAStartup(22, &wsad);
#endif

        server4 = listen4(port, backlog, INADDR_ANY);
        server6 = listen6(port, backlog, in6addr_any);

        running = true;
        server_thread = std::thread(serve_MIMOServer, this);
        fprintf(stderr, "Listening on port %d\n", port);
    }
}

void MIMOServer::stop()
{
    if (running)
    {
        running = false;
        server_thread.join();

        fprintf(stderr, "Shutting down server...\n");
        int32_t ret = shutdown(server4, SHUT_RDWR);
        if (ret == SOCKET_ERROR)
        {
            ret = GET_ERROR;
            // Ignore socket not connected errors
            if (ret != 10057)
            {
                fprintf(stderr, "Error shutting down server4 socket %d (%d)\n", server4, ret);
            }
        }
        ret = CLOSESOCKET(server4);
        if (ret == SOCKET_ERROR)
        {
            ret = GET_ERROR;
            fprintf(stderr, "Error closing server4 socket %d (%d)\n", server4, ret);
        }

        ret = shutdown(server6, SHUT_RDWR);
        if (ret == SOCKET_ERROR)
        {
            ret = GET_ERROR;
            // Ignore socket not connected errors
            if (ret != 10057)
            {
                fprintf(stderr, "Error shutting down server6 socket %d (%d)\n", server6, ret);
            }
        }
        ret = CLOSESOCKET(server6);
        if (ret == SOCKET_ERROR)
        {
            ret = GET_ERROR;
            fprintf(stderr, "Error closing server6 socket %d (%d)\n", server6, ret);
        }

        bool stubborn = false;
        while (inputs.size() > 0)
        {
            fprintf(stderr, "Hanging up %lu %sclients%s\n", inputs.size(), (stubborn ? "stubborn " : ""), (inputs.size() > 1 ? "s" : ""));

            for (int32_t i = 0 ; i < inputs.size() ; i++)
            {
                hangup(inputs[i]);
                i--;
            }
            stubborn = true;
        }

        hangups.clear();
        inputs.clear();
    }
}

void MIMOServer::on_hangup(int32_t c)
{
    hangup_lock.lock();
    hangups.push_back(c);
    hangup_lock.unlock();
}

// Called from serve_MIMOServer() when a client hangs up.
void MIMOServer::hangup(int32_t c)
{
    fprintf(stderr, "Hanging up %d\n", c);

    // @todo The python checks for already hung up clients,
    // but we'll just arrange for that not to happen.
    for (int32_t i = 0 ; i < inputs.size() ; i++)
    {
        if (inputs[i] == c)
        {
            inputs.erase(inputs.begin()+i);
            break;
        }
    }

    SocketThread* sock = threadmap[c];
    sock->stop();
    delete sock;
    threadmap.erase(c);

    if (data_callback2 == NULL)
    {
        hangup_callback(c);
    }
    else
    {
        hangup_callback2(c, hangup_context);
    }
}
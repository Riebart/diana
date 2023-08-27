#include "MIMOServer.hpp"

#include "utility.hpp"

#include <stdio.h>

#define LOCK(l) l.lock()
#define UNLOCK(l) l.unlock()
#define THREAD_CREATE(t, f, a) t = std::thread(f, a)
#define THREAD_JOIN(t) if (t.joinable()) t.join()

//! @todo #ifdef between perror() and WSAGetLastErrorMessage()

// We can actually use Berkeley style sockets everywhere, just need to include the right stuff
// http://en.wikipedia.org/wiki/Berkeley_sockets

#ifdef _WIN32
#include <winsock2.h>
// http://stackoverflow.com/questions/15660203/inet-pton-identifier-not-found
// Make sure to link with ws2_32.lib
#include <ws2tcpip.h>
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "ws2_32.lib")
#define SHUT_RDWR SD_BOTH
#define GET_ERROR WSAGetLastError()
#define CLOSESOCKET closesocket
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>

#define GET_ERROR errno
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define CLOSESOCKET close
#define SOCKET int32_t
#endif

#define MAX_SOCKET_RETRIES 256
#define SOCKET_RETRY_DELAY 5 // in milliseconds

namespace Diana
{
    int64_t MIMOServer::socket_read(int fd, char* buf, int64_t count)
    {
        std::chrono::milliseconds dura(SOCKET_RETRY_DELAY);
        int64_t nbytes = 0;
        int64_t curbytes;
        int32_t nretries = MAX_SOCKET_RETRIES;
        
        while (nbytes < count)
        {
            //! @todo Maybe use recvmsg()?
            // Try to wait for all of the data, if possible.
            curbytes = recv(fd, buf + (size_t)nbytes, (int)(count - nbytes), MSG_WAITALL);
            if (curbytes == -1)
            {
                nretries--;
                std::this_thread::sleep_for(dura);
            }
            else
            {
                nretries = MAX_SOCKET_RETRIES;
            }
            
            if (nretries == 0)
            {
                // Flip the sign of the bytes returnd, as a reminder to check up on errno and errmsg
                nbytes *= -1;
                break;
            }
            nbytes += curbytes;
        }

        return nbytes;
    }

    int64_t MIMOServer::socket_write(int fd, char* buf, int64_t count)
    {
        std::chrono::milliseconds dura(SOCKET_RETRY_DELAY);
        int64_t nbytes = 0;
        int64_t curbytes;
        int32_t nretries = MAX_SOCKET_RETRIES;

        while (nbytes < count)
        {
            curbytes = send(fd, buf + (size_t)nbytes, (int)(count - nbytes), 0);
            if (curbytes == -1)
            {
                nretries--;
                std::this_thread::sleep_for(dura);
            }
            else
            {
                nretries = MAX_SOCKET_RETRIES;
            }

            if (nretries == 0)
            {
                // Flip the sign of the bytes returnd, as a reminder to check up on errno and errmsg
                nbytes *= -1;
                break;
            }
            nbytes += curbytes;
        }

        return nbytes;
    }

    //! @todo Don't need to pass pointers, just the server which contains the other information.
    //! @todo Potentially move message parsing into here, because message handling will cause the socket to block.
    class SocketThread
    {
        friend class MIMOServer;

        friend void* serve_SocketThread(void* sock);
        friend void* serve_MIMOServer(void* server);

    public:
        SocketThread(int32_t c, void(*handler)(int32_t), MIMOServer* server);
        SocketThread(int32_t c, void(*handler)(int32_t, void*), void* handler_context, MIMOServer* server);
        ~SocketThread();

    private:
        int32_t c;
        void(*handler)(int32_t);
        void(*handler2)(int32_t, void*);
        void* handler_context;
        bool running;

        THREAD_T t;

        MIMOServer* server;

        void start();
        void stop();
    };

    void* serve_SocketThread(void* sockV)
    {
        fprintf(stderr, "Socket serving thread PID: %u\n", get_this_thread_pid());
        SocketThread* sock = (SocketThread*)sockV;

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
        }

        return NULL;
    }

    SocketThread::SocketThread(int32_t c, void(*handler)(int32_t), MIMOServer* server)
    {
        this->c = c;
        this->handler = handler;
        this->handler2 = NULL;
        this->handler_context = NULL;
        this->server = server;
        running = false;
    }

    SocketThread::SocketThread(int32_t c, void(*handler)(int32_t, void*), void* handler_context, MIMOServer* server)
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

            THREAD_CREATE(t, serve_SocketThread, (void*)this);
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

            THREAD_JOIN(t);
        }
    }

    // More on non-blocking IO: https://publib.boulder.ibm.com/infocenter/iseries/v5r3/index.jsp?topic=%2Frzab6%2Frzab6xnonblock.htm
    void* serve_MIMOServer(void* serverV)
    {
        fprintf(stderr, "MIMOServer main thread PID: %u\n", get_this_thread_pid());

        MIMOServer* server = (MIMOServer*)serverV;

        fd_set fds;

#ifdef _WIN32
        const timeval timeout = { 0, 10000 };
#else
        // The select() syscall uses { seconds, microseconds } as the timeout
        // The pselect() syscall, which modern gcc and kernels map select() to,
        //    uses { seconds, nanoseconds }
        // timeval timeout = { 0, 10000 }; // Use this for select()
        timespec timeout = { 0, 10000000 }; // Use this for pselect()
        int32_t maxfd = (server->server6 > server->server4 ? server->server6 : server->server4) + 1;
        int32_t fd_array[] = { server->server4, server->server6 };
#endif

        int32_t nready;

        while (server->running)
        {
#ifdef _WIN32
            fds.fd_count = 2;
            fds.fd_array[0] = server->server4;
            fds.fd_array[1] = server->server6;
            nready = select(2, &fds, NULL, NULL, &timeout);
#else
            FD_ZERO(&fds);
            FD_SET(server->server4, &fds);
            FD_SET(server->server6, &fds);
            nready = pselect(maxfd, (fd_set*)&fds, NULL, NULL, &timeout, NULL);
#endif

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
                LOCK(server->hangup_lock);
                size_t hungup = server->inputs.size();

                for (size_t i = 0; i < server->hangups.size(); i++)
                {
                    server->hangup(server->hangups[i]);
                }

                hungup -= server->inputs.size();
                fprintf(stderr, "Successfully hung up %lu client%s\n", hungup, ((hungup > 1) ? "s" : ""));

                server->hangups.clear();
                UNLOCK(server->hangup_lock);
                continue;
            }

            // The only ones we're looking for are 
            if (nready > 0)
            {
                struct sockaddr_storage addr = { 0 };

#ifdef _WIN32
                int32_t addrlen = sizeof(struct sockaddr_storage);

                for (uint32_t i = 0; i < fds.fd_count; i++)
                {
                    int32_t curfd = fds.fd_array[i];
                    int32_t sockoptslen = 4;
#else
                socklen_t addrlen = sizeof(struct sockaddr_storage);

                for (uint32_t i = 0 ; i < 2 ; i++)
                {
                    int32_t curfd;
                    if (FD_ISSET(fd_array[i], &fds))
                    {
                        curfd = fd_array[i];
                    }
                    else
                    {
                        continue;
                    }

                    socklen_t sockoptslen = 4;
#endif

                    // First check to see if the socket is ready for a connection, or if we're shutting them down
                    int32_t err = -12345;

                    nready = getsockopt(curfd, SOL_SOCKET, SO_ERROR, (char*)&err, &sockoptslen);
                    //fprintf(stderr, "%d\n", err);
                    // I don't think we are ready to accept this connection maybe?
                    if (err != 0)
                    {
                        continue;
                    }

                    int32_t c = accept(curfd, (struct sockaddr*)&addr, &addrlen);
                    if (c == INVALID_SOCKET)
                    {
                        nready = GET_ERROR;
                        fprintf(stderr, "Unable to accept connection on socket %d (%d)\n", curfd, nready);
                        continue;
                    }

                    if (curfd == server->server4)
                    {
                        char buf[16];
                        struct sockaddr_in* addr4 = (struct sockaddr_in*)&addr;
                        inet_ntop(AF_INET, &addr4->sin_addr, buf, 16);
                        fprintf(stderr, "Received connection from %s:%d\n", buf, ntohs(addr4->sin_port));
                    }
                    else // if (curfd == server->server6)
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

        return NULL;
    }

    MIMOServer::MIMOServer(void(*data_callback)(int32_t), void(*hangup_callback)(int32_t), int32_t port, int32_t backlog)
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

    MIMOServer::MIMOServer(void(*data_callback)(int32_t, void*), void* data_context, void(*hangup_callback)(int32_t, void*), void* hangup_context, int32_t port, int32_t backlog)
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
#if __x86_64__
            fprintf(stderr, "The threadmap still has %lu things at the end of the destructor!\n", (uint64_t)threadmap.size());
#else
            fprintf(stderr, "The threadmap still has %llu things at the end of the destructor!\n", (uint64_t)threadmap.size());
#endif
            std::map<int32_t, SocketThread*>::iterator it;
            for (it = threadmap.begin(); it != threadmap.end(); ++it)
            {
                it->second->stop();
                delete it->second;
            }
        }
    }

    int32_t mimo_listen(int32_t port, int32_t backlog, int32_t family, uint32_t addr4, in6_addr addr6)
    {
        int32_t ret;

        int32_t server = socket(family, SOCK_STREAM, IPPROTO_TCP);
        if (server == INVALID_SOCKET)
        {
            ret = GET_ERROR;
            fprintf(stderr, "Can not create MIMOServer socket (%d)\n", ret);
            exit(EXIT_FAILURE);
        }

        uint32_t sockopts = 1;
        ret = setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&sockopts, 4);

        if (ret == SOCKET_ERROR)
        {
            ret = GET_ERROR;
            fprintf(stderr, "Can not set MIMOServer socket options (%d)\n", ret);
            exit(EXIT_FAILURE);
        }

        switch (family)
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
#ifndef _WIN32
            sockopts = 1;
            ret = setsockopt(server, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&sockopts, 4);

            if (ret == SOCKET_ERROR)
            {
                perror(NULL);
                ret = GET_ERROR;
                fprintf(stderr, "Can not set MIMOServer v6 socket to v6 only (%d)\n", ret);
                exit(EXIT_FAILURE);
            }
#endif

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
            perror(NULL);
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

    int32_t mimo_listen4(int32_t port, int32_t backlog, uint32_t addr)
    {
        return mimo_listen(port, backlog, AF_INET, addr, in6addr_any);
    }

    int32_t mimo_listen6(int32_t port, int32_t backlog, in6_addr addr)
    {
        return mimo_listen(port, backlog, AF_INET6, INADDR_ANY, addr);
    }

    void MIMOServer::start()
    {
        if (!running)
        {
#ifdef _WIN32
            WSADATA wsad;
            int ret = WSAStartup(22, &wsad);
            if (ret != 0)
            {
                fprintf(stderr, "Unable to initialize Windows Sockets runtime (%d)\n", ret);
                exit(EXIT_FAILURE);
            }
#endif

            server4 = mimo_listen4(port, backlog, INADDR_ANY);
            server6 = mimo_listen6(port, backlog, in6addr_any);

            running = true;

            THREAD_CREATE(server_thread, serve_MIMOServer, (void*)this);

            fprintf(stderr, "Listening on port %d\n", port);
        }
    }

    void MIMOServer::stop()
    {
        if (running)
        {
            running = false;

            THREAD_JOIN(server_thread);

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
#if __x86_64__
                fprintf(stderr, "Hanging up %lu %sclients%s\n", (uint64_t)inputs.size(), (stubborn ? "stubborn " : ""), (inputs.size() > 1 ? "s" : ""));
#else
                fprintf(stderr, "Hanging up %llu %sclients%s\n", (uint64_t)inputs.size(), (stubborn ? "stubborn " : ""), (inputs.size() > 1 ? "s" : ""));
#endif

                for (size_t i = 0; i < inputs.size(); i++)
                {
                    hangup(inputs[i]);
                }

                stubborn = true;
            }

            hangups.clear();
            inputs.clear();
        }
    }

    void MIMOServer::on_hangup(int32_t c)
    {
        LOCK(hangup_lock);
        hangups.push_back(c);
        UNLOCK(hangup_lock);
    }

    // Called from serve_MIMOServer() when a client hangs up.
    void MIMOServer::hangup(int32_t c)
    {
        fprintf(stderr, "Hanging up %d\n", c);

        // @todo The python checks for already hung up clients,
        // but we'll just arrange for that not to happen.
        for (size_t i = 0; i < inputs.size(); i++)
        {
            if (inputs[i] == c)
            {
                inputs.erase(inputs.begin() + i);
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
}

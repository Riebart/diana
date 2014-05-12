#include <iostream>
#include <stdlib.h>
#include <assert.h>

#include <signal.h>

#ifdef WIN32
#include <WinSock2.h>
#else
#include <sys/socket.h>
#endif

#include "universe.hpp"
#include "MIMOServer.hpp"

bool running = true;
Universe* u;

void sighandler(int32_t sig)
{
    running = false;
    u->stop_net();
    u->stop_sim();
}

int32_t compare(const void* aV, const void* bV)
{
    uint64_t a = *(uint64_t*)aV;
    uint64_t b = *(uint64_t*)bV;
    return ((a < b) ? -1 : ((a == b) ? 0 : 1));
}

void dc(int32_t c)
{
    fprintf(stderr, "DC %d\n", c);
    char buf[1024];
    int32_t ngot = recv(c, buf, 1023, 0);
    if (ngot == SOCKET_ERROR)
    {
        ngot = WSAGetLastError();
        fprintf(stderr, "Couldn't read from socket %d (%d)\n", c, ngot);
        return;
    }

    buf[ngot] = 0;
    fprintf(stderr, "%s", buf);
}

void hc(int32_t c)
{
    fprintf(stderr, "HC %d\n", c);
}

void main(int32_t argc, char** argv)
{
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT,  &sighandler);

    //u = new Universe(0.001, 0.05, 0.5, 5505, 1);
    u = new Universe(1e-9, 1e-9, 0.5, 5505, 1, 1.0, false);

    try
    {
        u->start_net();
        u->start_sim();

        double frametimes[4];
        std::chrono::seconds dura(1);
        uint64_t last_ticks = u->get_ticks();
        uint64_t cur_ticks;

        while (running)
        {
            u->get_frametime(frametimes);
            cur_ticks = u->get_ticks();
            fprintf(stderr, "%g, %g, %g, %g, %llu\n", frametimes[0], frametimes[1], frametimes[2], frametimes[3], cur_ticks - last_ticks);
            last_ticks = cur_ticks;
            std::this_thread::sleep_for(dura);
        }

        u->stop_net();
        u->stop_sim();
        delete u;
    }
    catch(char* e)
    {
        fprintf(stderr, "Caught error: %s\n", e);
    }
}

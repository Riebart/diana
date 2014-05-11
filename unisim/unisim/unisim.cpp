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

	u = new Universe(0.0, 0.001, 0.01, 5505, 1);

	try
	{
		u->start_net();
		u->start_sim();

		double frametimes[3];
		std::chrono::seconds dura(1);

		while (running)
		{
			u->get_frametime(frametimes);
			fprintf(stderr, "%g, %g %g\n", frametimes[0], frametimes[1], frametimes[2]);
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

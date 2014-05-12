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
std::vector<struct PhysicsObject*> objs;

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

void pool_rack()
{
    double ball_mass = 0.15;
    double cue_ball_mass = 0.26;
    double ball_radius = 0.056896;
    double cue_ball_radius = 1.1 * ball_radius;

    int num_rows = 5;

    // This loop produces a trangle of balls that points down the negative y axis.
    // That is, the 'head' ball is further negative than the 'back' of the rack.
    //
    // Mathematica code.
    // numRows = 5;
    // radius = 1;
    // balls = Reap[For[i = 0, i < numRows, i++,
    //    For[j = 0, j <= i, j++,
    //    x = (i - 2 j) radius;
    //    y = Sqrt[3]/2 (1 + 2 i) radius;
    //    Sow[Circle[{x, y}, radius]]
    //    ]
    //    ]][[2]][[1]];
    // Graphics[balls]

    struct PhysicsObject* obj;
    struct Vector3 vector3_zero = { 0.0, 0.0, 0.0 };
    struct Vector3 position = { 0.0, 0.0, 0.0 };
    struct Vector3 velocity = { 0.0, -25, 0.0 };

    double C = 1;
    double y_scale = sqrt(3) / 2;
    double y_offset = 100;

    for (int i = 0; i < num_rows ; i++)
    {
        for (int j = 0 ; j < i+1 ; j++)
        {
            obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
            objs.push_back(obj);

            position.x = C * (i - 2 * j) * ball_radius;
            position.y = y_offset - C * y_scale * (1 + 2 * i) * ball_radius;

            PhysicsObject_init(obj, u, &position, &vector3_zero, &vector3_zero, &vector3_zero, ball_mass, ball_radius, NULL);
            u->add_object(obj);
        }
    }

    obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
    position.x = 0.0;
    position.y = 10.0;
    position.z = 0.0;
    PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, ball_mass, ball_radius, NULL);
    u->add_object(obj);
    objs.push_back(obj);
}

void simple_collision()
{
    struct PhysicsObject* obj;
    struct Vector3 vector3_zero = { 0.0, 0.0, 0.0 };
    struct Vector3 position = { 0.0, 0.0, 0.0 };
    struct Vector3 velocity = { 0.0, 0.0, 0.0 };

    obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
    objs.push_back(obj);
    position.x = 0.0;
    velocity.x = -5.0;
    PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, 1, 1, NULL);
    u->add_object(obj);

    obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
    objs.push_back(obj);
    position.x = -20;
    velocity.x = 0.0;
    PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, 1, 1, NULL);
    u->add_object(obj);
}

void print_positions()
{
    for (int i = 0 ; i < objs.size() ; i++)
    {
        fprintf(stderr, "%g   %g   %g\n", objs[i]->position.x, objs[i]->position.y, objs[i]->position.z);
    }
}

void main(int32_t argc, char** argv)
{
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT,  &sighandler);

    u = new Universe(0.001, 0.05, 0.5, 5505, 1);
    //u = new Universe(1e-9, 1e-9, 0.5, 5505, 1, 1.0, false);

    try
    {
        u->start_net();
        u->start_sim();

        //pool_rack();
        simple_collision();

        double frametimes[4];
        std::chrono::seconds dura(1);
        uint64_t last_ticks = u->get_ticks();
        uint64_t cur_ticks;

        while (running)
        {
            u->get_frametime(frametimes);
            cur_ticks = u->get_ticks();
            fprintf(stderr, "%g, %g, %g, %g, %llu\n", frametimes[0], frametimes[1], frametimes[2], frametimes[3], cur_ticks - last_ticks);
            print_positions();
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

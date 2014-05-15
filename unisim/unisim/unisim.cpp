#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <signal.h>

#ifdef CPP11THREADS
#include <chrono>
#else
#include <unistd.h>
#endif

#include "universe.hpp"
#include "MIMOServer.hpp"

/// @todo Let's start talking about 128-bit fixed-point arithmetic for some stuff
/// http://msdn.microsoft.com/en-us/library/windows/desktop/aa383711(v=vs.85).aspx
/// GCC: __uint128_t and __int128_t
/// Look back at the UniverseGenerator code, and see if we can resurrect some of it.

bool running = true;
Universe* u;
std::vector<struct PhysicsObject*> objs;
std::vector<struct Beam*> beams;

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
    PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, cue_ball_mass, cue_ball_radius, NULL);
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
    position.y = 0.0;
    velocity.x = -5.0;
    PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, 1, 1, NULL);
    u->add_object(obj);

    obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
    objs.push_back(obj);
    position.x = -20;
    position.y = 0.0;
    velocity.x = 0.0;
    PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, 1, 1, NULL);
    u->add_object(obj);
}

void fast_collision()
{
    struct PhysicsObject* obj;
    struct Vector3 vector3_zero = { 0.0, 0.0, 0.0 };
    struct Vector3 position = { 0.0, 0.0, 0.0 };
    struct Vector3 velocity = { 0.0, 0.0, 0.0 };
    double mass = 1.0;

    obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
    objs.push_back(obj);
    position.x = 0.0;
    position.y = 0.0;
    velocity.x = -5000.0;
    PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, mass, 1, NULL);
    obj->health = 1e10;
    u->add_object(obj);

    obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
    objs.push_back(obj);
    position.x = -20;
    position.y = 0.0;
    velocity.x = 0.0;
    PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, 1, 1, NULL);
    obj->health = 1e10;
    u->add_object(obj);
}

void beam_collision()
{
    struct Vector3 vector3_zero = { 0.0, 0.0, 0.0 };
    struct Vector3 position = { 10.0, 0.0, 0.0 };
    struct Vector3 velocity = { -4000.0, 0.0, 0.0 };
    struct Vector3 up = { 0.0, 0.0, 1.0 };
    struct Beam* beam;

    beam = (struct Beam*)malloc(sizeof(struct Beam));
    beams.push_back(beam);
    Beam_init(beam, u, &position, &velocity, &up, 1, 1, 1000, BEAM_WEAP);
    u->add_object((struct PhysicsObject*)beam);

    double mass = 1.0;
    struct PhysicsObject* obj;

    obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
    objs.push_back(obj);
    position.x = 0.0;
    position.y = 0.0;
    velocity.x = 0.0;
    PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, mass, 1, NULL);
    obj->health = 1e10;
    u->add_object(obj);

    obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
    objs.push_back(obj);
    position.x = -20;
    position.y = 0.0;
    velocity.x = 0.0;
    PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, 1, 1, NULL);
    obj->health = 1e10;
    u->add_object(obj);
}

void beam_multi_collision()
{
    struct Vector3 vector3_zero = { 0.0, 0.0, 0.0 };
    struct Vector3 position = { 10.0, 0.0, 0.0 };
    struct Vector3 velocity = { -4000.0, 0.0, 0.0 };
    struct Vector3 up = { 0.0, 0.0, 1.0 };
    struct Beam* beam;

    beam = (struct Beam*)malloc(sizeof(struct Beam));
    beams.push_back(beam);
    Beam_init(beam, u, &position, &velocity, &up, 1, 1, 1000, BEAM_WEAP);
    u->add_object((struct PhysicsObject*)beam);

    double mass = 1.0;
    double radius = 1.0;
    struct PhysicsObject* obj;

    obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
    objs.push_back(obj);
    position.x = 0.0;
    position.y = 0.0;
    velocity.x = 0.0;
    PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, mass, radius, NULL);
    obj->health = 1e10;
    u->add_object(obj);

    for (int i = 0 ; i < 10 ; i++)
    {
        obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
        objs.push_back(obj);
        position.x = 0.0;
        position.z = 1.0 + i * radius;
        velocity.x = 0.0;
        PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, mass, radius, NULL);
        obj->health = 1e10;
        u->add_object(obj);

        obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
        objs.push_back(obj);
        position.x = 0.0;
        position.z = -1.0 - i * radius;
        velocity.x = 0.0;
        PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, mass, radius, NULL);
        obj->health = 1e10;
        u->add_object(obj);
    }
}

void print_positions()
{
    for (uint32_t i = 0 ; i < objs.size() ; i++)
    {
        fprintf(stderr, "PO%u   %g   %g   %g\n", i, objs[i]->position.x, objs[i]->position.y, objs[i]->position.z);
    }

    for (uint32_t i = 0 ; i < beams.size() ; i++)
    {
        fprintf(stderr, "BM%u   %g   %g   %g\n", i, beams[i]->front_position.x, beams[i]->front_position.y, beams[i]->front_position.z);
    }
}

int main(int32_t argc, char** argv)
{
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT,  &sighandler);

    u = new Universe(0.001, 0.05, 0.5, 5505, 1);
    //u = new Universe(1e-6, 1e-6, 0.5, 5505, 1, 1.0, false);

    try
    {
        u->start_net();
        u->start_sim();

        //pool_rack();
        //simple_collision();
        //fast_collision();
        //beam_collision();
        beam_multi_collision();

        double frametimes[4];
        uint64_t last_ticks = u->get_ticks();
        uint64_t cur_ticks;

#ifdef CPP11THREADS
        std::chrono::seconds dura(1);
#endif

        while (running)
        {
            u->get_frametime(frametimes);
            cur_ticks = u->get_ticks();
#if _WIN64 || __x86_64__
            fprintf(stderr, "%g, %g, %g, %g, %g, %lu\n", frametimes[0], frametimes[1], frametimes[2], frametimes[3], u->total_sim_time(), cur_ticks - last_ticks);
#else
            fprintf(stderr, "%g, %g, %g, %g, %g, %llu\n", frametimes[0], frametimes[1], frametimes[2], frametimes[3], u->total_sim_time(), cur_ticks - last_ticks);
#endif
            print_positions();
            last_ticks = cur_ticks;

#ifdef CPP11THREADS
            std::this_thread::sleep_for(dura);
#else
            usleep(1000000);
#endif
        }

        u->stop_net();
        u->stop_sim();
        delete u;

        return 0;
    }
    catch(char* e)
    {
        fprintf(stderr, "Caught error: %s\n", e);
        return 1;
    }
}

#include <stdio.h>
#include <signal.h>
#include <chrono>

#include "universe.hpp"
#include "MIMOServer.hpp"

volatile bool running = true;

void sighandler(int32_t sig)
{
    fprintf(stderr, "Caught Ctrl+C, stopping.\n");
    running = false;
}

void check_packing()
{
    struct Diana::PhysicsObject p;
    // On g++ 64-bit, we need %lu, all other times we need %llu
#if __x86_64__
    printf("%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
#else
    printf("%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
#endif
        (uint64_t)&p.type - (uint64_t)&p,
        (uint64_t)&p.phys_id - (uint64_t)&p,
        (uint64_t)&p.universe - (uint64_t)&p,
        (uint64_t)&p.box - (uint64_t)&p,
        (uint64_t)&p.t - (uint64_t)&p,
        (uint64_t)&p.position - (uint64_t)&p,
        (uint64_t)&p.velocity - (uint64_t)&p,
        (uint64_t)&p.ang_velocity - (uint64_t)&p,
        (uint64_t)&p.thrust - (uint64_t)&p,
        (uint64_t)&p.forward - (uint64_t)&p,
        (uint64_t)&p.up - (uint64_t)&p,
        (uint64_t)&p.right - (uint64_t)&p,
        (uint64_t)&p.mass - (uint64_t)&p,
        (uint64_t)&p.radius - (uint64_t)&p,
        (uint64_t)&p.health - (uint64_t)&p,
        (uint64_t)&p.obj_type - (uint64_t)&p,
        (uint64_t)&p.art_id - (uint64_t)&p,
        (uint64_t)&p.emits_gravity - (uint64_t)&p);

    struct Diana::SmartPhysicsObject s;
#if __x86_64__
    printf("%lu %lu\n",
#else
    printf("%llu %llu\n",
#endif
        (uint64_t)&s.pobj - (uint64_t)&s,
        (uint64_t)&s.socket - (uint64_t)&s);

    struct Diana::Beam b;
#if __x86_64__
    printf("%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
#else
    printf("%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
#endif
        (uint64_t)&b.type - (uint64_t)&b,
        (uint64_t)&b.phys_id - (uint64_t)&b,
        (uint64_t)&b.universe - (uint64_t)&b,
        (uint64_t)&b.scan_target - (uint64_t)&b,
        (uint64_t)&b.origin - (uint64_t)&b,
        (uint64_t)&b.direction - (uint64_t)&b,
        (uint64_t)&b.up - (uint64_t)&b,
        (uint64_t)&b.right - (uint64_t)&b,
        (uint64_t)&b.cosines - (uint64_t)&b,
        (uint64_t)&b.speed - (uint64_t)&b,
        (uint64_t)&b.area_factor - (uint64_t)&b,
        (uint64_t)&b.energy - (uint64_t)&b,
        (uint64_t)&b.distance_travelled - (uint64_t)&b,
        (uint64_t)&b.max_distance - (uint64_t)&b);
}

int main(int32_t argc, char** argv)
{
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);

    struct Diana::Universe::Parameters params;
    params.verbose_logging = false;
    params.min_vis_frametime = 0.04;
    Diana::Universe* u = new Diana::Universe(params);

    u->start_net();
    u->start_sim();

    double frametimes[4];
    uint64_t last_ticks = u->get_ticks();
    uint64_t cur_ticks;

    std::chrono::seconds dura(1);

    fprintf(stderr, "Physics Framtime, Wall Framtime, Game Frametime, Vis Frametime, Total Sim Time, NTicks\n");
    while (running)
    {
        u->get_frametime(frametimes);
        cur_ticks = u->get_ticks();

#if __x86_64__
        fprintf(stderr, "%g, %g, %g, %g, %g, %lu\n", frametimes[0], frametimes[1], frametimes[2], frametimes[3], u->total_sim_time(), cur_ticks - last_ticks);
#else
        fprintf(stderr, "%g, %g, %g, %g, %g, %llu\n", frametimes[0], frametimes[1], frametimes[2], frametimes[3], u->total_sim_time(), cur_ticks - last_ticks);
#endif

        last_ticks = cur_ticks;
        std::this_thread::sleep_for(dura);
    }

    u->stop_net();
    u->stop_sim();
    delete u;

    return 0;
}

#include <stdio.h>
#include <signal.h>
#include <chrono>

#include "utility.hpp"
#include "universe.hpp"
#include "MIMOServer.hpp"

volatile bool running = true;

void check_packing()
{
    struct Diana::PhysicsObject p;
    // On g++ 64-bit, we need %lu, all other times we need %llu
#if __x86_64__
    printf("%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
#else
    printf("%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
#endif
           (uint64_t)&p.poh.type - (uint64_t)&p,
           (uint64_t)&p.poh.phys_id - (uint64_t)&p,
           (uint64_t)&p.poh.universe - (uint64_t)&p,
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
           (uint64_t)&b.poh.type - (uint64_t)&b,
           (uint64_t)&b.poh.phys_id - (uint64_t)&b,
           (uint64_t)&b.poh.universe - (uint64_t)&b,
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
    check_packing();
    return 0;
}

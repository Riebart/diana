#include <stdio.h>
#include <signal.h>
#include <chrono>

#include "utility.hpp"
#include "universe.hpp"
#include "MIMOServer.hpp"

#include "argparse.hpp"

volatile bool running = true;

void sighandler(int32_t sig)
{
    fprintf(stderr, "Caught Ctrl+C, stopping.\n");
    running = false;
}

#define s(str) (std::string)(str)

int main(int32_t argc, char** argv)
{
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);

    struct Diana::Universe::Parameters params;
    params.verbose_logging = false;
    params.realtime_physics = true;
    params.min_physics_frametime = 0.001;
    Diana::Universe* u = new Diana::Universe(params);

    u->start_net();
    u->start_sim();

    double frametimes[4];
    uint64_t last_ticks = u->get_ticks();
    uint64_t cur_ticks;

    std::chrono::seconds dura(1);

	fprintf(stderr, "Unisim main thread PID: %u\n", get_this_thread_pid());
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

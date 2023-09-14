#include <stdio.h>
#include <signal.h>
#include <chrono>

#include "utility.hpp"
#include "universe.hpp"
#include "MIMOServer.hpp"

#include "argparse.hpp"

#include "__version.hpp"

volatile bool running = true;

void sighandler(int32_t sig)
{
    fprintf(stderr, "Caught Ctrl+C, stopping.\n");
    running = false;
}

void print_version()
{
    printf("git version:     %s-%s\n", GIT_BRANCH, GIT_VERSION);
    printf("git commit date: %s\n", GIT_COMMIT_DATE);
    printf("build date:      %s\n", BUILD_DATE);
}

int main(int32_t argc, char **argv)
{
    struct Diana::Universe::Parameters params;

    ArgumentParser parser(
        argc, argv,
        "unisim", "Simulates universes!",
        80, true);

#include "__universe_args.hpp"

    bool opt_version = parser.get_flag_option("-v", "--version", "Print version of the binary and source code used to generate it. Includes date of last commit, state of working tree when built, and date and time of build", false).result.option_value;

    bool parse_success = parser.finished_parsing();

    if (!parse_success)
    {
        fprintf(stderr, "Error parsing arguments\n");
        parser.print_help();
        return 1;
    }

    if (opt_version)
    {
        print_version();
        return 0;
    }

    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);

    Diana::Universe *u = new Diana::Universe(params);

    u->start_net();
    u->start_sim();

    double frametimes[4];
    uint64_t last_ticks = u->get_ticks();
    uint64_t cur_ticks;

    struct Diana::Universe::TickMetrics tick_metrics;

    std::chrono::seconds dura(1);

    fprintf(stderr, "Unisim main thread PID: %u\n", get_this_thread_pid());
    fprintf(stderr, "Physics Framtime, Wall Framtime, Game Frametime, Vis Frametime, Total Sim Time, NTicks\n");
    while (running)
    {
        u->get_frametime(frametimes);
        cur_ticks = u->get_ticks();
        tick_metrics = u->get_tick_metrics();

#if __x86_64__
        fprintf(stderr, "%g, %g, %g, %g, %g, %lu\n", frametimes[0], frametimes[1], frametimes[2], frametimes[3], u->total_sim_time(), cur_ticks - last_ticks);
#else
        fprintf(stderr, "%g, %g, %g, %g, %g, %llu\n", frametimes[0], frametimes[1], frametimes[2], frametimes[3], u->total_sim_time(), cur_ticks - last_ticks);
#endif

        tick_metrics._fprintf(stderr);

        last_ticks = cur_ticks;
        std::this_thread::sleep_for(dura);
    }

    u->stop_net();
    u->stop_sim();
    delete u;

    return 0;
}

#include "utility.hpp"

#include <stdint.h>
#include <string.h>

#include <fstream>
#include <thread>

#include <unistd.h>
#include <sys/types.h>

uint32_t get_this_thread_pid()
{
    // std::thread::id thread_id = std::this_thread::get_id();

#ifdef _WIN32
    return 0;
#else
    // // The only real way to get the thread PID is to use `/proc/self`
    // std::ifstream infile ("/proc/self/status", std::ios::in | std::ios::binary);
    // char status[4096];
    // infile.read(status, 4096);
    // size_t n_read = infile.tellg();
    // char* pid_marker = "\x0aPid:\x09";
    // uint32_t pid = 0;

    // // Find the instance of "Pid: "
    // for (int i = 0 ; i < n_read - 6; i++)
    // {
    //     if (strncmp(pid_marker, status + i, 6) == 0)
    //     {
    //         sscanf(status + i + 6, "%u", &pid);
    //         break;
    //     }
    // }

    // return pid;

    return gettid();
#endif
}
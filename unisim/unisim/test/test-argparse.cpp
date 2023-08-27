#include <stdio.h>

#include "argparse.hpp"

#define s(str) (std::string)(str)


int main(int argc, char** argv)
{
    ArgumentParser parser(argc, argv, "test-argparse", "Tests the argument parser implementation", 80, true);

    struct option<bool> opt_flag = parser.get_flag_option("-r", "--remove", "This is a short description for a required flag", true);
    struct option<double> opt_double = parser.get_basic_option("-id", "--include-double", 0.002, "This is a long, description. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
    struct option<std::string> opt_string = parser.get_basic_option("-is", "--include-string", s("default_string"), "Required option that takes a value", true);

    fprintf(stderr, "%d %d\n", opt_flag.result.option_present, opt_flag.result.option_value);
    fprintf(stderr, "%d %g\n", opt_double.result.option_present, opt_double.result.option_value);
    fprintf(stderr, "%d %s\n", opt_string.result.option_present, opt_string.result.option_value.data());

    bool parse_complete = parser.finished_parsing();

    fprintf(stderr, "Parse success %d\n", parse_complete);

    return 0;
}
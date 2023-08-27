#include <stdio.h>

#include "argparse.hpp"

int main(int argc, char** argv)
{
    struct option<bool> opt_flag = get_flag_option(argc, argv, "-r", "--remove");
    struct option<double> opt_double = get_basic_option(argc, argv, "-id", "--include-double", 0.002);
    struct option<std::string> opt_string = get_basic_option(argc, argv, "-is", "--include-string", s("default_string"));

    fprintf(stderr, "%d %d\n", opt_flag.result.option_present, opt_flag.result.option_value);
    fprintf(stderr, "%d %g\n", opt_double.result.option_present, opt_double.result.option_value);
    fprintf(stderr, "%d %s\n", opt_string.result.option_present, opt_string.result.option_value.data());

    return 0;
}
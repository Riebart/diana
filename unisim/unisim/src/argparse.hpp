#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP

#include <string>
#include <charconv>

template <typename T> struct option_result {
    bool option_present;
    bool option_valid;
    std::string option_string;
    T option_value;
};

template<typename T> struct option {
    option() :
        short_option(""),
        long_option(""),
        option_description(""),
        is_flag(false),
        is_required(false),
        validator(NULL) {}

    std::string short_option;
    std::string long_option;
    T default_value;
    std::string option_description;
    bool is_flag;
    bool is_required;
    T (*validator)(struct option_result<T> value);
    struct option_result<T> result;
};

template <typename T> struct option_result<T> get_option_value(int argc, char** argv, struct option<T>* option_def)
{
    option_def->result.option_present = false;
    // Prematurely set the value to the default.
    // It'll get set to something else if we actually find the option.
    option_def->result.option_value = option_def->default_value;

    for (int i = 0; i < argc; ++i)
    {
        std::string arg = argv[i];
        std::string option;

        bool found_long_option = ((option_def->long_option != "") && (arg.find(option_def->long_option) == 0));
        bool found_short_option = ((option_def->short_option != "") && (arg.find(option_def->short_option) == 0));

        if (found_long_option){
            option = option_def->long_option;
        }
        else if (found_short_option) {
            option = option_def->short_option;
        }

        // If we found either option
        if(found_long_option || found_short_option)
        {
            option_def->result.option_present = true;

            // For non-flag options, get the string that is the value.
            if (!option_def->is_flag)
            {
                std::size_t found = arg.find_first_of(option);
                
                // There are 3 ways we can have a value provided:
                //
                
                //   This one will be split by the shell, so there will be no characters
                //   left after parsing the option string
                //
                // prog -aval
                //   Find this one by distinguishing it from

                // If the argument length is the option length, then the
                // value is guaranteed to be in the next arg, if it exists.
                //
                // Example: prog -a val
                if ((arg.length() == option.length()) && (i < (argc - 1)))
                {
                    option_def->result.option_string = argv[i+1];
                    i++;
                }
                // If the character immediately following the option string is '='
                // Then we can substring after '=' for the value string
                //
                // Example: prog -a=val
                else if ((arg.length() >= option.length() + 1) && (arg[option.length()] == '='))
                {
                    option_def->result.option_string = arg.substr(found + option.length() + 1);
                }
                // This is just, the last option.
                //
                // prog -aval
                else if (arg.length() >= option.length() + 1)
                {
                    option_def->result.option_string = arg.substr(found + option.length());
                }
                else
                {
                    // Uhh.... I don't know how we got here?
                }
            }

            // Validate/parse the option value.
            option_def->result.option_value = option_def->validator(option_def->result);
        }
        // else if(option_def->is_required)
        // {
        // }
    }

    return option_def->result;
}

#define INIT_OPT_STRUCT(t) struct option<t> opt; \
    opt.short_option = short_option; \
    opt.long_option = long_option; \
    opt.default_value = default_value;

struct option<bool> get_flag_option(int argc, char** argv, std::string short_option, std::string long_option)
{
    bool default_value = false;
    INIT_OPT_STRUCT(bool);
    opt.is_flag = true;
    opt.validator = [](struct option_result<bool> optr) {
        optr.option_valid = true;
        return optr.option_present;
    };

    get_option_value(argc, argv, &opt);
    return opt;
}

struct option<std::string> get_basic_option(int argc, char** argv, std::string short_option, std::string long_option, std::string default_value)
{
    INIT_OPT_STRUCT(std::string);
    opt.is_flag = false;
    opt.validator = [](struct option_result<std::string> optr) {
        return optr.option_string;
    };

    get_option_value(argc, argv, &opt);
    return opt;
}

template<typename T> struct option<T> get_basic_option(int argc, char** argv, std::string short_option, std::string long_option, T default_value)
{
    INIT_OPT_STRUCT(T);
    opt.is_flag = false;
    opt.validator = [](struct option_result<T> optr) {
        T result = optr.option_value; // Set it to the current value, which by construction is the default value;
        // Technically, we should check the error code to make sure nothing bad happened.
        auto [prt, ec] = std::from_chars(
            optr.option_string.data(),
            optr.option_string.data() + optr.option_string.length(),
            result);
        optr.option_valid = (ec == std::errc()); // All OK
        return result;
    };

    get_option_value(argc, argv, &opt);
    return opt;
}

#endif

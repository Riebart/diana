#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP

#include <string>

template <typename T> struct option_result {
    bool option_present;
    std::string option_string;
    T option_value;
};

template<typename T> struct option {
    std::string short_option;
    std::string long_option;
    std::string option_description;
    bool is_flag;
    bool is_required;
    T (*validator)(struct option_result<T> value);
};

bool get_flag_option(int argc, char** argv, std::string short_option, std::string long_option);
template <typename T> T get_basic_option(int argc, char** argv, std::string short_option, std::string long_option);
template <typename T> struct option_result<T> get_option_value(int argc, char** argv, struct option<T> option_def);

// DEFINITIONS

template <typename T> struct option_result<T> get_option_value(int argc, char** argv, struct option<T> option_def)
{
    struct option_result<T> result;
    result.option_present = false;

    for (int i = 0; i < argc; ++i)
    {
        std::string arg = argv[i];
        std::string option;

        bool found_long_option = (arg.find(option_def.long_option) == 0);
        bool found_short_option = (arg.find(option_def.short_option) == 0);

        if (found_long_option){
            option = option_def.long_option;
        }
        else if (found_short_option) {
            option = option_def.short_option;
        }

        // If we found either option
        if(found_long_option || found_short_option)
        {
            result.option_present = true;

            if (!option_def.is_flag)
            {
                std::size_t found = arg.find_first_of(option);
                
                // Look at the character after the option
                // If it's an equals, then split this argv
                // Otherwise, look at the next argv
                if ((arg.length() >= option.length()+1) && (arg[option.length()] == '='))
                {
                    result.option_string = arg.substr(found + option.length() + 1);
                }
                else
                {
                    if (i < (argc - 1))
                    {
                        result.option_string = argv[i+1];
                        i++;
                    }
                }

                result.option_value = option_def.validator(result.option_string);
            }
        }
        else if(option_def.is_required)
        {
        }
    }

    return result;
}

bool get_flag_option(int argc, char** argv, std::string short_option, std::string long_option)
{
    struct option<bool>
}

template <typename T> T get_basic_option(int argc, char** argv, std::string short_option, std::string long_option)
{

}

// auto getCmdOption(int argc, char** argv, std::string short_option, std::string long_option, bool is_flag, bool is_required = false)
// {
//     if (is_flag)
//     {
//         struct option<bool> option;
//         option.is_flag = is_flag;
//         option.is_required = is_required;
//         option.short_option = short_option;
//         option.long_option = long_option;
//         option.validator = [](struct option_result<bool> optr) { return optr.option_present; };-
//         return getCmdOption<bool>(argc, argv, option);
//     }
//     else
//     {
//         struct option<std::string> option;
//         option.is_flag = is_flag;
//         option.is_required = is_required;
//         option.short_option = short_option;
//         option.long_option = long_option;
//         option.validator = [](struct option_result<std::string> optr) { return optr.option_string; };
//         return getCmdOption<std::string>(argc, argv, option);
//     }
// }

#endif

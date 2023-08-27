#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP

#include <iostream>
#include <string>
#include <vector>
#include <charconv>
#include <iomanip>

#define __INIT_OPT_STRUCT(t) struct option<t> opt; \
    opt.short_option = short_option; \
    opt.long_option = long_option; \
    opt.default_value = default_value; \
    opt.option_description = description; \
    opt.is_required = is_required;

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

// The ArgumentParser class takes care of handling global-behaviours:
// - --help behaviour, accomplished
class ArgumentParser
{
    int argc;
    char** argv;
    size_t descrption_wrap_width = 80;
    bool error_while_parsing = false;

    std::string progname;
    std::string proghelp;

    struct arg_help
    {
        std::string name;
        std::string description;
        bool is_flag;
        bool is_required;
    };

    std::vector<struct arg_help> arg_help_strings;

    void log_argument(std::string _name, std::string _description, bool _is_flag, bool _is_required)
    {
        struct arg_help item;
        item.name = _name;
        item.description = _description;
        item.is_flag = _is_flag;
        item.is_required = _is_required;

        this->arg_help_strings.push_back(item);
    }

    static std::string wrap_string(std::string str, size_t wrap_length, std::string newline_padding = "")
    {
        size_t wrap_point = wrap_length;
        int num_wraps = 0;

        while (wrap_point < str.length())
        {
            size_t cur_wrap = str.rfind(' ', wrap_point);
            if (cur_wrap != std::string::npos)
            {
                str.at(cur_wrap) = '\n';
                if (newline_padding != "")
                {
                    str = str.insert(cur_wrap + 1, newline_padding);
                }
                wrap_point = cur_wrap + wrap_length + newline_padding.length();
                num_wraps++;
            }
        }

        if (num_wraps > 0)
        {
            str = str.append("\n");
        }

        return str;
    }

    public:
        ArgumentParser(int _argc, char** _argv, std::string _progname, std::string _proghelp, size_t wrap_width = 80)
        {
            this->argc = _argc;
            this->argv = _argv;

            this->progname = _progname;
            this->proghelp = _proghelp;

            this->descrption_wrap_width = wrap_width;

            this->error_while_parsing = false;
        }

        // Perform final-parsing work, including checking for help, 
        bool finished_parsing()
        {
            bool help_opt = this->get_flag_option("", "--help").result.option_value;
            bool parsing_success = false;

            if (help_opt)
            {
                this->print_help();
                parsing_success = false;
            }
            else
            {
                parsing_success = !this->error_while_parsing;
            }

            return parsing_success;
        }

        void print_help()
        {
            std::cout << "Usage: " << this->progname;
            size_t max_name_length = 0;

            for (std::vector<struct arg_help>::iterator it = this->arg_help_strings.begin() ; 
                    it != this->arg_help_strings.end() ; ++it)
            {
                if (it->is_required)
                {
                    std::cout << " <" << it->name;
                    if (!it->is_flag)
                    {
                        std::cout << "=";
                    }
                    std::cout << ">";
                }

                if (it->name.length() > max_name_length)
                {
                    max_name_length = it->name.length();
                }
            }

            std::cout << " [other options]" << std::endl << std::endl;
            std::cout << this->proghelp << std::endl << std::endl;
            std::string description_wrap_padding(max_name_length + 2 + 4, ' ');

            for (std::vector<struct arg_help>::iterator it = this->arg_help_strings.begin() ; it != this->arg_help_strings.end() ; ++it)
            {
                std::cout << 
                    std::setw(0) << "    " << 
                    std::left << 
                    std::setfill(' ') << 
                    std::setw(max_name_length + 2) << it->name << 
                    this->wrap_string(
                        (it->is_flag ? "(flag) " : "") + it->description,
                        this->descrption_wrap_width,
                        description_wrap_padding) << 
                    std::endl; 
            }
        }
    
        template <typename T> struct option_result<T> get_option_value(struct option<T>* option_def)
        {
            this->log_argument(
                (option_def->long_option != "" ? 
                    option_def->long_option :
                    option_def->short_option),
                option_def->option_description,
                option_def->is_flag,
                option_def->is_required);

            option_def->result.option_present = false;
            // Prematurely set the value to the default.
            // It'll get set to something else if we actually find the option.
            option_def->result.option_value = option_def->default_value;

            for (int i = 0; i < this->argc; ++i)
            {
                std::string arg = this->argv[i];
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
                if (found_long_option || found_short_option)
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
                        if ((arg.length() == option.length()) && (i < (this->argc - 1)))
                        {
                            option_def->result.option_string = this->argv[i+1];
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
            }

            if (!option_def->result.option_present && option_def->is_required)
            {
                this->error_while_parsing = true;
            }

            return option_def->result;
        }

        struct option<bool> get_flag_option(std::string short_option, std::string long_option, std::string description = "", bool is_required = false)
        {
            bool default_value = false;
            __INIT_OPT_STRUCT(bool);
            opt.is_flag = true;
            opt.validator = [](struct option_result<bool> optr) {
                optr.option_valid = true;
                return optr.option_present;
            };

            get_option_value(&opt);
            return opt;
        }

        struct option<std::string> get_basic_option(std::string short_option, std::string long_option, std::string default_value, std::string description = "", bool is_required = false)
        {
            __INIT_OPT_STRUCT(std::string);
            opt.is_flag = false;
            opt.validator = [](struct option_result<std::string> optr) {
                return optr.option_string;
            };

            get_option_value(&opt);
            return opt;
        }

        template<typename T> struct option<T> get_basic_option(std::string short_option, std::string long_option, T default_value, std::string description = "", bool is_required = false)
        {
            __INIT_OPT_STRUCT(T);
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

            get_option_value(&opt);
            return opt;
        }

};

#endif

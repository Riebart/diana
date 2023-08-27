#!/bin/bash

# cat lib/include/universe.hpp | sed -n '/^ *struct Parameters$/{:a;/\n *\};/!{N;ba};s/\n/¶/gp}'

parameters=$(cat lib/include/universe.hpp |
    sed -n '/^ *struct Parameters$/{:a;/\n *\};/!{N;ba};p}' |
    sed -n 's/^ *\([a-z]*\) \([a-z_]*\);/\1 \2/p')

echo "$parameters" | while read type var
do
    description=$(tac lib/include/universe.hpp | 
        sed -n "/$type $var/{:a;/\n$/!{N;ba};p}" |
        tail -n+2 | tac |
        sed 's|^ *//||;s/^ *//;s/ *$//' |
        grep -v "^$" | tr '\n' ' ')
    
    echo "$description" | xxd >&2
    
    cli_arg_str=$(echo "$var" | tr '_' '-')
    default_value=$(cat lib/include/universe.hpp | sed -n "s/^ *${var}(\([^)]*\)),*$/\1/p")

    if [ "$type" == "bool" ]
    then
        echo "${type} opt_${var} = parser.get_flag_option(
            \"\",
            \"--${cli_arg_str}\",
            \"${description}\", false).result.option_value;"
    else
        echo "${type} opt_${var} = parser.get_basic_option(
            \"\",
            \"--${cli_arg_str}\",
            ${default_value},
            \"${description}\", false).result.option_value;"
    fi
    echo "params.${var} = opt_${var};"
done > src/__universe_args.hpp
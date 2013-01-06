#!/bin/sh

# This runs through all of the script files named ##-script in this directory
# and sleeps for a second in between each.

for s in `ls -1 *script | sort -n`
do
    cat $s
    sleep 1
done

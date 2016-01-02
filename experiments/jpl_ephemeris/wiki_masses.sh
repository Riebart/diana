#!/bin/bash

wget -qO- "https://en.wikipedia.org/wiki/$1_(moon)" | grep -m1 -A1 Mass | tail -n1 | tr '><' '\n' | grep -E "^[0-9.]+$" | tr '\n' ' ' | cut -d' ' -f1,3 | tr ' ' 'e'
#wget -qO- "https://en.wikipedia.org/wiki/$1_(moon)" | grep -m1 -A1 Mass | tail -n1 | tr '><' '\n'


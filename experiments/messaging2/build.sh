astyle -j --style=ansi --suffix=none --keep-one-line-blocks *cpp *hpp
g++ -O3 --std=c++23 -Wall -Wpedantic -o v1 v1_naive_struct.cpp &&
g++ -O3 --std=c++23 -Wall -Wpedantic -o v2 v2_chained_templates.cpp &&
./v1 &&
echo "---" &&
./v2

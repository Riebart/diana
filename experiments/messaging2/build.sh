astyle -j --style=ansi --suffix=none --keep-one-line-blocks *cpp *hpp
echo "BUILDING V1" &&
g++ -O3 --std=c++23 -Wall -Wpedantic -o bin/v1 v1_naive_struct.cpp &&
echo "BUILDING V2" &&
g++ -O3 --std=c++23 -Wall -Wpedantic -o bin/v2 v2_chained_templates.cpp &&
echo "BUILDING V4" &&
gcc -O3 -Wall -Wpedantic -o bin/v4 v4_c_macro.c &&
./bin/v1 &&
echo "---" &&
./bin/v2 &&
echo "---" &&
./bin/v4 "1 1 0 0 1 0"
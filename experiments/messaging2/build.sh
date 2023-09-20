astyle -j --style=ansi --suffix=none --keep-one-line-blocks *cpp *hpp
echo "BUILDING V1" &&
g++ -O3 --std=c++23 -Wall -Wpedantic -o v1 v1_naive_struct.cpp &&
echo "BUILDING V2" &&
g++ -O3 --std=c++23 -Wall -Wpedantic -o v2 v2_chained_templates.cpp &&
: && # echo "BUILDING V3" &&
: && # g++ -O3 --std=c++23 -Wall -Wpedantic -o v3 v3_chained_stdoptionals.cpp &&
./v1 &&
echo "---" &&
./v2 &&
# echo "---" &&
# ./v3
:
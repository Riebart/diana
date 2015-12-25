#include "../unisim/unisim/bson.hpp"
#include <stdio.h>

int main(int argc, char** argv)
{
    BSONWriter bw;
    bw.push(5.05);
    bw.push((char*)"This is a lovely string.");
    uint8_t* out = bw.push_end();
    fwrite(out, 1, *(int32_t*)out, stdout);

    return 0;
}


#include <cstdint>

// Needed for htonl and stuff when rendering the byte string of the structures.
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#define ntohll(x) htonll(x)

template <typename T> inline T htons(T x)
{
    return htons((uint16_t)x);
}

template <typename T> inline T htonl(T x)
{
    return htonl((uint32_t)x);
}

template <typename T> inline T htonll(T x)
{
    return ((( (uint64_t)htonl((uint64_t)x)) << 32 ) + (uint64_t)htonl(((uint64_t)x) >> 32 ) );
}

template <typename T> void __hton(T& value)
{
    switch (sizeof(T))
    {
    case 2:
        value = htons<T>(value);
        break;
    case 4:
        value = htonl<T>(value);
        break;
    case 8:
        value = htonll<T>(value);
        break;
    default:
        break;
    }
}

template <>
void __hton(double& value)
{
    value = htonll<double>(value);
}

template <>
void __hton(std::uint64_t& value)
{
    value = htonll<std::uint64_t>(value);
}

template <>
void __hton(std::int64_t& value)
{
    value = htonll<std::int64_t>(value);
}

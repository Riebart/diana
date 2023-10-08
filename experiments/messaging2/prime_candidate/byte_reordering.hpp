#ifndef BYTE_REORDERING_HPP
#define BYTE_REORDERING_HPP

#include <cstdint>

// Needed for htonl and stuff when rendering the byte string of the structures.
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include <byteswap.h>

template <typename T> inline T _htons(T x)
{
    return htons((uint16_t)x);
}

template <typename T> inline T _ntohs(T x)
{
    return ntohs((uint16_t)x);
}

template <typename T> inline T _htonl(T x)
{
    return htonl((uint32_t)x);
}

template <typename T> inline T _ntohl(T x)
{
    return ntohl((uint32_t)x);
}

template <typename T> inline T _htonll(T x)
{
    auto words = reinterpret_cast<std::uint32_t*>(&x);
    words[0] = htonl(words[0]);
    words[1] = htonl(words[1]);
    return x;
}

// The below is identical because of the way endianness works at larger scales.
// Equivalent to: #define ntohll(x) htonll(x)
template <typename T> inline T _ntohll(T x)
{
    auto words = reinterpret_cast<std::uint32_t*>(&x);
    words[0] = htonl(words[0]);
    words[1] = htonl(words[1]);
    return x;
}

template <typename T> void __hton(T& value)
{
    switch (sizeof(T))
    {
    case 2:
        value = _htons<T>(value);
        break;
    case 4:
        value = _htonl<T>(value);
        break;
    case 8:
        value = _htonll<T>(value);
        break;
    default:
        break;
    }
}

template <typename T> void __ntoh(T& value)
{
    switch (sizeof(T))
    {
    case 2:
        value = _ntohs<T>(value);
        break;
    case 4:
        value = _ntohl<T>(value);
        break;
    case 8:
        value = _ntohll<T>(value);
        break;
    default:
        break;
    }
}

template <>
void __hton(double& value)
{
    value = _htonll<double>(value);
}

template <>
void __hton(std::uint64_t& value)
{
    value = _htonll<std::uint64_t>(value);
}

template <>
void __hton(std::int64_t& value)
{
    value = _htonll<std::int64_t>(value);
}

template <>
void __hton(char*& value) {}

template <>
void __ntoh(double& value)
{
    value = _ntohll<double>(value);
}

template <>
void __ntoh(std::uint64_t& value)
{
    value = _ntohll<std::uint64_t>(value);
}

template <>
void __ntoh(std::int64_t& value)
{
    value = _ntohll<std::int64_t>(value);
}

template <>
void __ntoh(char*& value) {}

#endif

// Implementes basic element types, that support hton, ntoh
// reflection on their own size, and optionality
//
// Provides basic support for Vector3 and Vector4 template types

#include <string>
#include <string.h>

#include "byte_reordering.hpp"

// #pragma pack(1)
template <typename T>
struct Element
{
    T value;

    bool read(std::uint8_t* data)
    {
        return false;
    }

    void hton()
    {
        __hton<T>(value);
    }

    std::size_t dump_size()
    {
        return sizeof(T);
    }

    void operator=(T newval)
    {
        this->value = newval;
    }
};

// #pragma pack(1)
template<>
struct Element<char*>
{
    char* value;
    std::size_t num_chars = 0;

    void hton() {}

    std::size_t dump_size()
    {
        return num_chars;
    }

    void operator=(char* newval)
    {
        this->value = newval;
        this->num_chars = strnlen(this->value, 4096);
    }
};

// #pragma pack(1)
template <typename T>
struct OptionalElement : public Element<T>
{
    bool present = false;

    bool read(std::uint8_t* data)
    {
        return false;
    }

    void operator=(T newval)
    {
        this->present = true;
        this->value = newval;
    }
};

template <typename T>
struct Vector3
{
    T x, y, z;
    Vector3(): x(), y(), z() {}
    Vector3(T x, T y, T z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }
};

template <typename T>
struct Vector4
{
    T w, x, y, z;
    Vector4(): w(), x(), y(), z() {}
    Vector4(T w, T x, T y, T z)
    {
        this->w = w;
        this->x = x;
        this->y = y;
        this->z = z;
    }
};

template <>
void Element<Vector3<double>>::hton()
{
    value.x = htonll(value.x);
    value.y = htonll(value.y);
    value.z = htonll(value.z);
}

template <>
void Element<Vector4<double>>::hton()
{
    value.w = htonll(value.w);
    value.x = htonll(value.x);
    value.y = htonll(value.y);
    value.z = htonll(value.z);
}

template <> void Element<std::string>::hton() {}
template <> std::size_t Element<std::string>::dump_size()
{
    return this->value.size();
}

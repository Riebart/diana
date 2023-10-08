#include <iostream>
#include <cstdint>

#include "byte_reordering.hpp"

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

    friend std::ostream& operator<<(std::ostream& os, const struct Vector3<T>& v)
    {
        os << "{\"x\":"<< v.x << ",\"y\":" << v.y << ",\"z\":" << v.z << "}";
        return os;
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

    friend std::ostream& operator<<(std::ostream& os, const struct Vector4<T>& v)
    {
        os << "{\"w\":" << v.w << ",\"x\":"<< v.x << ",\"y\":" << v.y << ",\"z\":" << v.z << "}";
        return os;
    }
};

template <>
void Element<Vector3<double>>::hton()
{
    value.x = _htonll(value.x);
    value.y = _htonll(value.y);
    value.z = _htonll(value.z);
}

template <>
void Element<Vector4<double>>::hton()
{
    value.w = _htonll(value.w);
    value.x = _htonll(value.x);
    value.y = _htonll(value.y);
    value.z = _htonll(value.z);
}

template <>
void Element<Vector3<double>>::ntoh()
{
    value.x = _ntohll(value.x);
    value.y = _ntohll(value.y);
    value.z = _ntohll(value.z);
}

template <>
void Element<Vector4<double>>::ntoh()
{
    value.w = _ntohll(value.w);
    value.x = _ntohll(value.x);
    value.y = _ntohll(value.y);
    value.z = _ntohll(value.z);
}

template <> void Element<std::string>::hton() {}
template <> void Element<std::string>::ntoh() {}
template <> std::size_t Element<std::string>::binary_size()
{
    return this->value.size();
}

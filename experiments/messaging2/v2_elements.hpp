// Implementes basic element types, that support hton, ntoh
// reflection on their own size, and optionality
//
// Provides basic support for Vector3 and Vector4 template types

#include <string>
#include <string.h>
#include <sstream>

#include "byte_reordering.hpp"

template<size_t N>
struct StringLiteral
{
    constexpr StringLiteral(const char (&str) [N])
    {
        std::copy_n(str, N, value);
    }
    char value[N];
};

template <typename T>
struct Element
{
    T value;

    Element() { value = T(); }
    bool read(std::uint8_t* data) { return false; }
    void hton() { __hton<T>(value); }
    std::size_t dump_size() { return sizeof(T); }
    void operator=(T newval) { this->value = newval; }
    operator T() const { return value; }

    bool json(std::string* s, int n)
    {
        std::ostringstream os;
        os << value;
        s->append(os.str());
        return true;
    }
};

template <typename T, StringLiteral Name>
struct NamedElement
{
    struct Element<T> element;

    NamedElement() : element() { }
    bool read(std::uint8_t* data) { return element.read(); }
    void hton() { element.hton(); }
    std::size_t dump_size() { return element.dump_size(); }
    void operator=(T newval) { element = newval; }
    operator T() const { return (T)element; }

    bool json(std::string* s, int n)
    {
        s->append("\"");
        s->append(Name.value);
        s->append("\":");
        element.json(s, n);
        return true;
    }
};

template<>
struct Element<const char*>
{
    const char* value = NULL;
    std::size_t num_chars = 0;

    void hton() {}
    std::size_t dump_size() { return num_chars + 1; }

    void operator=(const char* newval)
    {
        this->value = newval;
        this->num_chars = strnlen(this->value, 4096);
    }

    bool json(std::string* s, int n)
    {
        if (value == NULL)
        {
            s->append("\"\"");
        }
        else
        {
            s->append("\"");
            s->append(value);
            s->append("\"");
        }
        return true;
    }

    operator const char*() const { return value; }
};

template <typename T>
struct OptionalElement : Element<T>
{
    bool present = false;

    bool read(std::uint8_t* data) { return false; }
    std::size_t dump_size() { return 1 + (present ? Element<T>::dump_size() : 0); }

    void operator=(T newval)
    {
        this->present = true;
        Element<T>::operator = (newval);
    }

    bool json(std::string* s, int n)
    {
        if (present)
        {
            Element<T>::json(s, n);
        }
        return present;
    }

    operator bool() const { return present; }
};

template <typename T, StringLiteral Name>
struct NamedOptionalElement
{
    struct OptionalElement<T> element;
    NamedOptionalElement() : element() { }
    bool read(std::uint8_t* data) { return element.read(); }
    void hton() { element.hton(); }
    std::size_t dump_size() { return element.dump_size(); }
    void operator=(T newval) { element = newval; }

    bool json(std::string* s, int n)
    {
        if (element.present)
        {
            s->append("\"");
            s->append(Name.value);
            s->append("\":");
            element.json(s, n);
        }
        return element.present;
    }
};

// template <>
// struct OptionalElement<bool> : Element<bool>
// {
//     bool present = false;

//     bool read(std::uint8_t* data)
//     {
//         return false;
//     }

//     std::size_t dump_size()
//     {
//         return 1 + (present * (sizeof(bool) - 1));
//     }

//     void operator=(bool newval)
//     {
//         this->present = true;
//         this->value = newval;
//     }

//     operator bool() const { return present; }
//     explicit operator bool() const { return value; }
// };

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
        os << "{\"x\": "<< v.x << ",\"y\": " << v.y << ",\"z\": " << v.z << "}";
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
        os << "{\"w\": " << v.w << "\"x\": "<< v.x << ",\"y\": " << v.y << ",\"z\": " << v.z << "}";
        return os;
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

template <typename T> struct ChainedElement
{
    T next;
    std::size_t dump_size() { return next.dump_size(); }

    bool json(std::string* s, int n)
    {
        bool result = next.json(s, n);
        if (result && (n > 0))
        {
            s->append(",");
        }
        return result;
    }
};

template <typename T, typename TNext>
struct ElementC : ChainedElement<TNext>
{
    struct Element<T> element;

    std::size_t dump_size()
    {
        return element.dump_size() + ChainedElement<TNext>::dump_size();
    }

    bool json(std::string* s, int n)
    {
        ChainedElement<TNext>::json(s, n);
        s->append(element.json(n + 1));
        return true;
    }
};

template <typename T, StringLiteral Name, typename TNext>
struct NamedElementC : ChainedElement<TNext>
{
    struct Element<T> element;

    std::size_t dump_size() { return element.dump_size(); }

    bool json(std::string* s, int n)
    {
        ChainedElement<TNext>::json(s, n+1);
        s->append("\"");
        s->append(Name.value);
        s->append("\":");
        element.json(s, n);
        return true;
    }
};

template <typename T, typename TNext>
struct OptionalElementC : ChainedElement<TNext>
{
    struct OptionalElement<T> element;

    std::size_t dump_size()
    {
        return element.dump_size() + ChainedElement<TNext>::dump_size();
    }

    bool json(std::string* s, int n)
    {
        ChainedElement<TNext>::json(s, n + (this->element.present));
        if (element.present)
        {
            element.json(s, n);
        }
        return element.present;
    }
};

template <typename T, StringLiteral Name, typename TNext>
struct NamedOptionalElementC : ChainedElement<TNext>
{
    struct OptionalElement<T> element;

    bool json(std::string* s, int n)
    {
        ChainedElement<TNext>::json(s, n + (this->element.present));

        if (this->element.present)
        {
            s->append("\"");
            s->append(Name.value);
            s->append("\":");
            this->element.json(s, n);
        }
        return this->element.present;
    }
};
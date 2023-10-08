#ifndef REFLECTION_HPP
#define REFLECTION_HPP

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string.h> // Needed for strncpy
#include <string>

#include "macro_iterators.hpp"
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

    Element() : value() { }
    Element(T value)
    {
        this->value = value;
    }

    void hton()
    {
        __hton<T>(value);
    }

    void ntoh()
    {
        __ntoh<T>(value);
    }

    std::size_t binary_size() const
    {
        return sizeof(T);
    }

    std::size_t binary_read(std::uint8_t* data)
    {
        this->operator=(*(T*)data);
        this->ntoh();
        return sizeof(T);
    }

    std::size_t binary_write(std::uint8_t* buf) const
    {
        auto dest = (Element<T>*)buf;
        dest->operator=(value);
        dest->hton();
        return sizeof(T);
    }

    void operator=(T newval)
    {
        this->value = newval;
    }

    operator T() const
    {
        return value;
    }

    bool operator==(const Element<T>& other) const
    {
        // std::cerr << value << " " << other.value << std::endl;
        return value == other.value;
    }

    bool operator==(const T& other) const
    {
        return value == other;
    }

    bool json(std::string* s) const
    {
        // // The ostream approach is more flexible, but less efficient.
        // // It is about 30-40% slower for JSON (still like 2M elements/s, or 100MB/s)
        // // But the std::to_string() option is better overall.
        // std::ostringstream os;
        // os.precision(6); // Use std::fixed inline below as well to specify the output precision.
        // os << std::fixed << value;
        // s->append(os.str());
        s->append(std::to_string(value));
        return true;
    }
};

template <> void Element<std::string>::hton() {}
template <> void Element<std::string>::ntoh() {}
template <> bool Element<std::string>::json(std::string* s) const
{
    s->append("\"");
    s->append(value);
    s->append("\"");
    return true;
}
template <> std::size_t Element<std::string>::binary_size() const
{
    return this->value.length(); /*This is a synonym for .size()*/
}
template <> std::size_t Element<std::string>::binary_read(std::uint8_t* data)
{
    value = (const char*)data;
    return value.length() + 1;
}
template <> std::size_t Element<std::string>::binary_write(std::uint8_t* buf) const
{
    std::size_t num_chars = value.length();
    memcpy(buf, value.c_str(), num_chars);
    buf[num_chars] = 0;
    return num_chars + 1;
}

template<>
struct Element<const char*>
{
    const char* value = NULL;
    std::size_t num_chars = 0;
    bool free_mem = false;

    Element() : value(NULL), num_chars(0), free_mem(false) {}
    // ~Element() { if (free_mem) { delete value; }}
    inline void hton() {};
    void ntoh() {}

    inline std::size_t binary_size() const
    {
        return num_chars + 1;
    }

    inline std::size_t binary_read(std::uint8_t* data)
    {
        this->operator=((const char*)data);
        char* copy = new char[num_chars + 1];
        memcpy(copy, value, num_chars + 1);
        value = copy;
        free_mem = true;
        return num_chars + 1;
    }

    inline std::size_t binary_write(std::uint8_t* buf) const
    {
        memcpy(buf, value, num_chars);
        buf[num_chars] = 0;
        return num_chars + 1;
    }

    inline void operator=(const char* newval)
    {
        if (free_mem)
        {
            delete value;
            free_mem = false;
        }
        this->value = newval;
        this->num_chars = strnlen(this->value, 4096);
    }

    inline bool operator==(const Element<const char*>& other) const
    {
        return (num_chars == other.num_chars) && (strncmp(value, other.value, num_chars) == 0);
    }

    inline operator const char*() const
    {
        return value;
    }

    inline bool json(std::string* s) const
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
};

template <typename T>
struct Optional : Element<T>
{
    bool present;

    Optional() : Element<T>(), present(false) {}
    inline bool operator==(const Optional<T>& other) const
    {
        bool result = (present == other.present);
        if (result && present)
        {
            result = Element<T>::operator==(static_cast<Element<T>>(other));
        }
        return result;
    }

    inline bool operator==(const T& other) const
    {
        if (present)
        {
            return (this->value == other);
        }
        else
        {
            return false;
        }
    }

    inline std::size_t binary_size() const
    {
        return 1 + (present ? Element<T>::binary_size() : 0);
    }

    inline std::size_t binary_read(std::uint8_t* data)
    {
        present = (bool)data[0];
        return 1 + (present ? Element<T>::binary_read(data + 1) : 0);
    }

    inline std::size_t binary_write(std::uint8_t* buf) const
    {
        buf[0] = present;
        return 1 + (present ? Element<T>::binary_write(buf + 1) : 0);
    }

    inline void operator=(T newval)
    {
        this->present = true;
        Element<T>::operator = (newval);
    }

    inline bool json(std::string* s) const
    {
        if (present)
        {
            Element<T>::json(s);
        }
        return present;
    }

    inline operator bool() const
    {
        return present;
    }
};

template <typename T, StringLiteral Name>
struct NamedElement : Element<T>
{
    NamedElement() : Element<T>() {}

    inline bool json(std::string* s) const
    {
        s->append("\"");
        s->append(Name.value);
        s->append("\":");
        Element<T>::json(s);
        return true;
    }

    inline bool json(std::string* s, int n) const
    {
        s->append("\"");
        s->append(Name.value);
        s->append("\":{\"index\":");
        s->append(std::to_string(n));
        s->append(",\"value\":");
        Element<T>::json(s);
        s->append("}");
        return true; // Because there's always an element emitted, even if there's no set value.
    }
};

template <typename OptionalT, StringLiteral Name>
struct NamedElement<Optional<OptionalT>, Name> : Optional<OptionalT>
{
    NamedElement() : Optional<OptionalT>() {}

    inline bool json(std::string* s) const
    {
        if (this->present)
        {
            s->append("\"");
            s->append(Name.value);
            s->append("\":");
            Optional<OptionalT>::json(s);
        }
        return this->present;
    }

    inline bool json(std::string* s, int n) const
    {
        s->append("\"");
        s->append(Name.value);
        s->append("\":{\"index\":");
        s->append(std::to_string(n));
        s->append(",\"value\":");
        if (this->present)
        {
            Optional<OptionalT>::json(s);
        }
        else
        {
            s->append("null");
        }
        s->append("}");
        return true; // Because there's always an element emitted, even if there's no set value.
    }
};

//@NOTE Technically, this has a size when included, but not when inherited
// See: https://stackoverflow.com/questions/4041447/how-is-stdtuple-implemented
// Since we're not actually _storing_ this, just using it as a structure for casting purposes
// and as a chained accessor, then we can ignore it, largely.
//
// Casting a struct of the right shape to the _type_ of a link chain that ends with this
// will be harmless, and specializing the Link structure with the Empty as the Next type
// and not having a TNext member also removes this overhead.
struct Empty { };

// This assumes that T is a NamedElement, and TNext is another Link.
// Otherwise this won't compile.
template <typename T, typename TNext>
struct Link
{
    T value;
    TNext next;

    inline void hton()
    {
        value.hton();
        next.hton();
    }

    inline void ntoh()
    {
        value.ntoh();
        next.ntoh();
    }

    inline std::size_t binary_size() const
    {
        return value.binary_size() + next.binary_size();
    }

    std::size_t binary_read(std::uint8_t* data)
    {
        std::size_t count = 0;
        count += value.binary_read(data);
        data += count;
        count += next.binary_read(data);
        return count;
    }

    std::size_t binary_write(std::uint8_t* buf) const
    {
        std::size_t count = 0;
        count += value.binary_write(buf);
        buf += count;
        count += next.binary_write(buf);
        return count;
    }

    inline bool json(std::string* s) const
    {
        bool result = value.json(s);
        if (result)
        {
            s->append(",");
        }
        return next.json(s);
    }

    inline bool json_n(std::string* s, int n) const
    {
        bool result = value.json(s, n);
        if (result)
        {
            s->append(",");
        }
        return next.json_n(s, n + 1);
    }

    inline bool operator==(const Link<T, TNext>& other) const
    {
        return (value == other.value) && (next == other.next);
    }
};

template<typename T>
struct Link<T, Empty>
{
    T value;

    inline void hton()
    {
        value.hton();
    }

    inline void ntoh()
    {
        value.ntoh();
    }

    inline std::size_t binary_size() const
    {
        return value.binary_size();
    }

    inline std::size_t binary_read(std::uint8_t* data)
    {
        std::size_t count = 0;
        count += value.binary_read(data);
        data += count;
        return count;
    }

    inline std::size_t binary_write(std::uint8_t* buf) const
    {
        std::size_t count = 0;
        count += value.binary_write(buf);
        buf += count;
        return count;
    }

    inline bool json(std::string* s) const
    {
        return value.json(s);
    }

    inline bool json_n(std::string* s, int n) const
    {
        return value.json(s, n);
    }

    inline bool operator==(const Link<T, Empty>& other) const
    {
        return value == other.value;
    }
};

#define _UNDERSCORE(X) _##X
#define UNDERSCORE(X) _UNDERSCORE(X)
#define MEMBER_INITIALIZER(FQTV) TYPEOF(FQTV) UNDERSCORE(NAMEOF(FQTV)),
#define MEMBER_CONSTRUCTOR(FQTV) NAMEOF(FQTV)(UNDERSCORE(NAMEOF(FQTV))),
#define DEFAULT_MEMBER_CONSTRUCTOR(FQTV) NAMEOF(FQTV)(),
#define MEMBER(FQTV) TYPEOF(FQTV) NAMEOF(FQTV);
#define LINKED_MEMBER(FQTV, N) struct Link<NamedElement<TYPEOF(FQTV), STRINGIZE(NAMEOF(FQTV))>,
#define CLOSE_TEMPLATE(X) >

#define LINKED_DATA_STRUCTURE(...) \
    FOR_EACH_N(LINKED_MEMBER, __VA_ARGS__) \
    struct Empty \
    FOR_EACH(CLOSE_TEMPLATE, __VA_ARGS__)

#define LIST_THIS(...) auto list_this = ((LINKED_DATA_STRUCTURE(__VA_ARGS__)*)this);

#define __REFLECTION_STRUCT(STRUCT_NAME, ...) struct STRUCT_NAME { \
    FOR_EACH(MEMBER, __VA_ARGS__); \
    STRUCT_NAME() : REMOVE_TRAILING_COMMA(FOR_EACH(DEFAULT_MEMBER_CONSTRUCTOR, __VA_ARGS__)) {}; \
    STRUCT_NAME(REMOVE_TRAILING_COMMA(FOR_EACH(MEMBER_INITIALIZER, __VA_ARGS__))) : REMOVE_TRAILING_COMMA(FOR_EACH(MEMBER_CONSTRUCTOR, __VA_ARGS__)) {}; \
    inline LINKED_DATA_STRUCTURE(__VA_ARGS__)* as_link() const { return ((LINKED_DATA_STRUCTURE(__VA_ARGS__)*)this); } \
    std::size_t binary_size() const { LIST_THIS(__VA_ARGS__); return list_this->binary_size(); } \
    std::size_t binary_read(std::uint8_t* data) { LIST_THIS(__VA_ARGS__); return list_this->binary_read(data); } \
    std::uint8_t* binary_write() const { std::uint8_t* buf = new std::uint8_t[this->binary_size()]; this->binary_write(buf); return buf; } \
    std::size_t binary_write(std::uint8_t* buf) const { LIST_THIS(__VA_ARGS__); return list_this->binary_write(buf); } \
    std::size_t bson_read() { return 0; } \
    std::size_t bson_write() const { return 0; } \
    void json(std::string* s) const { LIST_THIS(__VA_ARGS__); s->append("{"); list_this->json(s); if (s->back() == ',') { s->pop_back(); } s->append("}"); } \
    std::string json() const { std::string s{}; json(&s); return s; } \
    std::string json_n() const { LIST_THIS(__VA_ARGS__); std::string s("{"); list_this->json_n(&s, 0); if (s.back() == ',') { s.pop_back(); } s.append("}"); return s; } \
    bool operator==(const STRUCT_NAME& other) const { LIST_THIS(__VA_ARGS__); return list_this->operator==(*((LINKED_DATA_STRUCTURE(__VA_ARGS__)*)(&other))); }; \
    friend std::ostream& operator<<(std::ostream& os, const STRUCT_NAME& v) { os << v.json(); return os; }\
}; \
template <> bool Element<STRUCT_NAME>::json(std::string* s) const { value.json(s); return true; } \
template <> void Element<STRUCT_NAME>::hton() { value.as_link()->hton(); } \
template <> void Element<STRUCT_NAME>::ntoh() { value.as_link()->ntoh(); }

#define REFLECTION_STRUCT(STRUCT_NAME, ...) __REFLECTION_STRUCT(STRUCT_NAME, __VA_ARGS__)

#endif

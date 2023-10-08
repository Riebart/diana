#include "macro_iterators.hpp"

#include <string>
#include <string.h> // Needed for strncpy
#include <sstream>
#include "byte_reordering.hpp"

template<size_t N>
struct StringLiteral
{
    constexpr StringLiteral(const char (&str) [N]) { std::copy_n(str, N, value); }
    char value[N];
};

template <typename T>
struct Element
{
    T value;

    Element() { value = T(); }
    Element(T value) { this->value = value; }
    void hton() { __hton<T>(value); }
    void ntoh() { __ntoh<T>(value); }

    std::size_t binary_size() { return sizeof(T); }
    std::size_t binary_read(std::uint8_t* data)
    {
        this->operator=(*(T*)data);
        this->ntoh();
        return sizeof(T);
    }
    std::size_t binary_write(std::uint8_t* buf)
    {
        auto dest = (Element<T>*)buf;
        dest->operator=(value);
        dest->hton();
        return sizeof(T);
    }

    void operator=(T newval) { this->value = newval; }
    operator T() const { return value; }

    bool json(std::string* s)
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

template<>
struct Element<const char*>
{
    const char* value = NULL;
    std::size_t num_chars = 0;

    Element() : value(NULL), num_chars(0) {}
    void hton() {}; void ntoh() {}

    std::size_t binary_size() { return num_chars + 1; }
    std::size_t binary_read(std::uint8_t* data)
    {
        this->operator=((const char*)data);
        return num_chars + 1;
    }
    std::size_t binary_write(std::uint8_t* buf)
    {
        memcpy(buf, value, num_chars + 1);
        return num_chars + 1;
    }

    void operator=(const char* newval) { this->value = newval; this->num_chars = strnlen(this->value, 4096); }
    operator const char*() const { return value; }

    bool json(std::string* s)
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
    
    std::size_t binary_size() { return 1 + (present ? Element<T>::binary_size() : 0); }
    std::size_t binary_read(std::uint8_t* data)
    {
        present = (bool)data[0];
        return 1 + (present ? Element<T>::binary_read(data + 1) : 0);
    }
    std::size_t binary_write(std::uint8_t* buf)
    {
        buf[0] = present;
        return 1 + (present ? Element<T>::binary_write(buf + 1) : 0);
    }
    
    void operator=(T newval) { this->present = true; Element<T>::operator = (newval); }
    bool json(std::string* s) { if (present) { Element<T>::json(s); } return present; }
    
    operator bool() const { return present; }
};

template <typename T, StringLiteral Name>
struct NamedElement : Element<T>
{
    NamedElement() : Element<T>() {}

    bool json(std::string* s)
    {
        s->append("\"");
        s->append(Name.value);
        s->append("\":");
        Element<T>::json(s);
        return true;
    }

    bool json(std::string* s, int N)
    {
        s->append("\"");
        s->append(Name.value);
        s->append("\":{\"index\":");
        s->append(std::to_string(N));
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

    bool json(std::string* s)
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

    bool json(std::string* s, int N)
    {
        s->append("\"");
        s->append(Name.value);
        s->append("\":{\"index\":");
        s->append(std::to_string(N));
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
template <typename T, int N, typename TNext>
struct Link
{
    T value;
    TNext next;

    std::size_t binary_size() { return value.binary_size() + next.binary_size(); }
    std::size_t binary_read(std::uint8_t* data)
    {
        std::size_t count = 0;
        count += value.binary_read(data);
        data += count;
        count += next.binary_read(data);
        return count;
    }
    std::size_t binary_write(std::uint8_t* buf)
    {
        std::size_t count = 0;
        count += value.binary_write(buf);
        buf += count;
        count += next.binary_write(buf);
        return count;
    }
    bool json(std::string* s) { bool result = next.json(s); if (result) { s->append(","); } return value.json(s); }
    bool json_n(std::string* s) { bool result = next.json_n(s); if (result) { s->append(","); } return value.json(s, N); }
};

template<typename T, int N>
struct Link<T, N, Empty>
{
    T value;

    std::size_t binary_size() { return value.binary_size(); }
    std::size_t binary_read(std::uint8_t* data)
    {
        std::size_t count = 0;
        count += value.binary_read(data);
        data += count;
        return count;
    }
    std::size_t binary_write(std::uint8_t* buf)
    {
        std::size_t count = 0;
        count += value.binary_write(buf);
        buf += count;
        return count;
    }
    bool json(std::string* s) { return value.json(s); }
    bool json_n(std::string* s) { return value.json(s, N); }
};

#define MEMBER(FQTV) TYPEOF(FQTV) NAMEOF(FQTV);
#define LINKED_MEMBER(FQTV, N) struct Link<NamedElement<TYPEOF(FQTV), STRINGIZE(NAMEOF(FQTV))>, N,
#define CLOSE_TEMPLATE(X) >

#define LINKED_DATA_STRUCTURE(...) \
    FOR_EACH_N(LINKED_MEMBER, __VA_ARGS__) \
    struct Empty \
    FOR_EACH(CLOSE_TEMPLATE, __VA_ARGS__) 

#define LIST_THIS(...) auto list_this = ((LINKED_DATA_STRUCTURE(__VA_ARGS__)*)this);

#define __REFLECTION_STRUCT(STRUCT_NAME, ...) struct STRUCT_NAME { \
    FOR_EACH(MEMBER, __VA_ARGS__); \
    std::size_t binary_size() { LIST_THIS(__VA_ARGS__); return list_this->binary_size(); } \
    std::size_t binary_read(std::uint8_t* data) { LIST_THIS(__VA_ARGS__); return list_this->binary_read(data); } \
    std::uint8_t* binary_write() { std::uint8_t* buf = new std::uint8_t[this->binary_size()]; this->binary_write(buf); return buf; } \
    std::size_t binary_write(std::uint8_t* buf) { LIST_THIS(__VA_ARGS__); return list_this->binary_write(buf); } \
    std::size_t bson_read() { return 0; } \
    std::size_t bson_write() { return 0; } \
    std::string json() { LIST_THIS(__VA_ARGS__); std::string s("{"); list_this->json(&s); if (s.back() == ',') { s.pop_back(); } s.append("}"); return s; } \
    std::string json_n() { LIST_THIS(__VA_ARGS__); std::string s("{"); list_this->json_n(&s); if (s.back() == ',') { s.pop_back(); } s.append("}"); return s; } \
};

// Because we do tail-up recursion for emitting the JSON, the fields the serialization appear
// in the wrong (reversed) order that they were specified in. This just reverses the order of
// the arguments so that things come out in the order specified.
#define REFLECTION_STRUCT(STRUCT_NAME, ...) __REFLECTION_STRUCT(STRUCT_NAME, REVERSE(__VA_ARGS__))

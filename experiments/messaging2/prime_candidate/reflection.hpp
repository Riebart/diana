#include "macro_iterators.hpp"

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
    bool read(std::uint8_t* data) { value = *(T*)data; return data + sizeof(T); }
    void hton() { __hton<T>(value); }
    std::size_t dump_size() { return sizeof(T); }
    void operator=(T newval) { this->value = newval; }
    operator T() const { return value; }

    bool json(std::string* s)
    {
        std::ostringstream os;
        os << value;
        s->append(os.str());
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

    operator const char*() const { return value; }
};

template <typename T>
struct Optional : Element<T>
{
    bool present;

    Optional() : present(false), Element<T>() {}

    bool read(std::uint8_t* data) { return (present ? Element<T>::read(data) : data); }
    std::size_t dump_size() { return 1 + (present ? Element<T>::dump_size() : 0); }

    void operator=(T newval)
    {
        this->present = true;
        Element<T>::operator = (newval);
    }

    bool json(std::string* s)
    {
        if (present)
        {
            Element<T>::json(s);
        }
        return present;
    }
    
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
};

//@TODO Technically, this has a size when included, but not when inherited
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

    std::size_t dump_size()
    {
        return value.dump_size() + next.dump_size();
    }

    bool json(std::string* s)
    {
        bool result = next.json(s);
        if (result)
        {
            s->append(",");
        }
        return value.json(s);
    }
};

template<typename T>
struct Link<T, Empty>
{
    T value;

    std::size_t dump_size()
    {
        return value.dump_size();
    }

    bool json(std::string* s)
    {
        return value.json(s);
    }
};

#define MEMBER(FQTV) TYPEOF(FQTV) NAMEOF(FQTV);
#define LINKED_MEMBER(FQTV) struct Link<NamedElement<TYPEOF(FQTV), STRINGIZE(NAMEOF(FQTV))>,
#define CLOSE_TEMPLATE(X) >

#define LINKED_DATA_STRUCTURE(...) \
    FOR_EACH(LINKED_MEMBER, __VA_ARGS__) \
    struct Empty \
    FOR_EACH(CLOSE_TEMPLATE, __VA_ARGS__) 

#define REFLECTION_STRUCT(STRUCT_NAME, ...) struct STRUCT_NAME { \
    FOR_EACH(MEMBER, __VA_ARGS__); \
    std::size_t dump_size() { return ((LINKED_DATA_STRUCTURE(__VA_ARGS__)*)this)->dump_size(); }\
    std::string json() { std::string s("{"); ((LINKED_DATA_STRUCTURE(__VA_ARGS__)*)this)->json(&s); s.append("}"); return s; }\
};

#include <iostream>


// We can use this one to strip parens that are contained in a token's value by using this function
// without parens:
//
// Example:
//  #define A (int)
//  #define A_WITHOUT_PARENS REM A // Resovles to: REM (int) == REM(int) == int
#define REM(...) __VA_ARGS__

// Just eat an entire set of __VA_ARGS__, useful for taildropping in recursion and for removing a paren-enclosed prefix
// 
// Example:
//  #define A (int) a
//  #define A_WITHOUT_PAREN_PREFIX EAT x // Resolves to: EAT (int) a == EAT(int) a == a
#define EAT(...)

// Strip off the type
#define NAMEOF(x) EAT x
// Show the type without parenthesis
#define VAR(x) REM x

// Retrieve the type
//   Use the parenthesized type to unvoke the separation macro token as a function
//   to split the type and variable name with a comma
#define TYPEOF(x) __TYPEOF_STRIP_PARENS(__TYPEOF_SEPARATE x,)
//   Paren and suffix with a comma args
#define __TYPEOF_SEPARATE(...) (__VA_ARGS__),
#define __TYPEOF_STRIP_PARENS(...) __TYPEOF_STRIP_NAME(__VA_ARGS__)
#define __TYPEOF_STRIP_NAME(x, ...) REM x

// The magic of double-indirection is required here to get the compiler to actually resolve
// this macro function, otherwise it leaves it as a macro call and not the resolved stringized
// token.
#define __STRINGIZE(X) #X
#define STRINGIZE(X) __STRINGIZE(X)

// Useful as a carrier for double-indirection when using concatenation to construct a macro
// function and arguments dynamically. Some compilers will not automatically resolve the
// dynamically concatenated macro function and args once constructed unless they are used.
//
// Example:
//  EXPAND(REVERSE_ ## N(__VA_ARGS__))
#define EXPAND(x) x

// Base cases for the iterator n-ary functions
#define REVERSE_0(...)
#define REVERSE_1(a, ...) a
// #define __REVERSE(N,...) EXPAND(REVERSE_ ## N(__VA_ARGS__))
// #define REVERSE_N(N, ...) EXPAND(__REVERSE(N, __VA_ARGS__))

#define FOR_EACH_REV_N_0(LAMBDA)
#define FOR_EACH_REV_N_1(LAMBDA, X) LAMBDA(X, 0)

#define FOR_EACH_0(LAMBDA)
#define FOR_EACH_1(LAMBDA, X) LAMBDA(X)

#define FOR_EACH_N_0(LAMBDA)
#define FOR_EACH_N_1(LAMBDA, X) LAMBDA(X, 0)

/*
(
    echo "#define ITERATOR_START_1UP(`seq 0 20 | sed 's/^/_/' | paste -sd ','`,NAME,...) NAME"

    echo "#define FOR_EACH(LAMBDA,...) ITERATOR_START_1UP(_0,__VA_ARGS__,`seq 20 -1 0 | sed 's/^/FOR_EACH_/' | paste -sd ','`) (LAMBDA,__VA_ARGS__)"
    echo "#define FOR_EACH_N(LAMBDA,...) ITERATOR_START_1UP(_0,__VA_ARGS__,`seq 20 -1 0 | sed 's/^/FOR_EACH_N_/' | paste -sd ','`) (LAMBDA,__VA_ARGS__)"
    echo "#define FOR_EACH_REV_N(LAMBDA,...) ITERATOR_START_1UP(_0,__VA_ARGS__,`seq 20 -1 0 | sed 's/^/FOR_EACH_REV_N_/' | paste -sd ','`) (LAMBDA,__VA_ARGS__)"
    echo "#define REVERSE(...) ITERATOR_START_1UP(_0,__VA_ARGS__,`seq 20 -1 0 | sed 's/^/REVERSE_/' | paste -sd ','`) (__VA_ARGS__)"

    for i in {2..20}; do echo "#define FOR_EACH_${i}(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_$[i-1](LAMBDA, __VA_ARGS__)"; done
    for i in {2..20}; do echo "#define FOR_EACH_N_${i}(LAMBDA, X, ...) LAMBDA(X, $[i-1])FOR_EACH_N_$[i-1](LAMBDA, __VA_ARGS__)"; done
    for i in {2..20}; do echo "#define FOR_EACH_REV_N_${i}(LAMBDA, X, ...) FOR_EACH_REV_N_$[i-1](LAMBDA, __VA_ARGS__)LAMBDA(X, $[i-1])"; done
    for i in {2..20}; do echo "#define REVERSE_${i}(a,...) EXPAND(REVERSE_$[i-1](__VA_ARGS__)),a"; done
) | clip.exe
*/

#define ITERATOR_START_1UP(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,NAME,...) NAME
#define FOR_EACH(LAMBDA,...) ITERATOR_START_1UP(_0,__VA_ARGS__,FOR_EACH_20,FOR_EACH_19,FOR_EACH_18,FOR_EACH_17,FOR_EACH_16,FOR_EACH_15,FOR_EACH_14,FOR_EACH_13,FOR_EACH_12,FOR_EACH_11,FOR_EACH_10,FOR_EACH_9,FOR_EACH_8,FOR_EACH_7,FOR_EACH_6,FOR_EACH_5,FOR_EACH_4,FOR_EACH_3,FOR_EACH_2,FOR_EACH_1,FOR_EACH_0) (LAMBDA,__VA_ARGS__)
#define FOR_EACH_N(LAMBDA,...) ITERATOR_START_1UP(_0,__VA_ARGS__,FOR_EACH_N_20,FOR_EACH_N_19,FOR_EACH_N_18,FOR_EACH_N_17,FOR_EACH_N_16,FOR_EACH_N_15,FOR_EACH_N_14,FOR_EACH_N_13,FOR_EACH_N_12,FOR_EACH_N_11,FOR_EACH_N_10,FOR_EACH_N_9,FOR_EACH_N_8,FOR_EACH_N_7,FOR_EACH_N_6,FOR_EACH_N_5,FOR_EACH_N_4,FOR_EACH_N_3,FOR_EACH_N_2,FOR_EACH_N_1,FOR_EACH_N_0) (LAMBDA,__VA_ARGS__)
#define FOR_EACH_REV_N(LAMBDA,...) ITERATOR_START_1UP(_0,__VA_ARGS__,FOR_EACH_REV_N_20,FOR_EACH_REV_N_19,FOR_EACH_REV_N_18,FOR_EACH_REV_N_17,FOR_EACH_REV_N_16,FOR_EACH_REV_N_15,FOR_EACH_REV_N_14,FOR_EACH_REV_N_13,FOR_EACH_REV_N_12,FOR_EACH_REV_N_11,FOR_EACH_REV_N_10,FOR_EACH_REV_N_9,FOR_EACH_REV_N_8,FOR_EACH_REV_N_7,FOR_EACH_REV_N_6,FOR_EACH_REV_N_5,FOR_EACH_REV_N_4,FOR_EACH_REV_N_3,FOR_EACH_REV_N_2,FOR_EACH_REV_N_1,FOR_EACH_REV_N_0) (LAMBDA,__VA_ARGS__)
#define REVERSE(...) ITERATOR_START_1UP(_0,__VA_ARGS__,REVERSE_20,REVERSE_19,REVERSE_18,REVERSE_17,REVERSE_16,REVERSE_15,REVERSE_14,REVERSE_13,REVERSE_12,REVERSE_11,REVERSE_10,REVERSE_9,REVERSE_8,REVERSE_7,REVERSE_6,REVERSE_5,REVERSE_4,REVERSE_3,REVERSE_2,REVERSE_1,REVERSE_0) (__VA_ARGS__)
#define FOR_EACH_2(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_1(LAMBDA, __VA_ARGS__)
#define FOR_EACH_3(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_2(LAMBDA, __VA_ARGS__)
#define FOR_EACH_4(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_3(LAMBDA, __VA_ARGS__)
#define FOR_EACH_5(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_4(LAMBDA, __VA_ARGS__)
#define FOR_EACH_6(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_5(LAMBDA, __VA_ARGS__)
#define FOR_EACH_7(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_6(LAMBDA, __VA_ARGS__)
#define FOR_EACH_8(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_7(LAMBDA, __VA_ARGS__)
#define FOR_EACH_9(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_8(LAMBDA, __VA_ARGS__)
#define FOR_EACH_10(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_9(LAMBDA, __VA_ARGS__)
#define FOR_EACH_11(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_10(LAMBDA, __VA_ARGS__)
#define FOR_EACH_12(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_11(LAMBDA, __VA_ARGS__)
#define FOR_EACH_13(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_12(LAMBDA, __VA_ARGS__)
#define FOR_EACH_14(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_13(LAMBDA, __VA_ARGS__)
#define FOR_EACH_15(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_14(LAMBDA, __VA_ARGS__)
#define FOR_EACH_16(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_15(LAMBDA, __VA_ARGS__)
#define FOR_EACH_17(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_16(LAMBDA, __VA_ARGS__)
#define FOR_EACH_18(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_17(LAMBDA, __VA_ARGS__)
#define FOR_EACH_19(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_18(LAMBDA, __VA_ARGS__)
#define FOR_EACH_20(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_19(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_2(LAMBDA, X, ...) LAMBDA(X, 1)FOR_EACH_N_1(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_3(LAMBDA, X, ...) LAMBDA(X, 2)FOR_EACH_N_2(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_4(LAMBDA, X, ...) LAMBDA(X, 3)FOR_EACH_N_3(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_5(LAMBDA, X, ...) LAMBDA(X, 4)FOR_EACH_N_4(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_6(LAMBDA, X, ...) LAMBDA(X, 5)FOR_EACH_N_5(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_7(LAMBDA, X, ...) LAMBDA(X, 6)FOR_EACH_N_6(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_8(LAMBDA, X, ...) LAMBDA(X, 7)FOR_EACH_N_7(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_9(LAMBDA, X, ...) LAMBDA(X, 8)FOR_EACH_N_8(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_10(LAMBDA, X, ...) LAMBDA(X, 9)FOR_EACH_N_9(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_11(LAMBDA, X, ...) LAMBDA(X, 10)FOR_EACH_N_10(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_12(LAMBDA, X, ...) LAMBDA(X, 11)FOR_EACH_N_11(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_13(LAMBDA, X, ...) LAMBDA(X, 12)FOR_EACH_N_12(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_14(LAMBDA, X, ...) LAMBDA(X, 13)FOR_EACH_N_13(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_15(LAMBDA, X, ...) LAMBDA(X, 14)FOR_EACH_N_14(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_16(LAMBDA, X, ...) LAMBDA(X, 15)FOR_EACH_N_15(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_17(LAMBDA, X, ...) LAMBDA(X, 16)FOR_EACH_N_16(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_18(LAMBDA, X, ...) LAMBDA(X, 17)FOR_EACH_N_17(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_19(LAMBDA, X, ...) LAMBDA(X, 18)FOR_EACH_N_18(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_20(LAMBDA, X, ...) LAMBDA(X, 19)FOR_EACH_N_19(LAMBDA, __VA_ARGS__)
#define FOR_EACH_REV_N_2(LAMBDA, X, ...) FOR_EACH_REV_N_1(LAMBDA, __VA_ARGS__)LAMBDA(X, 1)
#define FOR_EACH_REV_N_3(LAMBDA, X, ...) FOR_EACH_REV_N_2(LAMBDA, __VA_ARGS__)LAMBDA(X, 2)
#define FOR_EACH_REV_N_4(LAMBDA, X, ...) FOR_EACH_REV_N_3(LAMBDA, __VA_ARGS__)LAMBDA(X, 3)
#define FOR_EACH_REV_N_5(LAMBDA, X, ...) FOR_EACH_REV_N_4(LAMBDA, __VA_ARGS__)LAMBDA(X, 4)
#define FOR_EACH_REV_N_6(LAMBDA, X, ...) FOR_EACH_REV_N_5(LAMBDA, __VA_ARGS__)LAMBDA(X, 5)
#define FOR_EACH_REV_N_7(LAMBDA, X, ...) FOR_EACH_REV_N_6(LAMBDA, __VA_ARGS__)LAMBDA(X, 6)
#define FOR_EACH_REV_N_8(LAMBDA, X, ...) FOR_EACH_REV_N_7(LAMBDA, __VA_ARGS__)LAMBDA(X, 7)
#define FOR_EACH_REV_N_9(LAMBDA, X, ...) FOR_EACH_REV_N_8(LAMBDA, __VA_ARGS__)LAMBDA(X, 8)
#define FOR_EACH_REV_N_10(LAMBDA, X, ...) FOR_EACH_REV_N_9(LAMBDA, __VA_ARGS__)LAMBDA(X, 9)
#define FOR_EACH_REV_N_11(LAMBDA, X, ...) FOR_EACH_REV_N_10(LAMBDA, __VA_ARGS__)LAMBDA(X, 10)
#define FOR_EACH_REV_N_12(LAMBDA, X, ...) FOR_EACH_REV_N_11(LAMBDA, __VA_ARGS__)LAMBDA(X, 11)
#define FOR_EACH_REV_N_13(LAMBDA, X, ...) FOR_EACH_REV_N_12(LAMBDA, __VA_ARGS__)LAMBDA(X, 12)
#define FOR_EACH_REV_N_14(LAMBDA, X, ...) FOR_EACH_REV_N_13(LAMBDA, __VA_ARGS__)LAMBDA(X, 13)
#define FOR_EACH_REV_N_15(LAMBDA, X, ...) FOR_EACH_REV_N_14(LAMBDA, __VA_ARGS__)LAMBDA(X, 14)
#define FOR_EACH_REV_N_16(LAMBDA, X, ...) FOR_EACH_REV_N_15(LAMBDA, __VA_ARGS__)LAMBDA(X, 15)
#define FOR_EACH_REV_N_17(LAMBDA, X, ...) FOR_EACH_REV_N_16(LAMBDA, __VA_ARGS__)LAMBDA(X, 16)
#define FOR_EACH_REV_N_18(LAMBDA, X, ...) FOR_EACH_REV_N_17(LAMBDA, __VA_ARGS__)LAMBDA(X, 17)
#define FOR_EACH_REV_N_19(LAMBDA, X, ...) FOR_EACH_REV_N_18(LAMBDA, __VA_ARGS__)LAMBDA(X, 18)
#define FOR_EACH_REV_N_20(LAMBDA, X, ...) FOR_EACH_REV_N_19(LAMBDA, __VA_ARGS__)LAMBDA(X, 19)
#define REVERSE_2(a,...) EXPAND(REVERSE_1(__VA_ARGS__)),a
#define REVERSE_3(a,...) EXPAND(REVERSE_2(__VA_ARGS__)),a
#define REVERSE_4(a,...) EXPAND(REVERSE_3(__VA_ARGS__)),a
#define REVERSE_5(a,...) EXPAND(REVERSE_4(__VA_ARGS__)),a
#define REVERSE_6(a,...) EXPAND(REVERSE_5(__VA_ARGS__)),a
#define REVERSE_7(a,...) EXPAND(REVERSE_6(__VA_ARGS__)),a
#define REVERSE_8(a,...) EXPAND(REVERSE_7(__VA_ARGS__)),a
#define REVERSE_9(a,...) EXPAND(REVERSE_8(__VA_ARGS__)),a
#define REVERSE_10(a,...) EXPAND(REVERSE_9(__VA_ARGS__)),a
#define REVERSE_11(a,...) EXPAND(REVERSE_10(__VA_ARGS__)),a
#define REVERSE_12(a,...) EXPAND(REVERSE_11(__VA_ARGS__)),a
#define REVERSE_13(a,...) EXPAND(REVERSE_12(__VA_ARGS__)),a
#define REVERSE_14(a,...) EXPAND(REVERSE_13(__VA_ARGS__)),a
#define REVERSE_15(a,...) EXPAND(REVERSE_14(__VA_ARGS__)),a
#define REVERSE_16(a,...) EXPAND(REVERSE_15(__VA_ARGS__)),a
#define REVERSE_17(a,...) EXPAND(REVERSE_16(__VA_ARGS__)),a
#define REVERSE_18(a,...) EXPAND(REVERSE_17(__VA_ARGS__)),a
#define REVERSE_19(a,...) EXPAND(REVERSE_18(__VA_ARGS__)),a
#define REVERSE_20(a,...) EXPAND(REVERSE_19(__VA_ARGS__)),a

#define STRING_WITH_SEP(X) STRINGIZE(X),
#define STRING_WITH_SEP_N(X,N) (N,STRINGIZE(X)),

#ifdef TEST_GENERATIVE_MACROS
REVERSE(1)
REVERSE(1,2,3,4,5,6,7,8,9,10)
REVERSE(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20)

FOR_EACH(STRING_WITH_SEP,1)
FOR_EACH(STRING_WITH_SEP,1,2,3,4,5,6,7,8,9,10)
FOR_EACH(STRING_WITH_SEP,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20)

FOR_EACH_N(STRING_WITH_SEP_N,1)
FOR_EACH_N(STRING_WITH_SEP_N,1,2,3,4,5,6,7,8,9,10)
FOR_EACH_N(STRING_WITH_SEP_N,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20)

FOR_EACH_REV_N(STRING_WITH_SEP_N,1)
FOR_EACH_REV_N(STRING_WITH_SEP_N,1,2,3,4,5,6,7,8,9,10)
FOR_EACH_REV_N(STRING_WITH_SEP_N,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20)

FOR_EACH_REV_N(STRING_WITH_SEP_N,REVERSE(1))
FOR_EACH_REV_N(STRING_WITH_SEP_N,REVERSE(1,2,3,4,5,6,7,8,9,10))
FOR_EACH_REV_N(STRING_WITH_SEP_N,REVERSE(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20))
#endif

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

#define ACCESSOR3(FQTV, N) TYPEOF(FQTV) & NAMEOF(FQTV) () { return JOIN(msg., REPEAT_ ## N (next.))value ; };
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

#define REFLECTION_STRUCT2(STRUCT_NAME, ...) struct STRUCT_NAME { \
    FOR_EACH(MEMBER, __VA_ARGS__); \
    LINKED_DATA_STRUCTURE(__VA_ARGS__) msg; \
    std::size_t dump_size() { return msg.dump_size(); }\
};

REFLECTION_STRUCT(ReflectionStruct,
             (std::int64_t) a,
             (Optional<double>) b,
             (Optional<double>) c,
             (Optional<float>) d
            );

REFLECTION_STRUCT2(ReflectionStruct2,
             (std::int64_t) a,
             (Optional<double>) b,
             (Optional<double>) c,
             (Optional<float>) d
            );

#include <iostream>
#include <cstdint>
#include <cfloat>

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

REFLECTION_STRUCT(PhysicalPropertiesMsg,
    (std::int64_t) server_id,
    (std::int64_t) client_id,
    (Optional<const char*>) object_type,
    (Optional<double>) mass,
    (Optional<double>) radius,
    (Optional<Vector3<double>>) position,
    (Optional<Vector3<double>>) velocity,
    (Optional<Vector3<double>>) thrust,
    (Optional<Vector4<double>>) orientation
)

int main(int argc, char** argv)
{
    ReflectionStruct rs{};

    ReflectionStruct2 rs2;
    std::cout << sizeof(rs) << " " << sizeof(rs.a) << " " << sizeof(rs.b) << " " << sizeof(rs.c) << " " << sizeof(rs.d) << std::endl;
    std::cout << sizeof(rs2) << " " << sizeof(rs2.msg) << " " << sizeof(rs.a) << " " << sizeof(rs.b) << " " << sizeof(rs.c) << " " << sizeof(rs.d) << std::endl;
    std::cout << sizeof(NamedElement<std::int64_t, "ABCDE">) << std::endl;
    std::cout << sizeof(NamedElement<float, "ABCDE">) << std::endl;
    std::cout << sizeof(NamedElement<Optional<char>, "ABCDE">) << std::endl;
    std::cout << sizeof(NamedElement<Optional<float>, "ABCDE">) << std::endl;
    std::cout << sizeof(NamedElement<Optional<double>, "ABCDE">) << std::endl;
    std::cout << sizeof(Link<NamedElement<Optional<double>, "ABCDE">, Empty>) << std::endl;
    std::cout << "DS " << rs.dump_size() << std::endl << rs.json() << std::endl;
    rs.b = 0.0;
    std::cout << "DS " << rs.dump_size() << std::endl << rs.json() << std::endl;
    rs.c = 0.0;
    std::cout << "DS " << rs.dump_size() << std::endl << rs.json() << std::endl;
    rs.d = 0.0f;
    std::cout << "DS " << rs.dump_size() << std::endl << rs.json() << std::endl;

    std::cout << "Physical Properties message testing..." << std::endl;
    PhysicalPropertiesMsg msg{};
    std::cout << sizeof(msg) << std::endl;
    std::cout << msg.dump_size() << " " << msg.json() << std::endl;
    msg.radius = 0.0;
    std::cout << msg.dump_size() << " " << msg.json() << std::endl;
    msg.mass = 0.0;
    std::cout << msg.dump_size() << " " << msg.json() << std::endl;
    msg.object_type = "This is some stuff!"; // strnlen() = 19 + null
    std::cout << msg.dump_size() << " " << msg.json() << std::endl;
    return 0;
}
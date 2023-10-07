#define EXPAND(x) x

#define REVERSE_1(a) a
#define REVERSE_2(a,b) b,a
#define REVERSE_3(a,...) EXPAND(REVERSE_2(__VA_ARGS__)),a
#define REVERSE_4(a,...) EXPAND(REVERSE_3(__VA_ARGS__)),a
#define REVERSE_5(a,...) EXPAND(REVERSE_4(__VA_ARGS__)),a
#define REVERSE_6(a,...) EXPAND(REVERSE_5(__VA_ARGS__)),a
#define REVERSE_7(a,...) EXPAND(REVERSE_6(__VA_ARGS__)),a
#define REVERSE_8(a,...) EXPAND(REVERSE_7(__VA_ARGS__)),a
#define REVERSE_9(a,...) EXPAND(REVERSE_8(__VA_ARGS__)),a
#define REVERSE_10(a,...) EXPAND(REVERSE_9(__VA_ARGS__)),a
#define __REVERSE(N,...) EXPAND(REVERSE_ ## N(__VA_ARGS__))
#define REVERSE_N(N, ...) EXPAND(__REVERSE(N, __VA_ARGS__))

#define REVERSE_START(_1, _2, _3, _4, _5, _6, _7, _9, _10, N, ...) N
#define REVERSE(...) EXPAND(REVERSE_START(__VA_ARGS__,REVERSE_9,REVERSE_8,REVERSE_7,REVERSE_6,REVERSE_5,REVERSE_4,REVERSE_3,REVERSE_2,REVERSE_1,REVERSE_0)(__VA_ARGS__))

#define FOR_EACH_0(LAMBDA)
#define FOR_EACH_1(LAMBDA, X) LAMBDA(X)
#define FOR_EACH_2(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_1(LAMBDA, __VA_ARGS__)
#define FOR_EACH_3(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_2(LAMBDA, __VA_ARGS__)
#define FOR_EACH_4(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_3(LAMBDA, __VA_ARGS__)
#define FOR_EACH_5(LAMBDA, X, ...) LAMBDA(X)FOR_EACH_4(LAMBDA, __VA_ARGS__)

#define FOR_EACH_N_0(LAMBDA)
#define FOR_EACH_N_1(LAMBDA, X) LAMBDA(X, 0)
#define FOR_EACH_N_2(LAMBDA, X, ...) LAMBDA(X, 1)FOR_EACH_N_1(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_3(LAMBDA, X, ...) LAMBDA(X, 2)FOR_EACH_N_2(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_4(LAMBDA, X, ...) LAMBDA(X, 3)FOR_EACH_N_3(LAMBDA, __VA_ARGS__)
#define FOR_EACH_N_5(LAMBDA, X, ...) LAMBDA(X, 4)FOR_EACH_N_4(LAMBDA, __VA_ARGS__)

#define FOR_EACH_REV_N_0(LAMBDA)
#define FOR_EACH_REV_N_1(LAMBDA, X) LAMBDA(X, 0)
#define FOR_EACH_REV_N_2(LAMBDA, X, ...) FOR_EACH_REV_N_1(LAMBDA, __VA_ARGS__)LAMBDA(X, 1)
#define FOR_EACH_REV_N_3(LAMBDA, X, ...) FOR_EACH_REV_N_2(LAMBDA, __VA_ARGS__)LAMBDA(X, 2)
#define FOR_EACH_REV_N_4(LAMBDA, X, ...) FOR_EACH_REV_N_3(LAMBDA, __VA_ARGS__)LAMBDA(X, 3)
#define FOR_EACH_REV_N_5(LAMBDA, X, ...) FOR_EACH_REV_N_4(LAMBDA, __VA_ARGS__)LAMBDA(X, 4)

#define FOR_EACH_2UP_0(LAMBDA)
#define FOR_EACH_2UP_1(LAMBDA, X, Y) LAMBDA(X, Y)
#define FOR_EACH_2UP_2(LAMBDA, X, Y, ...) LAMBDA(X, Y)FOR_EACH_2UP_1(LAMBDA, __VA_ARGS__)
#define FOR_EACH_2UP_3(LAMBDA, X, Y, ...) LAMBDA(X, Y)FOR_EACH_2UP_2(LAMBDA, __VA_ARGS__)
#define FOR_EACH_2UP_4(LAMBDA, X, Y, ...) LAMBDA(X, Y)FOR_EACH_2UP_3(LAMBDA, __VA_ARGS__)
#define FOR_EACH_2UP_5(LAMBDA, X, Y, ...) LAMBDA(X, Y)FOR_EACH_2UP_4(LAMBDA, __VA_ARGS__)

#define FOR_EACH_3UP_0(LAMBDA)
#define FOR_EACH_3UP_1(LAMBDA, X, Y, Z) LAMBDA(X, Y, Z)
#define FOR_EACH_3UP_2(LAMBDA, X, Y, Z, ...) LAMBDA(X, Y, Z)FOR_EACH_3UP_1(LAMBDA, __VA_ARGS__)
#define FOR_EACH_3UP_3(LAMBDA, X, Y, Z, ...) LAMBDA(X, Y, Z)FOR_EACH_3UP_2(LAMBDA, __VA_ARGS__)
#define FOR_EACH_3UP_4(LAMBDA, X, Y, Z, ...) LAMBDA(X, Y, Z)FOR_EACH_3UP_3(LAMBDA, __VA_ARGS__)
#define FOR_EACH_3UP_5(LAMBDA, X, Y, Z, ...) LAMBDA(X, Y, Z)FOR_EACH_3UP_4(LAMBDA, __VA_ARGS__)

#define FOR_EACH_START1(_0,_1,_2,_3,_4,_5,NAME,...) NAME
#define FOR_EACH(LAMBDA,...) \
  FOR_EACH_START1(_0,__VA_ARGS__,\
    FOR_EACH_5,FOR_EACH_4,FOR_EACH_3,\
    FOR_EACH_2,FOR_EACH_1,FOR_EACH_0) (LAMBDA,__VA_ARGS__)

#define FOR_EACH_N(LAMBDA,...) \
  FOR_EACH_START1(_0,__VA_ARGS__,\
    FOR_EACH_N_5,FOR_EACH_N_4,FOR_EACH_N_3,\
    FOR_EACH_N_2,FOR_EACH_N_1,FOR_EACH_N_0) (LAMBDA,__VA_ARGS__)

#define FOR_EACH_REV_N(LAMBDA,...) \
  FOR_EACH_START1(_0,__VA_ARGS__,\
    FOR_EACH_REV_N_5,FOR_EACH_REV_N_4,FOR_EACH_REV_N_3,\
    FOR_EACH_REV_N_2,FOR_EACH_REV_N_1,FOR_EACH_REV_N_0) (LAMBDA,__VA_ARGS__)

#define FOR_EACH_START2(_0,__0,_1,__1,_2,__2,_3,__3,_4,__4,_5,__5,NAME,...) NAME
#define FOR_EACH_2UP(LAMBDA, ...) \
  FOR_EACH_START2(_0,__0,__VA_ARGS__,\
    FOR_EACH_2UP_5,FOR_EACH_2UP_5,FOR_EACH_2UP_4,FOR_EACH_2UP_4,FOR_EACH_2UP_3,FOR_EACH_2UP_3,\
    FOR_EACH_2UP_2,FOR_EACH_2UP_2,FOR_EACH_2UP_1,FOR_EACH_2UP_1,FOR_EACH_2UP_0,FOR_EACH_2UP_0) (LAMBDA,__VA_ARGS__)

#define FOR_EACH_START3(_0,__0,___0,_1,__1,___1,_2,__2,___2,_3,__3,___3,_4,__4,___4,_5,__5,___5,NAME,...) NAME
#define FOR_EACH_3UP(LAMBDA, ...) \
  FOR_EACH_START3(_0,__VA_ARGS__,\
    FOR_EACH_3UP_5,FOR_EACH_3UP_4,FOR_EACH_3UP_3,\
    FOR_EACH_3UP_2,FOR_EACH_3UP_1,FOR_EACH_3UP_0) (LAMBDA,__VA_ARGS__)

// Example
// Some actions
#define QUALIFIER(X) X::
#define OPEN_NS(X)   namespace X {
#define CLOSE_NS(X)  }
// Helper function
#define QUALIFIED(NAME,...) FOR_EACH(QUALIFIER,__VA_ARGS__)NAME

// Emit some code
QUALIFIED(MyFoo,Outer,Next,Inner)  foo();

FOR_EACH(OPEN_NS,Outer,Next,Inner)
class Foo;
FOR_EACH(CLOSE_NS,Outer,Next,Inner)


#define ACCESSOR(TYPE, NAME) TYPE & NAME;
#define MEMBER_S(TYPE, NAME) struct NamedElementC<TYPE, #NAME,
#define CLOSE_TEMPLATE2(X, Y) >

#define NAMEDSTRUCT1(STRUCT_NAME, ...) struct STRUCT_NAME {\
    FOR_EACH_2UP(ACCESSOR, __VA_ARGS__);\
    FOR_EACH_2UP(MEMBER_S, __VA_ARGS__)\
    struct Empty \
    FOR_EACH_2UP(CLOSE_TEMPLATE2, __VA_ARGS__);\
};

NAMEDSTRUCT1(NestyNesterson,
             int, a,
             char, b,
             double, c,
             float, d
            )

#define REM(...) __VA_ARGS__
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

#define _STRINGIZE(X) #X
#define STRINGIZE(X) _STRINGIZE(X)
#define ACCESSOR2(FQTV) TYPEOF(FQTV) & NAMEOF(FQTV) () { return 0; };
#define MEMBER2(FQTV) struct Link<TYPEOF(FQTV), STRINGIZE(NAMEOF(FQTV)),
#define CLOSE_TEMPLATE1(X) >

#define NAMEDSTRUCT2(STRUCT_NAME, ...) struct STRUCT_NAME { \
    FOR_EACH(ACCESSOR2, __VA_ARGS__); \
    FOR_EACH(MEMBER2, __VA_ARGS__) \
    struct Empty \
    FOR_EACH(CLOSE_TEMPLATE1, __VA_ARGS__);\
};

NAMEDSTRUCT2(Nester,
             (int) a,
             (char) b,
             (double) c,
             (float) d
            )

#define ___JOIN(X, Y) X Y // Needed to ensure that strings concatenate without spaces
#define JOIN(X, Y) ___JOIN(X,Y) // Needed to ensure that strings concatenate without spaces
#define REPEAT_0(X)
#define REPEAT_1(X) X
#define REPEAT_2(X) JOIN(X,REPEAT_1(X))
#define REPEAT_3(X) JOIN(X,REPEAT_2(X))
#define REPEAT_4(X) JOIN(X,REPEAT_3(X))
#define REPEAT_5(X) JOIN(X,REPEAT_4(X))

#define ___NEXT(X) .next
#define ACCESSOR3(FQTV, N) TYPEOF(FQTV) & NAMEOF(FQTV) () { return JOIN(msg., REPEAT_ ## N (next.))value ; };
#define NAMEDSTRUCT3(STRUCT_NAME, ...) struct STRUCT_NAME { \
    FOR_EACH_REV_N(ACCESSOR3, REVERSE(__VA_ARGS__)); \
    FOR_EACH(MEMBER2, __VA_ARGS__) \
    struct Empty \
    FOR_EACH(CLOSE_TEMPLATE1, __VA_ARGS__);\
};

#include <type_traits>

struct Empty {};

template <typename T, typename std::enable_if<!std::is_same<T, float>::value, int>::type = struct Empty>
struct Optional
{
    bool p;
    T v;
};

template<typename T, typename TNext>
struct Link
{
    TNext next;
    T value;
};

NAMEDSTRUCT3(MessageName,
             (int) a,
             (Optional<char>) b,
             (double) c,
             (Optional<float>) d
            );

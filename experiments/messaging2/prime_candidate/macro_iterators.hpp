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

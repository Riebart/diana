#include <stdio.h>
#include <stdint.h>
// #include <stdbool.h> // This is the right way, but it makes for messy preprocessor output
// typedef enum { false, true } bool;

#define WN(N, T) N ## wrap ## T

#define W(N, T) \
    typedef struct WN(N, T) { T v; } WN(N, T); \
    inline static size_t dump_size_ ## N ## wrap ## T (WN(N, T) *s) { return dump_size_ ## T (&s->v); }

#define S(N, T) \
    typedef struct N { T v; } N; \
    inline static size_t dump_size_ ## N(N *s) { return sizeof(T); }

#define OptionalProperty(N, T, TN) \
    W(N, T) \
    typedef struct N { WN(N, T) v; TN n; } N; \
    inline static size_t dump_size_ ## N (N *s) { return dump_size_ ## N ## wrap ## T (&s->v) + dump_size_ ## TN (&s->n); }

#define RequiredProperty(N, T, TN) \
    inline static size_t dump_size_ ## T (T *s) { return sizeof(T); } \
    OptionalProperty(N, T, TN)

#define OPT(T) \
    typedef struct OPT ## T { T v; int p; } OPT ## T; \
    inline static size_t dump_size_ ## OPT ## T (OPT ## T *s) { return 1 + (s->p ? sizeof(s->v) : 0); }

typedef struct Vector3
{
    double w, x, y, z;
} Vector3;

typedef struct Vector4
{
    double w, x, y, z;
} Vector4;

OPT(double)
OPT(Vector3)
OPT(Vector4)

S(A, int64_t)
RequiredProperty(B, int64_t, A)
OptionalProperty(C, OPTdouble, B)
OptionalProperty(D, OPTdouble, C)
OptionalProperty(E, OPTVector3, D)
OptionalProperty(F, OPTVector3, E)
OptionalProperty(G, OPTVector3, F)
OptionalProperty(PhysPropsMsgR, OPTVector4, G)

struct PhysPropsMsg
{
    OPTVector4 orienatation;
    OPTVector3 thrust, velocity, position;
    OPTdouble radius, mass;
    int64_t client_id, server_id;
};

int main(int argc, char** argv)
{
    int b[6];
    sscanf(argv[1], "%d %d %d %d %d %d", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]);
    PhysPropsMsgR msg;
    struct PhysPropsMsg* v = (struct PhysPropsMsg*)(&msg);
    v->orienatation.p = b[0];
    v->position.p = b[1];
    v->velocity.p = b[2];
    v->thrust.p = b[3];
    v->mass.p = b[4];
    v->radius.p = b[5];

    printf("%d\n", v->orienatation.p);
    printf("%d\n", v->thrust.p);
    printf("%d\n", v->mass.p);
    printf("%lu\n", sizeof(msg));
    printf("%lu\n", dump_size_PhysPropsMsgR(&msg));
    return 0;
}

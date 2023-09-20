// #include <stdint.h>
// #include <stdbool.h> // This is the right way, but it makes for mess preprocessor output
typedef enum { false, true } bool;

struct Vector3
{
    double x, y, z;
};

typedef struct Vector3
{
    double w, x, y, z;
} Vector3;

typedef struct Vector4
{
    double w, x, y, z;
} Vector4;

#define OPTIONAL(type) typedef struct OPT ## type { type val; bool present; } OPT ## type;

OPTIONAL(double)
OPTIONAL(Vector3)
OPTIONAL(Vector4)

struct PPM
{
    int client_id, server_id;
    OPTdouble mass, radius;
    OPTVector3 position, velocity, acceleration;
    OPTVector4 orientation;
};



uint PPM_dump_size(struct PPM* ppm)
{
    return (
        sizeof(ppm->client_id) + 
        sizeof(ppm->server_id) + 
        sizeof(ppm->mass.val) * ppm->mass.present + 
        sizeof(ppm->radius.val) * ppm->radius.present + 
        sizeof(ppm->position.val) * ppm->position.present + 
    );
}
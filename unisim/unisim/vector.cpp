#include "vector.hpp"

#include <math.h>
#include <stdlib.h>

#define SIGN(x) (((x) < 0) ? -1 : (((x) > 0) ? 1 : 0))
#define CHOP_CUTOFF 1e-8

typedef struct Vector3 V3;

//! @todo Clarify some of these functions. In-place operations can be done by specifying
//! return value as one of the inputs, assuming there is no interdependency. Ass explicit
//! in-place options for all of these, and you can differentiate by signature.
//! @todo standardize what allocates a vector and what doesn't. Maybe force no allocations?

V3* Vector3_alloc(int32_t n)
{
    return (V3*)malloc(sizeof(V3) * n);
}

V3* Vector3_clone(V3* v)
{
    V3* r = Vector3_alloc();

    r->x = v->x;
    r->y = v->y;
    r->z = v->z;

    return r;
}

void Vector3_init(V3* out, double x, double y, double z)
{
    out->x = x;
    out->y = y;
    out->z = z;
}

void Vector3_init(V3* out, V3* v)
{
    out->x = v->x;
    out->y = v->y;
    out->z = v->z;
}

bool Vector3_almost_zeroS(double v)
{
    return ((v > -CHOP_CUTOFF) && (v < CHOP_CUTOFF));
}

bool Vector3_almost_zero(V3* v)
{
    return (Vector3_almost_zeroS(v->x) && Vector3_almost_zeroS(v->y) && Vector3_almost_zeroS(v->z));
}

void Vector3_add(V3* out, V3* v1, V3* v2)
{
    out->x = v1->x + v2->x;
    out->y = v1->y + v2->y;
    out->z = v1->z + v2->z;
}

void Vector3_add(V3* v1, V3* v2)
{
    Vector3_add(v1, v1, v2);
}

void Vector3_subtract(V3* out, V3* v1, V3* v2)
{
    out->x = v1->x - v2->x;
    out->y = v1->y - v2->y;
    out->z = v1->z - v2->z;
}

void Vector3_subtract(V3* v1, V3* v2)
{
    Vector3_subtract(v1, v1, v2);
}

void Vector3_scale(V3* out, V3* v, double s)
{
    out->x = v->x * s;
    out->y = v->y * s;
    out->z = v->z * s;
}

void Vector3_scale(V3* v, double s)
{
    Vector3_scale(v, v, s);
}

void Vector3_normalize(V3* out, V3* v)
{
    Vector3_scale(out, v, 1.0 / Vector3_length(v));
}

void Vector3_normalize(V3* v)
{
    Vector3_scale(v, 1.0 / Vector3_length(v));
}

void Vector3_cross(V3* out, V3* v1, V3* v2)
{
    out->x = v1->y * v2->z - v1->z * v2->y;
    out->y = v1->z * v2->x - v1->x * v2->z;
    out->z = v1->x * v2->y - v1->y * v2->x;
}

void Vector3_project_onto(V3* out, V3* v, V3* axis)
{
    *out = *axis;
    Vector3_scale(out, Vector3_dot(v, axis) / Vector3_length2(axis));
}

void Vector3_project_down(V3* out, V3* v, V3* axis)
{
    Vector3_project_onto(out, v, axis);
    Vector3_subtract(out, v, out);
}

double Vector3_dot(V3* v1, V3* v2)
{
    return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

double Vector3_length2(V3* v)
{
    return v->x * v->x + v->y * v->y + v->z * v->z;
}

double Vector3_length(V3* v)
{
    return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

double Vector3_distance2(V3* v1, V3* v2)
{
    V3 d;
    Vector3_subtract(&d, v1, v2);
    return Vector3_length2(&d);
}

double Vector3_distance(V3* v1, V3* v2)
{
    V3 d;
    Vector3_subtract(&d, v1, v2);
    return Vector3_length(&d);
}

//! Produce a vector that starts at 2 and goes to 1
void Vector3_ray(V3* out, V3* v1, V3* v2)
{
    Vector3_subtract(out, v2, v1);
    Vector3_normalize(out);
}

void Vector3_fmad(V3* v, double s, V3* u)
{
    v->x += s * u->x;
    v->y += s * u->y;
    v->z += s * u->z;
}

V3* Vector3_easy_look_at(V3* look)
{
    // Produce some arbitrary up and right vectors for a given look-at vector.
    // Do this by finding a vector that dots to zero with the look vector,
    // then just cross for the right vector.

    // forward, up, right
    V3* ret = Vector3_alloc(3);

    ret[0] = *look;
    Vector3_init(&ret[1], -look->z, 0, look->x);
    Vector3_cross(&ret[2], &ret[1], look);

    return ret;
}

V3* Vector3_easy_look_at2(V3* forward, V3* up, V3* right, V3* look)
{
    // forward, up, right
    V3* ret = Vector3_alloc(3);

    V3 diff = {0, 0, 0};
    Vector3_subtract(&diff, forward, look);

    if (Vector3_almost_zero(&diff))
    {
        return NULL;
    }

    V3 axis = {0, 0, 0};
    Vector3_cross(&axis, &diff, forward);

    //! @TODO FINISH THIS
    return ret;
}

//@staticmethod
//def get_orientation(forward, up, right):
//    # Turn the three vectors in a four-tuple that uniquely defines then
//    # orientation basis vectors, assuming they are unit vectors.
//    return [ forward.x, forward.y, up.x, up.y ]

//@staticmethod
//def from_orientation(o):
//    # Build the three orientation basis vectors from the orientation
//    # 4-tuple
//    forward = Vector3([o[0], o[1], sqrt(1 - o[0] * o[0] - o[1] * o[1])])
//    up = Vector3([o[2], o[3], sqrt(1 - o[2] * o[2] - o[3] * o[3])])
//    right = Vector3.cross(up, forward)

//    return [ forward, up, right ]

void Vector3_rotate_around(V3* v, double x, double y, double z, double angle)
{
    if (Vector3_almost_zeroS(angle))
    {
        return;
    }

    double c = cos(angle);
    double s = sin(angle);
    double l2 = x * x + y * y + z * z;
    double l = sqrt(l2);

    if (Vector3_almost_zeroS(l2))
    {
        return;
    }

    double x2 = v->y*((x*y-c*x*y)/l2+(s*z)/l)+(v->x*(pow(x,2)+c*(pow(y,2)+pow(z,2))))/l2+(-((s*y)/l)+(x*z-c*x*z)/l2)*v->z;
    double y2 = v->x*((x*y-c*x*y)/l2-(s*z)/l)+(v->y*(pow(y,2)+c*(pow(x,2)+pow(z,2))))/l2+((s*x)/l+(y*z-c*y*z)/l2)*v->z;
    double z2 = v->x*((s*y)/l+(x*z-c*x*z)/l2)*+v->y*(-((s*x)/l)+(y*z-c*y*z)/l2)+((c*(pow(x,2)+pow(y,2))+pow(z,2))*v->z)/l2;

    Vector3_init(v, x2, y2, z2);
}

void Vector3_rotate_around(V3* v, V3* axis, double angle)
{
    Vector3_rotate_around(v, axis->x, axis->y, axis->z, angle);
}

void Vector3_apply_ypr(V3* forward, V3* up, V3* right, V3* angles)
{
    // Apply yaw, pitch, and roll.
    // Order matters here, and changing the order changes the result.
    Vector3_rotate_around(forward, up, angles->x);
    Vector3_rotate_around(right, up, angles->x);

    Vector3_rotate_around(forward, right, angles->y);
    Vector3_rotate_around(up, right, angles->y);

    Vector3_rotate_around(right, forward, angles->z);
    Vector3_rotate_around(up, forward, angles->z);
}

//! Compares two AABBs, and returns negative if the first comes before the second, positive if vice versa, and zero otherwise.
int32_t Vector3_compare_aabb(struct AABB* a, struct AABB* b)
{
    double c;
    
    c = a->l.x - b->l.x;
    if (!Vector3_almost_zeroS(c))
    {
        return SIGN(c);
    }

    c = a->l.y - b->l.y;
    if (!Vector3_almost_zeroS(c))
    {
        return SIGN(c);
    }

    c = a->l.z - b->l.z;
    if (!Vector3_almost_zeroS(c))
    {
        return SIGN(c);
    }

    return 0;
}

int32_t Vector3_compare_aabb(struct AABB* a, struct AABB* b, int32_t d)
{
    double c;

    c = ((double*)&a->l)[d] - ((double*)&b->l)[d];

    if (!Vector3_almost_zeroS(c))
    {
        return SIGN(c);
    }
    else
    {
        return 0;
    }
}

int32_t Vector3_compare_aabbX(struct AABB* a, struct AABB* b)
{
    double c;
    
    c = a->l.x - b->l.x;
    if (!Vector3_almost_zeroS(c))
    {
        return SIGN(c);
    }
    else
    {
        return 0;
    }
}

int32_t Vector3_compare_aabbY(struct AABB* a, struct AABB* b)
{
    double c;
    
    c = a->l.y - b->l.y;
    if (!Vector3_almost_zeroS(c))
    {
        return SIGN(c);
    }
    else
    {
        return 0;
    }
}

int32_t Vector3_compare_aabbZ(struct AABB* a, struct AABB* b)
{
    double c;
    
    c = a->l.z - b->l.z;
    if (!Vector3_almost_zeroS(c))
    {
        return SIGN(c);
    }
    else
    {
        return 0;
    }
}

bool Vector3_intersect_interval(double al, double au, double bl, double bu)
{
    // We need to do a full test for the other two dimensions.
    // This means test all of the intersection conditions as well as being careful
    // to be sensitive to then butting up against each other.
    // 1) The end of b intersecting the start of a
    //                AAAAAAAAAA
    //           BBBBBBBBBB
    // 2) The end of a intersecting the start of b
    //                AAAAAAAAAA
    //                      BBBBBBBBBB
    // 3) a contained in b
    //                AAAAAAAAAA
    //             BBBBBBBBBBBBBBBB
    // 4) b contained in a
    //                AAAAAAAAAA
    //                  BBBBB
    //
    // 1 and 4 both have that b->u < a->u, and 2 and 3 both share that a->u < b->u
    //
    // The cases of equality don't necessarily need ot be handled explicitly, but
    // they need to be cared for. We can handle them, but canonically considering them
    // an 'intersection', and bundling a call to Vector3_almost_zeroS() of the double
    // diffference with the case of proper intersection. See above with how we got into
    // this block.
    // The cases are:
    // 1e) b touches the low side of a
    //                AAAAAAAAAA
    //      BBBBBBBBBB
    // 2e) a touches the low side of b
    //                AAAAAAAAAA
    //                          BBBBBBBBBB
    // 3e) b is contained in a and touches one or both of the ends
    //                AAAAAAAAAA
    //                BBBBBBBBBB
    //                BBBBBB
    //                    BBBBBB
    // 4e) a is contained in b and touches one or both of the ends
    //                AAAAAA
    //                    AAAAAA
    //                AAAAAAAAAA
    //                BBBBBBBBBB
    //
    // On the other hand, the ways in which we can NOT be intersecting is much, much simpler:
    // A) b is wholly in front of a
    //                AAAAAAAAA.au|
    //                            |bl.BBBBBBBBB
    // B) a is wholly in front of b
    //                 |al.AAAAAAAAA
    //     BBBBBBBBB.bu|
    //
    // So check to see if there are gaps in between au and bl, or bu and al
    
    double d;

    // Case A: A before B
    d = bl - au; // Should be positive if there's a gap
    if (d > 0)
    {
        return false;
    }

    // Case B: A after B
    d = al - bu; // Should be positive if there's a gap
    if (d > 0)
    {
        return false;
    }

    return true;
}

bool Vector3_intersect_aabb(struct AABB* a, struct AABB* b)
{
    //! @todo This isn't correct, fix it.

    // Since the primary use is going to be when a starts before b
    // First check to see if b->l is 'less than' a->u
    // We need that to be true for all components.
    bool c = (a->u.x >= b->l.x);
    c &= (a->u.y >= b->l.y);
    c &= (a->u.z >= b->l.z);

    if (c)
    {
        return true;
    }

    // The alternative is that a->l is less than b->u
    c = (b->u.x >= a->l.x);
    c &= (b->u.y >= a->l.y);
    c &= (b->u.z >= a->l.z);

    return c;
}

bool Vector3_intersect_aabb(struct AABB* a, struct AABB* b, int d)
{
    return Vector3_intersect_interval(((double*)&a->l)[d], ((double*)&a->u)[d], ((double*)&b->l)[d], ((double*)&b->u)[d]);
}

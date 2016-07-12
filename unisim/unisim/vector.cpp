#include "vector.hpp"

#include <math.h>
#include <stdlib.h>
#include <stdexcept>

#define SIGN(x) (((x) < 0) ? -1 : (((x) > 0) ? 1 : 0))
#define CHOP_CUTOFF 1e-8

namespace Diana
{
    typedef struct Vector3 V3;
    typedef struct Vector4 V4;

    //V3* Vector3_easy_look_at(V3* look)
    //{
    //    // Produce some arbitrary up and right vectors for a given look-at vector.
    //    // Do this by finding a vector that dots to zero with the look vector,
    //    // then just cross for the right vector.

    //    // forward, up, right
    //    V3* ret = new V3[3];
    //    //V3* ret = Vector3_alloc(3);

    //    ret[0] = *look;
    //    ret[1].init(-look->z, 0, look->x);
    //    //Vector3_init(&ret[1], -look->z, 0, look->x);
    //    ret[2] = ret[1].cross(*look);
    //    //Vector3_cross(&ret[2], &ret[1], look);

    //    return ret;
    //}

    //V3* Vector3_easy_look_at2(V3* forward, V3* up, V3* right, V3* look)
    //{
    //    // forward, up, right
    //    V3* ret = new V3[3];
    //    //V3* ret = Vector3_alloc(3);

    //    //V3 diff = { 0, 0, 0 };
    //    //Vector3_subtract(&diff, forward, look);
    //    V3 diff = *forward - *look;

    //    //if (Vector3_almost_zero(&diff))
    //    if (diff.almost_zero())
    //    {
    //        return NULL;
    //    }

    //    //V3 axis = { 0, 0, 0 };
    //    //Vector3_cross(&axis, &diff, forward);
    //    V3 axis = diff.cross(*forward);

    //    //! @TODO FINISH THIS
    //    return ret;
    //}

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

    //void Vector3_rotate_around(V3* v, double x, double y, double z, double angle)
    //{
    //    if (V3::almost_zeroS(angle))
    //    {
    //        return;
    //    }

    //    double c = cos(angle);
    //    double s = sin(angle);
    //    double l2 = x * x + y * y + z * z;
    //    double l = sqrt(l2);

    //    if (V3::almost_zeroS(l2))
    //    {
    //        return;
    //    }

    //    double x2 = v->y*((x*y - c*x*y) / l2 + (s*z) / l) + (v->x*(pow(x, 2) + c*(pow(y, 2) + pow(z, 2)))) / l2 + (-((s*y) / l) + (x*z - c*x*z) / l2)*v->z;
    //    double y2 = v->x*((x*y - c*x*y) / l2 - (s*z) / l) + (v->y*(pow(y, 2) + c*(pow(x, 2) + pow(z, 2)))) / l2 + ((s*x) / l + (y*z - c*y*z) / l2)*v->z;
    //    double z2 = v->x*((s*y) / l + (x*z - c*x*z) / l2)*+v->y*(-((s*x) / l) + (y*z - c*y*z) / l2) + ((c*(pow(x, 2) + pow(y, 2)) + pow(z, 2))*v->z) / l2;

    //    v->init(x2, y2, z2);
    //    //Vector3_init(v, x2, y2, z2);
    //}

    //void Vector3_rotate_around(V3* v, V3* axis, double angle)
    //{
    //    Vector3_rotate_around(v, axis->x, axis->y, axis->z, angle);
    //}

    //void Vector3_apply_ypr(V3* forward, V3* up, V3* right, V3* angles)
    //{
    //    // Apply yaw, pitch, and roll.
    //    // Order matters here, and changing the order changes the result.
    //    Vector3_rotate_around(forward, up, angles->x);
    //    Vector3_rotate_around(right, up, angles->x);

    //    Vector3_rotate_around(forward, right, angles->y);
    //    Vector3_rotate_around(up, right, angles->y);

    //    Vector3_rotate_around(right, forward, angles->z);
    //    Vector3_rotate_around(up, forward, angles->z);
    //}

    //template<typename T> int32_t AABBT<T>::operator<(struct AABBT<T>& b)
    //{
    //    T c = l.x - b.l.x;
    //    if (!Vector3T<T>::almost_zeroS(c))
    //    {
    //        return SIGN(c);
    //    }

    //    c = l.y - b.l.y;
    //    if (!Vector3T<T>::almost_zeroS(c))
    //    {
    //        return SIGN(c);
    //    }

    //    c = l.z - b.l.z;
    //    if (!Vector3T<T>::almost_zeroS(c))
    //    {
    //        return SIGN(c);
    //    }

    //    return 0;
    //}

    //template<typename T> int32_t AABBT<T>::compare_y(struct AABBT<T>& b)
    //{
    //    T c = l.y - b.l.y;
    //    if (!Vector3T<T>::almost_zeroS(c))
    //    {
    //        return SIGN(c);
    //    }
    //    else
    //    {
    //        return 0;
    //    }
    //}

    //template<typename T> int32_t AABBT<T>::compare_z(struct AABBT<T>& b)
    //{
    //    T c = l.z - b.l.z;
    //    if (!Vector3T<T>::almost_zeroS(c))
    //    {
    //        return SIGN(c);
    //    }
    //    else
    //    {
    //        return 0;
    //    }
    //}

    //bool AABB::intersect(struct AABB& b)
    //{
    //    //! @todo This isn't correct, fix it.

    //    // Since the primary use is going to be when a starts before b
    //    // First check to see if b->l is 'less than' a->u
    //    // We need that to be true for all components.
    //    bool c = (u.x >= b.l.x);
    //    c &= (u.y >= b.l.y);
    //    c &= (u.z >= b.l.z);

    //    if (c)
    //    {
    //        return true;
    //    }

    //    // The alternative is that a->l is less than b->u
    //    c = (b.u.x >= l.x);
    //    c &= (b.u.y >= l.y);
    //    c &= (b.u.z >= l.z);

    //    return c;
    //}
}

#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <stdint.h>

namespace Diana
{
    struct Vector3
    {
        double x, y, z;
    };

    struct Vector4
    {
        double w, x, y, z;
    };

    const struct Vector3 vector3d_zero = { 0.0, 0.0, 0.0 };
    const struct Vector4 vector4d_zero = { 0.0, 0.0, 0.0, 0.0 };

    //! Represents an axis-aligned bounding box
    struct AABB
    {
        //! Lower coordinates
        struct Vector3 l;
        //! Upper coordinates
        struct Vector3 u;
    };

    struct Vector3* Vector3_alloc(int32_t n = 1);
    struct Vector3* Vector3_clone(struct Vector3* v);

    void Vector3_init(struct Vector3* v, double x, double y, double z);
    void Vector3_init(struct Vector3* out, struct Vector3* v);

    void Vector4_init(struct Vector4* v, double w, double x, double y, double z);
    void Vector4_init(struct Vector4* out, struct Vector4* v);

    bool Vector3_almost_zeroS(double v);
    bool Vector3_almost_zero(struct Vector3* v);

    void Vector3_round(struct Vector3* v, double e);
    void Vector3_roundS(double v, double e);

    void Vector3_add(struct Vector3* out, struct Vector3* v1, struct Vector3* v2);
    void Vector3_add(struct Vector3* v1, struct Vector3* v2);
    void Vector3_subtract(struct Vector3* out, struct Vector3* v1, struct Vector3* v2);
    void Vector3_subtract(struct Vector3* v1, struct Vector3* v2);
    void Vector3_scale(struct Vector3* out, struct Vector3* v, double s);
    void Vector3_scale(struct Vector3* v, double s);
    void Vector3_normalize(struct Vector3* out, struct Vector3* v);
    void Vector3_normalize(struct Vector3* v);

    void Vector3_cross(struct Vector3* out, struct Vector3* v1, struct Vector3* v2);
    void Vector3_project_onto(struct Vector3* out, struct Vector3* v, struct Vector3* axis);
    void Vector3_project_down(struct Vector3* out, struct Vector3* v, struct Vector3* axis);

    double Vector3_dot(struct Vector3* v1, struct Vector3* v2);
    const double Vector3_length2(const struct Vector3* v);
    const double Vector3_length(const struct Vector3* v);
    double Vector3_distance2(struct Vector3* v1, struct Vector3* v2);
    double Vector3_distance(struct Vector3* v1, struct Vector3* v2);

    void Vector3_ray(struct Vector3* out, struct Vector3* v1, struct Vector3* v2);
    void Vector3_fmad(struct Vector3* v, double s, struct Vector3* u);

    struct Vector3* Vector3_easy_look_at(struct Vector3* look);
    struct Vector3* Vector3_easy_look_at2(struct Vector3* forward, struct Vector3* up, struct Vector3* right, struct Vector3* look);
    void Vector3_rotate_around(struct Vector3* v, double x, double y, double z, double angle);
    void Vector3_rotate_around(struct Vector3* v, struct Vector3* axis, double angle);
    void Vector3_apply_ypr(struct Vector3* forward, struct Vector3* up, struct Vector3* right, struct Vector3* angles);

    int32_t Vector3_compare_aabb(struct AABB* a, struct AABB* b);
    int32_t Vector3_compare_aabb(struct AABB* a, struct AABB* b, int32_t d);
    int32_t Vector3_compare_aabbX(struct AABB* a, struct AABB* b);
    int32_t Vector3_compare_aabbY(struct AABB* a, struct AABB* b);
    int32_t Vector3_compare_aabbZ(struct AABB* a, struct AABB* b);
    bool Vector3_intersect_interval(double al, double au, double bl, double bu);
    bool Vector3_intersect_aabb(struct AABB* a, struct AABB* b);
}
#endif

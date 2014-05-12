#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <stdint.h>

struct Vector3
{
    double x, y, z;
};

struct Vector3* Vector3_alloc(int32_t n  = 1);
struct Vector3* Vector3_clone(struct Vector3* v);

void Vector3_init(struct Vector3* v, double x, double y, double z);
void Vector3_init(struct Vector3* out, struct Vector3* v);

bool Vector3_almost_zeroS(double v);
bool Vector3_almost_zero(struct Vector3* v);

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
double Vector3_length2(struct Vector3* v);
double Vector3_length(struct Vector3* v);
double Vector3_distance2(struct Vector3* v1, struct Vector3* v2);
double Vector3_distance(struct Vector3* v1, struct Vector3* v2);

void Vector3_ray(struct Vector3* out, struct Vector3* v1, struct Vector3* v2);
void Vector3_fmad(struct Vector3* v, double s, struct Vector3* u);

struct Vector3* Vector3_easy_look_at(struct Vector3* look);
struct Vector3* Vector3_easy_look_at2(struct Vector3* forward, struct Vector3* up, struct Vector3* right, struct Vector3* look);
void Vector3_rotate_around(struct Vector3* v, double x, double y, double z, double angle);
void Vector3_rotate_around(struct Vector3* v, struct Vector3* axis, double angle);
void Vector3_apply_ypr(struct Vector3* forward, struct Vector3* up, struct Vector3* right, struct Vector3* angles);

#endif

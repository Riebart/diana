#pragma once

#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <stdint.h>

#include <math.h>

namespace Diana
{
    struct Vector
    {
        static bool almost_zeroS(double a) { return ((a > -1e-8) && (a < 1e-8)); }
        static bool almost_zeroS(int64_t a) { return (a == 0); }
        template<typename T> static bool almost_zeroS(T a) { return Vector::almost_zeroS((double)a); }
        template<typename T> static int32_t sgn(T val) { return (T(0) < val) - (val < T(0)); }
        template<typename T> static T abs(T val) { return (Vector::sgn(val) * val); }
    };

    template<typename T = double> struct Vector3T
    {
        T x, y, z;

        Vector3T<T>() {}
        Vector3T<T>(const struct Vector3T<T>& a) { init(a); }
        Vector3T<T>(T _x, T _y, T _z) { init(_x, _y, _z); }

        void init(T _x, T _y, T _z) { x = _x; y = _y; z = _z; }
        void init(const struct Vector3T<T>& a) { x = a.x; y = a.y; z = a.z; }

        inline bool almost_zero() const
        {
            return Vector::almost_zeroS(x) &&
                Vector::almost_zeroS(y) &&
                Vector::almost_zeroS(z);
        }

        // @todo For integer T, it might be faster to get the integar part only
        // See: http://stackoverflow.com/questions/4930307/fastest-way-to-get-the-integer-part-of-sqrtn
        inline static T length(T x, T y, T z) { return (T)sqrt(x * x + y * y + z * z); }
        inline static T length2(T x, T y, T z) { return x * x + y * y + z * z; }

        inline static T length(const struct Vector3T<T>& a) { return Vector3T<T>::length(a.x, a.y.a.z); }
        inline static T length2(const struct Vector3T<T>& a) { return Vector3T<T>::length2(a.x, a.y.a.z); }

        inline T length() const { return Vector3T<T>::length(x, y, z); }
        inline T length2() const { return Vector3T<T>::length2(x, y, z); }

        inline T distance(const struct Vector3T<T>& a) const { return Vector3T<T>::length(x - a.x, y - a.y, z - a.z); }
        inline T distance2(const struct Vector3T<T>& a) const { return Vector3T<T>::length2(x - a.x, y - a.y, z - a.z); }

        inline double length2_dbl() const
        {
            return Vector3T<double>::length2((double)x, (double)y, (double)z);
        }

        inline double distance2_dbl(const struct Vector3T<T>& a) const
        {
            return Vector3T<double>::length2((double)(x - a.x), (double)(y - a.y), (double)(z - a.z));
        }

        T maxabs() const
        {
            const auto l = [max](T v) { return (v > max ? v : max); };
            T max = Vector::abs(x);
            max = l(Vector::abs(y));
            max = l(Vector::abs(z));
            return max;
        }

        inline void normalize()
        {
            T l = length();
            if (Vector::almost_zeroS(l))
            {
                init(0, 0, 0);
            }
            else
            {
                this->operator/=(l);
            }
        }

        struct Vector3T<double> normalize_dbl()
        {
            // Doing this in two lines results in using the operator=() function,
            // however if it's done in one line (assignment and declaration on the
            // same line), it uses the converstion operations (not defined, so results
            // in a compiler error).
            struct Vector3T<double> r = *this;
            r.normalize();
            return r;
        }

        template<typename T2>
        static struct Vector3T<double> normalize(const struct Vector3T<T2>& a)
        {
            struct Vector3T<double> v = a;
            v.normalize();
            return v;
        }

        T dot(const struct Vector3T<T>& a) const { return x * a.x + y * a.y + z * a.z; }

        struct Vector3T<T> cross(const struct Vector3T<T>& a) const
        {
            return{
                y * a.z - z * a.y,
                z * a.x - x * a.z,
                x * a.y - y * a.x
            };
        }

        void rotate_around(double aX, double aY, double aZ, double theta)
        {
            if (Vector::almost_zeroS(theta))
            {
                return;
            }

            double c = cos(theta);
            double s = sin(theta);
            T l2 = aX * aX + aY * aY + aZ * aZ;
            double l = sqrt(l2);

            if (Vector::almost_zeroS(l2))
            {
                return;
            }

            T x2 = (T)(y*((aX*aY - c*aX*aY) / l2 + (s*aZ) / l) + (x*(pow(aX, 2) + c*(pow(aY, 2) + pow(aZ, 2)))) / l2 + (-((s*aY) / l) + (aX*aZ - c*aX*aZ) / l2)*z);
            T y2 = (T)(x*((aX*aY - c*aX*aY) / l2 - (s*aZ) / l) + (y*(pow(aY, 2) + c*(pow(aX, 2) + pow(aZ, 2)))) / l2 + ((s*aX) / l + (aY*aZ - c*aY*aZ) / l2)*z);
            T z2 = (T)(x*((s*aY) / l + (aX*aZ - c*aX*aZ) / l2)*+y*(-((s*aX) / l) + (aY*aZ - c*aY*aZ) / l2) + ((c*(pow(aX, 2) + pow(aY, 2)) + pow(aZ, 2))*z) / l2);

            init(x2, y2, z2);
        }

        void rotate_around(struct Vector3T<double>& axis, double theta)
        {
            rotate_around(axis.x, axis.y, axis.z, theta);
        }

        static void apply_yaw_pitch_roll(
            struct Vector3T<double>& forward,
            struct Vector3T<double>& up,
            struct Vector3T<double>& right,
            struct Vector3T<double>& angles)
        {
            forward.rotate_around(up, angles.x);
            right.rotate_around(up, angles.x);

            forward.rotate_around(right, angles.y);
            up.rotate_around(right, angles.y);

            right.rotate_around(forward, angles.z);
            up.rotate_around(forward, angles.z);
        }

        // For all of the following, using static_cast<T> as opposed to (T) results
        // in more stable and predictable performance. These should be all compiled out
        // (since this code is specialized at compile-time) for operations that have
        // the same types for both the argument and the object that owns the member
        // being called.
        template<typename T2>
        struct Vector3T<T> operator=(const struct Vector3T<T2>& a) { x = static_cast<T>(a.x); y = static_cast<T>(a.y); z = static_cast<T>(a.z); return *this; }

        template<typename T2>
        operator Vector3T<T2>() const { return{ (T2)x, (T2)y, (T2)z }; }

        template <typename T2, typename T3>
        inline void fmad(T2 a, struct Vector3T<T3>& b) { x += static_cast<T>(a * b.x); y += static_cast<T>(a * b.y); z += static_cast<T>(a * b.z); }

        template <typename T2>
        struct Vector3T<T> operator+(const struct Vector3T<T2>& a) const { return{ x + static_cast<T>(a.x), y + static_cast<T>(a.y), z + static_cast<T>(a.z) }; }
        template <typename T2>
        struct Vector3T<T> operator-(const struct Vector3T<T2>& a) const { return{ x - static_cast<T>(a.x), y - static_cast<T>(a.y), z - static_cast<T>(a.z) }; }
        template <typename T2>
        struct Vector3T<T> operator*(const struct Vector3T<T2>& a) const { return{ x * static_cast<T>(a.x), y * static_cast<T>(a.y), z * static_cast<T>(a.z) }; }
        template <typename T2>
        struct Vector3T<T> operator/(const struct Vector3T<T2>& a) const { return{ x / static_cast<T>(a.x), y / static_cast<T>(a.y), z / static_cast<T>(a.z) }; }

        template <typename T2>
        inline void operator+=(const struct Vector3T<T2>& a) { x += static_cast<T>(a.x); y += static_cast<T>(a.y); z += static_cast<T>(a.z); }
        template <typename T2>
        inline void operator-=(const struct Vector3T<T2>& a) { x -= static_cast<T>(a.x); y -= static_cast<T>(a.y); z -= static_cast<T>(a.z); }
        template <typename T2>
        inline void operator*=(const struct Vector3T<T2>& a) { x *= static_cast<T>(a.x); y *= static_cast<T>(a.y); z *= static_cast<T>(a.z); }
        template <typename T2>
        inline void operator/=(const struct Vector3T<T2>& a) { x /= static_cast<T>(a.x); y /= static_cast<T>(a.y); z /= static_cast<T>(a.z); }

        template <typename T2>
        struct Vector3T<T> operator+(T2 a) const { return{ x + static_cast<T>(a), y + static_cast<T>(a), z + static_cast<T>(a) }; }
        template <typename T2>
        struct Vector3T<T> operator-(T2 a) const { return{ x - static_cast<T>(a), y - static_cast<T>(a), z - static_cast<T>(a) }; }
        template <typename T2>
        struct Vector3T<T> operator*(T2 a) const { return{ x * static_cast<T>(a), y * static_cast<T>(a), z * static_cast<T>(a) }; }
        template <typename T2>
        struct Vector3T<T> operator/(T2 a) const { return{ x / static_cast<T>(a), y / static_cast<T>(a), z / static_cast<T>(a) }; }

        template <typename T2>
        inline void operator+=(T2 a) { x += static_cast<T>(a); y += static_cast<T>(a); z += static_cast<T>(a); }
        template <typename T2>
        inline void operator-=(T2 a) { x -= static_cast<T>(a); y -= static_cast<T>(a); z -= static_cast<T>(a); }
        template <typename T2>
        inline void operator*=(T2 a) { x *= static_cast<T>(a); y *= static_cast<T>(a); z *= static_cast<T>(a); }
        template <typename T2>
        inline void operator/=(T2 a) { x /= static_cast<T>(a); y /= static_cast<T>(a); z /= static_cast<T>(a); }

        struct Vector3T<T> project_onto(const struct Vector3T<T>& a) const
        {
            return a * (a.dot(*this) / a.length2());
        }

        struct Vector3T<T> project_down(const struct Vector3T<T>& a) const
        {
            return (*this - this->project_onto(a));
        }
    };

    // Implementation of a more numerically stable Euclidean norm function that
    // is less likely to result in integer overflows.
    int64_t Vector3T<int64_t>::length(int64_t x, int64_t y, int64_t z)
    {
        int64_t max = Vector::abs(x);
        const auto l = [max](int64_t v) { return (v > max ? v : max); };
        max = l(Vector::abs(y));
        max = l(Vector::abs(z));

        double x2 = (double)x / max;
        double y2 = (double)y / max;
        double z2 = (double)z / max;

        double sum = x2 * x2 + y2 * y2 + z2 * z2;
        return (int64_t)(max * sqrt(sum));
    }

    // int64_t normalization equates to setting the maximal value to
    // SGN(maximal_value), if there's multiple values of equal magnitude,
    // all values are 0.
    void Vector3T<int64_t>::normalize()
    {
        int64_t max = Vector::abs(x);
        const auto l = [max](int64_t v) { return (v > max ? v : max); };
        max = l(Vector::abs(y));
        max = l(Vector::abs(z));
        if (max != 0)
        {
            x /= max;
            y /= max;
            z /= max;
            if ((Vector::abs(x) + Vector::abs(y) + Vector::abs(z)) > 1)
            {
                x = 0;
                y = 0;
                z = 0;
            }
        }
    }

    // Specialized implementation for int64_t projected onto int64_t, achieved
    // by using an intermediate representation as doubles. This is necessary
    // for two reasons:
    // - There is a .length2(), which suffers from overflow issues with int types.
    // - There is a dot product of two int64_t vectors, which, like length2(),
    //   suffers from an overflow problem.
    //
    // By delegating, and casting the return values to/from double precision, both
    // of the above issues are addressed.
    // - It is crucial that the parameter be case to a double, as it is the one
    //   that has the length2() member called.
    // - By casting a to doubles, the number of casts is minimized.
    // - During the dot product, the int values will be implicitly casted to doubles,
    //   divided by the double length2(), and the int vector will be scaled by a double,
    //   resulting in the double scalar being cast back to an int.
    struct Vector3T<int64_t> Vector3T<int64_t>::project_onto(const struct Vector3T<int64_t>& a) const
    {
        return this->project_onto((struct Vector3T<double>)a);
    }

    template<typename T = double> struct Vector4T
    {
        T w, x, y, z;

        Vector4T<T>() {}
        Vector4T<T>(const struct Vector4T<T>& a) { init(a); }
        Vector4T<T>(T _w, T _x, T _y, T _z) { init(_w, _x, _y, _z); }

        void init(T _w, T _x, T _y, T _z) { w = _w;  x = _x; y = _y; z = _z; }
        void init(const struct Vector4T<T>& a) { w = a.w;  x = a.x; y = a.y; z = a.z; }

        bool almost_zero() const
        {
            return Vector<T>::almost_zeroS(w) &&
                Vector<T>::almost_zeroS(x) &&
                Vector<T>::almost_zeroS(y) &&
                Vector<T>::almost_zeroS(z);
        }
    };

    //! Represents an axis-aligned bounding box
    template<typename T = double> struct AABBT
    {
        //! Lower coordinates
        struct Vector3T<T> l;
        //! Upper coordinates
        struct Vector3T<T> u;

        int32_t compare_x(struct AABBT<T>& b) const { return tiz(l.x - b.l.x); }
        int32_t compare_y(struct AABBT<T>& b) const { return tiz(l.y - b.l.y); }
        int32_t compare_z(struct AABBT<T>& b) const { return tiz(l.z - b.l.z); }

        static bool intersect_interval(T al, T au, T bl, T bu)
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

            T d;

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

    private:
        int32_t tiz(T val) const { return (Vector::almost_zeroS(val) ? 0 : Vector::sgn(val)); }
    };

#define Vector3 Vector3T<double>
#define Vector3I Vector3T<int64_t>
#define Vector4 Vector4T<double>
#define Vector4I Vector4T<int64_t>
#define AABB AABBT<double>
#define AABBI AABBT<int64_t>

    const struct Vector3T<double> vector3d_zero = { 0.0, 0.0, 0.0 };
    const struct Vector3T<int64_t> vector3i_zero = { 0, 0, 0 };
    const struct Vector4T<double> vector4d_zero = { 0.0, 0.0, 0.0, 0.0 };
    const struct Vector4T<int64_t> vector4i_zero = { 0, 0, 0, 0 };
}
#endif

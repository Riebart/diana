#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <stdint.h>

#include <math.h>

namespace Diana
{
    template<typename T = double> struct VectorT
    {
        static bool almost_zeroS(double a) { return ((a > -1e-8) && (a < 1e-8)); }
        static bool almost_zeroS(int64_t a) { return (a == 0); }
    };

    template<typename T = double> struct Vector3T : VectorT<T>
    {
        T x, y, z;

        Vector3T<T>() {}
        Vector3T<T>(const struct Vector3T<T>& a) { init(a); }
        Vector3T<T>(T _x, T _y, T _z) { init(_x, _y, _z); }

        void init(T _x, T _y, T _z) { x = _x; y = _y; z = _z; }
        void init(const struct Vector3T<T>& a) { x = a.x; y = a.y; z = a.z; }

        bool almost_zero() const
        {
            return VectorT<T>::almost_zeroS(x) &&
                VectorT<T>::almost_zeroS(y) &&
                VectorT<T>::almost_zeroS(z);
        }

        // @todo For integer T, it might be faster to get the integar part only
        // See: http://stackoverflow.com/questions/4930307/fastest-way-to-get-the-integer-part-of-sqrtn
        static T length(T x, T y, T z) { return (T)sqrt(x * x + y * y + z * z); }
        static T length2(T x, T y, T z) { return x * x + y * y + z * z; }

        T length() const { return (T)sqrt(length2()); }
        T length2() const { return x * x + y * y + z * z; }

        T distance(const struct Vector3T<T>& a) const { return Vector3T<T>::length(x - a.x, y - a.y, z - a.z); }
        T distance2(const struct Vector3T<T>& a) const { return Vector3T<T>::length2(x - a.x, y - a.y, z - a.z); }

        void normalize()
        {
            T l = length();
            if (!Vector3T<T>::almost_zeroS(l))
            {
                this->operator/=(l);
            }
            else
            {
                init(0, 0, 0);
            }
        }

        static struct Vector3T<T> normalize(const struct Vector3T<T>& a)
        {
            struct Vector3T<T> v = a;
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
            if (Vector3T<T>::almost_zeroS(theta))
            {
                return;
            }

            double c = cos(theta);
            double s = sin(theta);
            T l2 = aX * aX + aY * aY + aZ * aZ;
            double l = sqrt(l2);

            if (Vector3T<T>::almost_zeroS(l2))
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
        template <typename T2, typename T3>
        void fmad(T2 a, struct Vector3T<T3>& b) { x += static_cast<T>(a * b.x); y += static_cast<T>(a * b.y); z += static_cast<T>(a * b.z); }

        template <typename T2>
        struct Vector3T<T> operator+(const struct Vector3T<T2>& a) const { return{ x + static_cast<T>(a.x), y + static_cast<T>(a.y), z + static_cast<T>(a.z) }; }
        template <typename T2>
        struct Vector3T<T> operator-(const struct Vector3T<T2>& a) const { return{ x - static_cast<T>(a.x), y - static_cast<T>(a.y), z - static_cast<T>(a.z) }; }
        template <typename T2>
        struct Vector3T<T> operator*(const struct Vector3T<T2>& a) const { return{ x * static_cast<T>(a.x), y * static_cast<T>(a.y), z * static_cast<T>(a.z) }; }
        template <typename T2>
        struct Vector3T<T> operator/(const struct Vector3T<T2>& a) const { return{ x / static_cast<T>(a.x), y / static_cast<T>(a.y), z / static_cast<T>(a.z) }; }

        template <typename T2>
        void operator+=(const struct Vector3T<T2>& a) { x += static_cast<T>(a.x); y += static_cast<T>(a.y); z += static_cast<T>(a.z); }
        template <typename T2>
        void operator-=(const struct Vector3T<T2>& a) { x -= static_cast<T>(a.x); y -= static_cast<T>(a.y); z -= static_cast<T>(a.z); }
        template <typename T2>
        void operator*=(const struct Vector3T<T2>& a) { x *= static_cast<T>(a.x); y *= static_cast<T>(a.y); z *= static_cast<T>(a.z); }
        template <typename T2>
        void operator/=(const struct Vector3T<T2>& a) { x /= static_cast<T>(a.x); y /= static_cast<T>(a.y); z /= static_cast<T>(a.z); }

        template <typename T2>
        struct Vector3T<T> operator+(T2 a) const { return{ x + static_cast<T>(a), y + static_cast<T>(a), z + static_cast<T>(a) }; }
        template <typename T2>
        struct Vector3T<T> operator-(T2 a) const { return{ x - static_cast<T>(a), y - static_cast<T>(a), z - static_cast<T>(a) }; }
        template <typename T2>
        struct Vector3T<T> operator*(T2 a) const { return{ x * static_cast<T>(a), y * static_cast<T>(a), z * static_cast<T>(a) }; }
        template <typename T2>
        struct Vector3T<T> operator/(T2 a) const { return{ x / static_cast<T>(a), y / static_cast<T>(a), z / static_cast<T>(a) }; }

        template <typename T2>
        void operator+=(T2 a) { x += static_cast<T>(a); y += static_cast<T>(a); z += static_cast<T>(a); }
        template <typename T2>
        void operator-=(T2 a) { x -= static_cast<T>(a); y -= static_cast<T>(a); z -= static_cast<T>(a); }
        template <typename T2>
        void operator*=(T2 a) { x *= static_cast<T>(a); y *= static_cast<T>(a); z *= static_cast<T>(a); }
        template <typename T2>
        void operator/=(T2 a) { x /= static_cast<T>(a); y /= static_cast<T>(a); z /= static_cast<T>(a); }

        struct Vector3T<T> project_onto(const struct Vector3T<T>& a) const
        {
            return a * (this->dot(a) / a.length2());
        }

        struct Vector3T<T> project_down(const struct Vector3T<T>& a) const
        {
            return (*this - this->project_onto(a));
        }
    };

    template<typename T = double> struct Vector4T : VectorT<T>
    {
        T w, x, y, z;

        Vector4T<T>() {}
        Vector4T<T>(const struct Vector4T<T>& a) { init(a); }
        Vector4T<T>(T _w, T _x, T _y, T _z) { init(_w, _x, _y, _z); }

        void init(T _w, T _x, T _y, T _z) { w = _w;  x = _x; y = _y; z = _z; }
        void init(const struct Vector4T<T>& a) { w = a.w;  x = a.x; y = a.y; z = a.z; }

        bool almost_zero() const
        {
            return VectorT<T>::almost_zeroS(w) &&
                VectorT<T>::almost_zeroS(x) &&
                VectorT<T>::almost_zeroS(y) &&
                VectorT<T>::almost_zeroS(z);
        }
    };

    //! Represents an axis-aligned bounding box
    template<typename T = double> struct AABBT
    {
        //! Lower coordinates
        struct Vector3T<T> l;
        //! Upper coordinates
        struct Vector3T<T> u;

        int32_t operator<(struct AABBT<T>& b);
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
        int32_t sgn(T val) const { return (T(0) < val) - (val < T(0)); }
        int32_t tiz(T val) const { return (VectorT<T>::almost_zeroS(val) ? 0 : sgn(val)); }
    };

#define Vector3 Vector3T<>
#define Vector3I Vector3T<int64_t>
#define Vector4 Vector4T<>
#define Vector4I Vector4T<int64_t>
#define AABB AABBT<>
#define AABBI AABBT<int64_t>

    const struct Vector3 vector3d_zero = { 0, 0, 0 };
    const struct Vector4 vector4d_zero = { 0, 0, 0, 0 };


}
#endif

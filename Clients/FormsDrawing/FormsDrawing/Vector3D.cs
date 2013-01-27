using System;
using System.Collections.Generic;
using System.Text;

namespace FormsDrawing
{
    /// <summary>
    /// A double-precision and pared-down version of the DirectX Vector3 class
    /// </summary>
    public class Vector3D
    {
        /// <summary>
        /// Static vector representing a zero vector. Saves on some overhead once in a while.
        /// </summary>
        public static Vector3D Zero = new Vector3D(0, 0, 0);

        /// <summary>
        /// Retrieves or sets the x component of a 3-D vector
        /// </summary>
        public double X
        {
            get { return x; }
            set { x = value; }
        }

        /// <summary>
        /// Retrieves or sets the y component of a 3-D vector
        /// </summary>
        public double Y
        {
            get { return y; }
            set { y = value; }
        }

        /// <summary>
        /// Retrieves or sets the z component of a 3-D vector
        /// </summary>
        public double Z
        {
            get { return z; }
            set { z = value; }
        }

        double x;
        double y;
        double z;

        #region Static Members

        /// <summary>
        /// Overload the addition operator for Vector3D objects.
        /// </summary>
        /// <param name="v1">Left hand vector.</param>
        /// <param name="v2">Right hand vector</param>
        /// <returns>The sum of the two input vectors.</returns>
        public static Vector3D operator +(Vector3D v1, Vector3D v2)
        {
            return new Vector3D(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
        }

        /// <summary>
        /// Overload the addition operator for adding/subtracting a scalar on the right. THis is to emulate similar functionality to the vector operations in HLSL.
        /// </summary>
        /// <param name="v1">Vector3D object to be offset.</param>
        /// <param name="d">Scalar offset</param>
        /// <returns>Vector3D object whose componses are that of the original after having been scaled by d in all dimensions.</returns>
        public static Vector3D operator +(Vector3D v1, double d)
        {
            return new Vector3D(v1.x + d, v1.y + d, v1.z + d);
        }

        /// <summary>
        /// Overload the subtraction operator for Vector3D objects.
        /// </summary>
        /// <param name="v1">Left hand vector.</param>
        /// <param name="v2">Right hand vector</param>
        /// <returns>The difference of the two input vectors.</returns>
        public static Vector3D operator -(Vector3D v1, Vector3D v2)
        {
            return new Vector3D(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
        }

        /// <summary>
        /// Overlaod multiplication on the left by a scalar.
        /// </summary>
        /// <param name="r">Scalar value to apply to the vector.</param>
        /// <param name="v">Vector to be scaled.</param>
        /// <returns>Vector pointing in the same direction as v but with r times the length.</returns>
        public static Vector3D operator *(double r, Vector3D v)
        {
            return new Vector3D(v.x * r, v.y * r, v.z * r);
        }

        /// <summary>
        /// Overlaod multiplication on the right by a scalar.
        /// </summary>
        /// <param name="r">Scalar value to apply to the vector.</param>
        /// <param name="v">Vector to be scaled.</param>
        /// <returns>Vector pointing in the same direction as v but with r times the length.</returns>
        public static Vector3D operator *(Vector3D v, double r)
        {
            return new Vector3D(v.x * r, v.y * r, v.z * r);
        }

        /// <summary>
        /// Return the component-wise floor of the specified Vector3D object.
        /// </summary>
        /// <param name="v">Vector3D object to operate on.</param>
        /// <returns>Vector3D object whose components represent the greatest integer less than or equal to the components of the original object.</returns>
        public static Vector3D Floor(Vector3D v)
        {
            return new Vector3D(Math.Floor(v.x), Math.Floor(v.y), Math.Floor(v.z));
        }

        /// <summary>
        /// Static method that returns a normalized vector.
        /// </summary>
        /// <param name="v">Vector to normalize.</param>
        /// <returns>A vector pointing in the same direction as v but with unit length.</returns>
        public static Vector3D Normalize(Vector3D v)
        {
            double l = v.Length();
            return new Vector3D(v.x / l, v.y / l, v.z / l);
        }

        /// <summary>
        /// Compute the dot product of two vectors.
        /// </summary>
        /// <param name="v1">Vector 1</param>
        /// <param name="v2">Vector 2</param>
        /// <returns>Scalar product (Dot product) of v1 and v2.</returns>
        public static double Dot(Vector3D v1, Vector3D v2)
        {
            return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
        }

        /// <summary>
        /// Compute the cross product of two vectors.
        /// </summary>
        /// <param name="v1">Left hand vector.</param>
        /// <param name="v2">Right hand vector.</param>
        /// <returns>A vector perpendicular to the two input vectors (according to the right-hand-rule) with length equal to the area of the parallelogram defined by the two input vectors.</returns>
        public static Vector3D Cross(Vector3D v1, Vector3D v2)
        {
            return new Vector3D(v1.y * v2.z - v2.y * v1.z, v1.z * v2.x - v2.z * v1.x, v1.x * v2.y - v2.x * v1.y);
        }

        /// <summary>
        /// For homogeneity with the Vector3 class a TransformCoordinate() method is included. This method returns m * v using the multiplication operator (v is treated as a column vector).
        /// </summary>
        /// <param name="v">Vector to be transformed.</param>
        /// <param name="m">Matrix containing the transformation.</param>
        /// <returns>The transformed vector.</returns>
        public static Vector3D TransformCoordinate(Vector3D v, MatrixD m)
        {
            return m * v;
        }

        #endregion

        #region Instance Members

        /// <summary>
        /// Default argument-free constructor.
        /// </summary>
        public Vector3D()
        {
        }

        /// <summary>
        /// Construct a double-precision 3-D vector from three coordiantes.
        /// </summary>
        /// <param name="x">X cordinate</param>
        /// <param name="y">Y coordiante</param>
        /// <param name="z">Z coordinate</param>
        public Vector3D(double x, double y, double z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        /// <summary>
        /// Returns the length of a 3-D vector.
        /// </summary>
        /// <returns>Length of the vector.</returns>
        public double Length()
        {
            return Math.Sqrt(x * x + y * y + z * z);
        }

        /// <summary>
        /// Returns the square of the length of a 3-D vector.
        /// </summary>
        /// <returns>Square of the length of the vector.</returns>
        public double LengthSq()
        {
            return x * x + y * y + z * z;
        }

        /// <summary>
        /// Normalize this vector.
        /// </summary>
        public void Normalize()
        {
            double l = Length();
            x /= l;
            y /= l;
            z /= l;
        }

        /// <summary>
        /// Returns the theta angle in spherical coordinates of this cartesian vector.
        /// </summary>
        /// <returns>Theta angle in spherical coordinates.</returns>
        public double SphericalTheta()
        {
            return Math.Atan2(Math.Sqrt(x * x + y * y), z);
        }

        /// <summary>
        /// Returns the phi angle in spherical coordinates of this cartesian vector.
        /// </summary>
        /// <returns>Theta phi in spherical coordinates.</returns>
        public double SphericalPhi()
        {
            return Math.Atan2(y, x);
        }

        /// <summary>
        /// Uses the formula (x >= s) ? 1 : 0 on each component and returns the resulting vector.
        /// </summary>
        /// <param name="s">Scalar value to test against.</param>
        /// <returns>Vector3D object representing the results of the comparisons.</returns>
        public Vector3D Step(double s)
        {
            return new Vector3D(x >= s ? 1 : 0, y >= s ? 1 : 0, z >= s ? 1 : 0);
        }

        /// <summary>
        /// Transform this vector according to the specified matrix.
        /// </summary>
        /// <param name="m">Matrix used to transform this matrix.</param>
        public void TransformCoordinate(MatrixD m)
        {
            Vector3D trans = m * this;
            x = trans.x;
            y = trans.y;
            z = trans.z;
        }

        /// <summary>
        /// Create a deep-copy of this object.
        /// </summary>
        /// <returns>A new Vector3D object that has the same coordiantes as this one.</returns>
        public Vector3D Copy()
        {
            return new Vector3D(x, y, z);
        }

        #endregion
    }
}

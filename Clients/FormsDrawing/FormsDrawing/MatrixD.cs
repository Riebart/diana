using System;
using System.Collections.Generic;
using System.Text;

namespace FormsDrawing
{
    /// <summary>
    /// Provides a double-precision analogue of all 
    /// </summary>
    public class MatrixD
    {
        double[,] matrix;

        /// <summary>
        /// Return a copy of the identity matrix.
        /// </summary>
        public static MatrixD Identity
        {
            get
            {
                MatrixD ret = new MatrixD();
                ret.matrix = new double[,] { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
                return ret;
            }
        }

        /// <summary>
        /// Compute the Householdre reflection (reflection along an axis). Source: http://en.wikipedia.org/wiki/Householder_transformation
        /// </summary>
        /// <param name="x">X-coordinate of the axis to reflect through.</param>
        /// <param name="y">Y-coordinate of the axis to reflect through.</param>
        /// <param name="z">Z-coordinate of the axis to reflect through.</param>
        /// <returns>4x4 matrix performing the desired reflection.</returns>
        public static MatrixD HouseholderReflection(double x, double y, double z)
        {
            // The axis of reflection must be a unit vector, so normalize it.
            double l = Math.Sqrt(x * x + y * y + z * z);
            x /= l;
            y /= l;
            z /= l;

            MatrixD ret = new MatrixD();

            ret.matrix = new double[,] { { 1 - 2 * x * x, -2 * x * y, -2 * x * z, 0 }, 
										 { -2 * x * x, 1 - 2 * y * y, -2 * y * z, 0 }, 
										 { -2 * x * z, -2 * y * z, 1 - 2 * z * z, 0 }, 
										 { 0, 0, 0, 1 } };

            return ret;
        }

        /// <summary>
        /// Compute the Householdre reflection (reflection along an axis). Source: http://en.wikipedia.org/wiki/Householder_transformation
        /// </summary>
        /// <param name="axis">Axis to reflecting through.</param>
        /// <returns>4x4 matrix performing the desired reflection.</returns>
        public static MatrixD HouseholderReflection(Vector3D axis)
        {
            return HouseholderReflection(axis.X, axis.Y, axis.Z);
        }

        /// <summary>
        /// Constructs the left-hand view matrix given a camera's properties. See http://msdn.microsoft.com/en-us/library/microsoft.windowsmobile.directx.matrix.lookatlh.aspx for details.
        /// </summary>
        /// <param name="position">Camera position used in translation.</param>
        /// <param name="target">Camera's look target.</param>
        /// <param name="up">A vector representing the up of the current world.</param>
        /// <returns></returns>
        public static MatrixD LookAtLH(Vector3D position, Vector3D target, Vector3D up)
        {
            Vector3D z = Vector3D.Normalize(target - position);
            Vector3D x = Vector3D.Normalize(Vector3D.Cross(up, z));
            Vector3D y = Vector3D.Cross(z, x);

            MatrixD ret = new MatrixD();

            ret.matrix = new double[,] { { x.X, y.X, z.X, 0 }, 
										 { x.Y, y.Y, z.Y, 0 }, 
										 { x.Z, y.Z, z.Z, 0 }, 
										 { -Vector3D.Dot(x, position), -Vector3D.Dot(y, position), -Vector3D.Dot(z, position), 1 } };

            return ret;
        }

        /// <summary>
        /// Compute the perspective projection matrix. See http://msdn.microsoft.com/en-us/library/microsoft.windowsmobile.directx.matrix.perspectivefovlh.aspx and http://knol.google.com/k/koen-samyn/perspective-transformation/2lijysgth48w1/19# for more information.
        /// </summary>
        /// <param name="fovY">Field of view in the vertical screen direction in radians.</param>
        /// <param name="aspectRatio">Aspect ratio defined as the view-space width divided by height.</param>
        /// <param name="nearPlane">How close is the near clipping plane.</param>
        /// <param name="farPlane">How far is the far clipping plane.</param>
        /// <returns></returns>
        public static MatrixD PerspectiveFovLH(double fovY, double aspectRatio, double nearPlane, double farPlane)
        {
            MatrixD ret = new MatrixD();

            ret.matrix = new double[,] { { 1 / (aspectRatio * Math.Tan(fovY / 2)), 0, 0, 0 }, { 0, 1 / Math.Tan(fovY / 2), 0, 0 }, { 0, 0, farPlane / (farPlane - nearPlane), 1 }, { 0, 0, -nearPlane * farPlane / (farPlane - nearPlane), 0 } };

            return ret;
        }

        /// <summary>
        /// Create a double-precision matrix that will rotate a column vector in homogeneous coordinates about the X axis.
        /// </summary>
        /// <param name="t">Angle to rotate by</param>
        /// <returns>4x4 matrix performing the desired rotation.</returns>
        public static MatrixD RotationX(double t)
        {
            MatrixD ret = new MatrixD();
            double c = Math.Cos(t);
            double s = Math.Sin(t);

            ret.matrix = new double[,] { { 1, 0, 0, 0 }, 
										 { 0, c, -s, 0 }, 
										 { 0, s, c, 0 }, 
										 { 0, 0, 0, 1 } };

            return ret;
        }

        /// <summary>
        /// Create a double-precision matrix that will rotate a column vector in homogeneous coordinates about the Y axis.
        /// </summary>
        /// <param name="t">Angle to rotate by</param>
        /// <returns>4x4 matrix performing the desired rotation.</returns>
        public static MatrixD RotationY(double t)
        {
            MatrixD ret = new MatrixD();
            double c = Math.Cos(t);
            double s = Math.Sin(t);

            ret.matrix = new double[,] { { c, 0, s, 0 }, 
										 { 0, 1, 0, 0 }, 
										 { -s, 0, c, 0 }, 
										 { 0, 0, 0, 1 } };

            return ret;
        }

        /// <summary>
        /// Create a double-precision matrix that will rotate a column vector in homogeneous coordinates about the Z axis.
        /// </summary>
        /// <param name="t">Angle to rotate by</param>
        /// <returns>4x4 matrix performing the desired rotation.</returns>
        public static MatrixD RotationZ(double t)
        {
            MatrixD ret = new MatrixD();
            double c = Math.Cos(t);
            double s = Math.Sin(t);

            ret.matrix = new double[,] { { c, -s, 0, 0 }, 
										 { s, c, 0, 0 }, 
										 { 0, 0, 1, 0 }, 
										 { 0, 0, 0, 1 } };

            return ret;
        }

        /// <summary>
        /// Compute a matrix that rotates around a given vector by a given angle. Source: http://inside.mines.edu/~gmurray/ArbitraryAxisRotation/
        /// </summary>
        /// <param name="x">X-coordiante of axis vector</param>
        /// <param name="y">Y-coordiante of axis vector</param>
        /// <param name="z">Z-coordiante of axis vector</param>
        /// <param name="t">Angle to rotate through. Clockwise oriented while looking down the axis towards the origin.</param>
        /// <returns></returns>
        public static MatrixD RotationAxis(double x, double y, double z, double t)
        {
            MatrixD ret = new MatrixD();
            double c = Math.Cos(t);
            double s = Math.Sin(t);
            double l = Math.Sqrt(x * x + y * y + z * z);
            double l2 = x * x + y * y + z * z;

            ret.matrix = new double[,] { { (x * x + (y * y + z * z) * c) / l2, (x * y - x * y * c) / l2 + z * s / l, (x * z - x * z * c) / l2 - y * s / l, 0 }, 
										 { (x * y - x * y * c) / l2 - z * s / l, (y * y + (x * x + z * z) * c) / l2, (y * z - y * z * c) / l2 + x * s / l, 0 }, 
										 { (x * z - x * z * c) / l2 + y * s / l, (y * z - y * z * c) / l2 - x * s / l, (z * z + (x * x + y * y) * c) / l2, 0 }, 
										 { 0, 0, 0, 1 } };

            return ret;
        }

        /// <summary>
        /// Compute a matrix that rotates around a given vector by a given angle. Source: http://inside.mines.edu/~gmurray/ArbitraryAxisRotation/
        /// </summary>
        /// <param name="v">Axis vector</param>
        /// <param name="t">Angle to rotate through. Clockwise oriented while looking down the axis towards the origin.</param>
        /// <returns></returns>
        public static MatrixD RotationAxis(Vector3D v, double t)
        {
            return RotationAxis(v.X, v.Y, v.Z, t);
        }

        /// <summary>
        /// Creates a matrix that translates a vector by another given vector.
        /// </summary>
        /// <param name="x">X-coordinate of vector to translate by</param>
        /// <param name="y">Y-coordinate of vector to translate by</param>
        /// <param name="z">Z-coordinate of vector to translate by</param>
        /// <returns>matrix that when multiplied on the right by a column, translates that column vector by v.</returns>
        public static MatrixD Translation(double x, double y, double z)
        {
            MatrixD ret = new MatrixD();

            ret.matrix = new double[,] { { 1, 0, 0, 0 },
										 { 0, 1, 0, 0 },
										 { 0, 0, 1, 0 },
										 { x, y, z, 1 } };

            return ret;
        }

        /// <summary>
        /// Creates a matrix that translates a vector by another given vector.
        /// </summary>
        /// <param name="v">Vector to translate by</param>
        /// <returns>matrix that when multiplied on the right by a column, translates that column vector by v.</returns>
        public static MatrixD Translation(Vector3D v)
        {
            return Translation(v.X, v.Y, v.Z);
        }

        /// <summary>
        /// Allow addition of two double-precision matrices.
        /// </summary>
        /// <param name="m1">Left hand matrix</param>
        /// <param name="m2">Rigth hand matrix</param>
        /// <returns>Sum of the two input matrices.</returns>
        public static MatrixD operator +(MatrixD m1, MatrixD m2)
        {
            MatrixD ret = new MatrixD();
            ret.matrix = new double[4, 4];

            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++)
                    ret.matrix[i, j] = m1.matrix[i, j] + m2.matrix[i, j];

            return ret;
        }

        /// <summary>
        /// Transform the specified vector by the transformation in the matrix. The Vector3D object is projected to w=1, transformed, and projected back.
        /// </summary>
        /// <param name="m">4x4 homogeneous transformation matrix.</param>
        /// <param name="v">3-D vector to be transformed.</param>
        /// <returns>3-D vector with the transforms contains in the matrix applied to v.</returns>
        public static Vector3D operator *(MatrixD m, Vector3D v)
        {
            return new Vector3D(v.X * m.matrix[0, 0] + v.Y * m.matrix[0, 1] + v.Z * m.matrix[0, 2] + m.matrix[0, 3],
                                v.X * m.matrix[1, 0] + v.Y * m.matrix[1, 1] + v.Z * m.matrix[1, 2] + m.matrix[1, 3],
                                v.X * m.matrix[2, 0] + v.Y * m.matrix[2, 1] + v.Z * m.matrix[2, 2] + m.matrix[2, 3]);
        }

        /// <summary>
        /// Multiplies two double-precision matrices together.
        /// </summary>
        /// <param name="m1">Matrix on left</param>
        /// <param name="m2">Matrix on right</param>
        /// <returns>The product of m1*m2</returns>
        public static MatrixD operator *(MatrixD m1, MatrixD m2)
        {
            MatrixD ret = new MatrixD();
            ret.matrix = new double[4, 4];

            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++)
                    ret.matrix[i, j] = m1.matrix[i, 0] * m2.matrix[0, j] +
                                       m1.matrix[i, 1] * m2.matrix[1, j] +
                                       m1.matrix[i, 2] * m2.matrix[2, j] +
                                       m1.matrix[i, 3] * m2.matrix[3, j];

            return ret;
        }
    }
}

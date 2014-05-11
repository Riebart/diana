#!/usr/bin/env python

from math import sin, cos, pi, sqrt
from protocols.universe_pb2 import VectorMsg

class Vector3:
    def __init__(self, v, y = None, z = None):
        if isinstance(v, VectorMsg):
            self.x = v.x
            self.y = v.y
            self.z = v.z
        else:        
            if y == None:
                self.x = v[0]
                self.y = v[1]
                self.z = v[2]
            else:
                self.x = v
                self.y = y
                self.z = z

    def clone(self):
        return Vector3(self.x, self.y, self.z)

    #def __init__(self, x, y, z):
        #self.x = x
        #self.y = y
        #self.z = z

    def length(self):
        r = self.x * self.x + self.y * self.y + self.z * self.z
        return sqrt(r)

    def length2(self):
        r = self.x * self.x + self.y * self.y + self.z * self.z
        return r

    def dist(self, v):
        x = v.x - self.x
        y = v.y - self.y
        z = v.z - self.z
        return sqrt(x * x + y * y + z * z)

    def dist2(self, v):
        x = v.x - self.x
        y = v.y - self.y
        z = v.z - self.z
        return x * x + y * y + z * z

    # Returns a unit vector that originates at v and goes to this vector.
    def ray(self, v):
        x = v.x - self.x
        y = v.y - self.y
        z = v.z - self.z
        m = sqrt(x * x + y * y + z * z)
        return Vector3([x / m, y / m, z / m])

    def unit(self):
        l = self.length()
        if Vector3.almost_zeroS(l):
            return Vector3([0,0,0])
        else:
            return Vector3([self.x / l, self.y / l, self.z / l])

    # In this case, self is the first vector in the cross product, because order
    # matters here
    def cross(self, v):
        rx = self.y * v.z - self.z * v.y
        ry = self.z * v.x - self.x * v.z
        rz = self.x * v.y - self.y * v.x
        return Vector3([rx, ry, rz])

    # Drag self down v until they dot to zero. Assumes v is normalized.
    def project_down_n(self, v):
        s = self.dot(v)
        return Vector3.combine([[1, self], [-s, v]])

    @staticmethod
    # Combines a linear combination of a bunch of vectors
    def combine(vecs):
        rx = 0.0
        ry = 0.0
        rz = 0.0

        for v in vecs:
            rx += v[0] * v[1].x
            ry += v[0] * v[1].y
            rz += v[0] * v[1].z

        return Vector3([rx, ry, rz])

    def normalize(self):
        if self.almost_zero():
            return

        l = self.length()
        self.x /= l
        self.y /= l
        self.z /= l

    def scale(self, c):
        self.x *= c
        self.y *= c
        self.z *= c

    # Adds v to self.
    def add(self, v, s = 1):
        self.x += s * v.x
        self.y += s * v.y
        self.z += s * v.z

    #override +
    def __add__(self, other):
        return Vector3([self.x+other.x, self.y+other.y, self.z+other.z])

    def sub(self, v):
        self.x -= v.x
        self.y -= v.y
        self.z -= v.z

    #override -
    def __sub__(self, other):
        return Vector3([self.x-other.x, self.y-other.y, self.z-other.z])

    def dot(self, v):
        return self.x * v.x + self.y * v.y + self.z * v.z

    @staticmethod
    def almost_zeroS(v):
        # Python doesn't seem to be able to distinguish exponents below -300,
        # So we'll cut off at -150
        if -1e-150 < v and v < 1e-150:
            return 1
        else:
            return 0

    def almost_zero(self):
        if Vector3.almost_zeroS(self.x) and Vector3.almost_zeroS(self.y) and Vector3.almost_zeroS(self.z):
            return 1
        else:
            return 0

    #overrid []
    def __getitem__(self, index):
        if index == 0:
            return self.x
        if index == 1:
            return self.y
        if index == 2:
            return self.z

        raise IndexError('Vector3 has only 3 dimensions')

    def __setitem__(self, index, value):
        if index == 0:
            self.x = value
        elif index == 1:
            self.y = value
        elif index == 2:
            self.z = value
        else:
            raise IndexError('Vector3 has only 3 dimensions')

    def __repr__(self):
        return "<%f, %f, %f>" % (self.x, self.y, self.z)

zero3d = Vector3(0, 0, 0)
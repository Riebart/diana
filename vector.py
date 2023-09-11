from __future__ import annotations
#!/usr/bin/env python
from math import sin, cos, pi, sqrt, pow, atan2

num = int|float

class Vector3:
    x: num
    y: num
    z: num

    @staticmethod
    def easy_look_at(look: Vector3) -> list[Vector3]:
        # produce some arbitrary up and right vectors for a given look-at vector.
        # Do this by finding a vector that dots to zero with the look vector,
        # then just cross for the right vector.

        up = Vector3([-look.z, 0, look.x])
        right = Vector3.cross(up, look)

        return [ look, up, right ]

    @staticmethod
    def easy_look_at2(forward: Vector3, up: Vector3, right: Vector3, look: Vector3) -> None|list[Vector3]:
        # Produce a new set of forward, up, right vectors build from an existing
        # set, and a new look direction. Do this by taking the cross-product of
        # of the difference between forward, and the new look, then crossing again
        # to get the up.
        diff = look - forward

        if diff.almost_zero():
            return None

        # Now get the new right vector by crossing the old look with the diff
        right = Vector3.cross(diff, look)

        # Up comes from crossing the new right and the look vectors
        up = Vector3.cross(look, right)

        return [ look, up, right ]

    @staticmethod
    def look_at(forward: Vector3, up: Vector3, right: Vector3, look: Vector3) -> None|Vector3:
        # Rotate an existing frame of reference to point along a new direction.
        # Do this by finding the axis of most efficient rotation that would
        # produce the new look vector, then rotate the up and right vectors
        # around that axis, by the same amount.
        diff = look - forward

        if diff.almost_zero():
            return None

        # Now get the axis of rotation by crossing the diff and look vectors.
        axis = Vector3.cross(diff, forward)

        # Now rotate the up and right vectors
        ### TODO ### FINISH THIS
        return None

    @staticmethod
    def get_orientation(forward: Vector3, up: Vector3, right: Vector3) -> Vector4:
        # Turn the three vectors in a four-tuple that uniquely defines then
        # orientation basis vectors, assuming they are unit vectors.
        return Vector4([ forward.x, forward.y, up.x, up.y ])

    @staticmethod
    def from_orientation(o:list[int]) -> list[Vector3]:
        # Build the three orientation basis vectors from the orientation
        # 4-tuple
        forward = Vector3([o[0], o[1], sqrt(1 - o[0] * o[0] - o[1] * o[1])])
        up = Vector3([o[2], o[3], sqrt(1 - o[2] * o[2] - o[3] * o[3])])
        right = Vector3.cross(up, forward)

        return [ forward, up, right ]

    def rotate_aroundV(self, axis: Vector3, angle: num ) -> None:
        # Rotate a vector around an axis, by an angle in radians.
        self.rotate_around(axis.x, axis.y, axis.z, angle)

    # ### TODO ### Apply the Euler-Rodrigues forumla here instead.
    # http://stackoverflow.com/questions/6802577/python-rotation-of-3d-vector
    def rotate_around(self, x:num, y:num, z:num , angle:num) -> None:
        if Vector3.almost_zeroS(angle):
            return

        c = cos(angle)
        s = sin(angle)
        l2 = x * x + y * y + z * z
        l = sqrt(l2)

        if Vector3.almost_zeroS(l2):
            return

        x2 = self.y*((x*y-c*x*y)/l2+(s*z)/l)+(self.x*(pow(x,2)+c*(pow(y,2)+pow(z,2))))/l2+(-((s*y)/l)+(x*z-c*x*z)/l2)*self.z
        y2 = self.x*((x*y-c*x*y)/l2-(s*z)/l)+(self.y*(pow(y,2)+c*(pow(x,2)+pow(z,2))))/l2+((s*x)/l+(y*z-c*y*z)/l2)*self.z
        z2 = self.x*((s*y)/l+(x*z-c*x*z)/l2)+self.y*(-((s*x)/l)+(y*z-c*y*z)/l2)+((c*(pow(x,2)+pow(y,2))+pow(z,2))*self.z)/l2

        self.x = x2
        self.y = y2
        self.z = z2

    @staticmethod
    def apply_ypr(forward:Vector3, up:Vector3, right:Vector3, angles:list[num]) -> None:
        # Apply yaw, pitch, and roll.
        # Order matters here, and changing the order changes the result.
        forward.rotate_aroundV(up, angles[0])
        right.rotate_aroundV(up, angles[0])

        forward.rotate_aroundV(right, angles[1])
        up.rotate_aroundV(right, angles[1])

        right.rotate_aroundV(forward, angles[2])
        up.rotate_aroundV(forward, angles[2])

    def __init__(self, v:list[num]|tuple(num,num,num)|num|Vector3, y:num|None = None, z:num|None = None) -> None:
        if isinstance(v, tuple) or isinstance(v, list) or isinstance(v, Vector3):
            self.x = v[0]
            self.y = v[1]
            self.z = v[2]
        else:
            self.x = v
            self.y = y
            self.z = z


    #def __init__(self, x, y, z):
        #self.x = x
        #self.y = y
        #self.z = z

    def length(self) -> float:
        r = self.x * self.x + self.y * self.y + self.z * self.z
        return sqrt(r)

    def length2(self) -> float:
        r = self.x * self.x + self.y * self.y + self.z * self.z
        return r

    def dist(self, v:Vector3) -> float:
        x = v.x - self.x
        y = v.y - self.y
        z = v.z - self.z
        return sqrt(x * x + y * y + z * z)

    def dist2(self, v:Vector3) -> float:
        x = v.x - self.x
        y = v.y - self.y
        z = v.z - self.z
        return x * x + y * y + z * z

    # Returns a unit vector that originates at v and goes to this vector.
    def ray(self, v:Vector3) -> Vector3:
        x = v.x - self.x
        y = v.y - self.y
        z = v.z - self.z
        m = sqrt(x * x + y * y + z * z)
        return Vector3([x / m, y / m, z / m])

    def unit(self) -> Vector3:
        l = self.length()
        if Vector3.almost_zeroS(l):
            return Vector3([0,0,0])
        else:
            return Vector3([self.x / l, self.y / l, self.z / l])

    # In this case, self is the first vector in the cross product, because order
    # matters here
    def cross(self, v:Vector3) -> Vector3:
        rx = self.y * v.z - self.z * v.y
        ry = self.z * v.x - self.x * v.z
        rz = self.x * v.y - self.y * v.x
        return Vector3([rx, ry, rz])

    # Drag self down v until they dot to zero. Assumes v is normalized.
    def project_down_n(self, v:Vector3) -> Vector3:
        s = self.dot(v)
        return Vector3.combine([(1, self), (-s, v)])

    @staticmethod
    # Combines a linear combination of a bunch of vectors
    def combine(vecs:list[tuple[num, Vector3]]) -> Vector3:
        rx = 0.0
        ry = 0.0
        rz = 0.0

        for v in vecs:
            rx += v[0] * v[1].x
            ry += v[0] * v[1].y
            rz += v[0] * v[1].z

        return Vector3([rx, ry, rz])

    def normalize(self) -> None:
        if self.almost_zero():
            return

        l = self.length()
        self.x /= l
        self.y /= l
        self.z /= l

    def scale(self, c:num) -> None:
        self.x *= c
        self.y *= c
        self.z *= c

    # Adds v to self.
    def add(self, v:Vector3, s:num = 1) -> None:
        self.x += s * v.x
        self.y += s * v.y
        self.z += s * v.z

    #override +
    def __add__(self, other:Vector3) -> Vector3:
        return Vector3([self.x+other.x, self.y+other.y, self.z+other.z])

    def sub(self, v:Vector3) -> None:
        self.x -= v.x
        self.y -= v.y
        self.z -= v.z

    #override -
    def __sub__(self, other: Vector3) -> Vector3:
        return Vector3([self.x-other.x, self.y-other.y, self.z-other.z])

    def dot(self, v:Vector3) -> float:
        return self.x * v.x + self.y * v.y + self.z * v.z

    @staticmethod
    def almost_zeroS(v:num) -> bool:
        # Python doesn't seem to be able to distinguish exponents below -300,
        # So we'll cut off at -150
        if -1e-150 < v and v < 1e-150:
            return True
        else:
            return False

    def almost_zero(self) -> bool:
        if Vector3.almost_zeroS(self.x) and Vector3.almost_zeroS(self.y) and Vector3.almost_zeroS(self.z):
            return True
        else:
            return False

    #overrid []
    def __getitem__(self, index:int) -> num:
        if index == 0:
            return self.x
        if index == 1:
            return self.y
        if index == 2:
            return self.z

        raise IndexError('Vector3 has only 3 dimensions')

    def __setitem__(self, index:int, value:num) -> None:
        if index == 0:
            self.x = value
        elif index == 1:
            self.y = value
        elif index == 2:
            self.z = value
        else:
            raise IndexError('Vector3 has only 3 dimensions')

    def __repr__(self) -> str:
        return "<%f, %f, %f>" % (self.x, self.y, self.z)

zero3d = Vector3(0, 0, 0)


#for now, just inherit Vector3's methods to fill it out
class Vector4(Vector3):
    w:num
    x:num
    y:num
    z:num

    def __init__(self, v:num|list[num], x:num|None = None, y:num|None = None, z:num|None = None) -> None:
        if isinstance(v, list):
            self.w = v[0]
            self.x = v[1]
            self.y = v[2]
            self.z = v[3]
        else:
            self.w = v
            self.x = x
            self.y = y
            self.z = z
            
            
    def clone(self) -> Vector4:
        return Vector4(self.w, self.x, self.y, self.z)
    
    # Adds v to self.
    def add(self, v:Vector4, s:num = 1) -> None: # type: ignore[override]
        self.w += s * v.w
        self.x += s * v.x
        self.y += s * v.y
        self.z += s * v.z            
        
    #override -
    def __sub__(self, other:Vector4) -> Vector4: # type: ignore[override]
        return Vector4([self.w-other.w,  self.x-other.x, self.y-other.y, self.z-other.z])        
    
    #override []
    def __getitem__(self, index:int) -> num:
        if index == 0:
            return self.w
        if index == 1:
            return self.x
        if index == 2:
            return self.y
        if index == 3:
            return self.z

        raise IndexError('Vector4 has only 4 dimensions')
    
    def __setitem__(self, index:int, value:num) -> None:
        if index == 0:
            self.w = value
        elif index == 1:
            self.x = value
        elif index == 2:
            self.y = value
        elif index == 3:
            self.z = value
        else:
            raise IndexError('Vector3 has only 3 dimensions')
        
    def __repr__(self) -> str:
        return "<%f, %f, %f, %f>" % (self.w, self.x, self.y, self.z)    

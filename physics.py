#!/usr/bin/env python

from math import sin, cos, pi, sqrt
from mimosrv import MIMOServer, send, receive

class Vector3:
    def __init__(self, v):
        self.x = v[0]
        self.y = v[1]
        self.z = v[2]

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

    def scale(self, c):
        self.x *= c
        self.y *= c
        self.z *= c

    # Adds v to self.
    def add(self, v):
        self.x += v.x
        self.y += v.y
        self.z += v.z

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

class PhysicsObject:
    def __init__(self, position = [ 0.0, 0.0, 0 ],
                        velocity = [ 0.0, 0.0, 0 ],
                        orientation = [ 0.0, 0.0, 0.0 ],
                        mass = 10.0,
                        radius = 1.0,
                        thrust = 0.0):
        self.position = Vector3(position)
        self.velocity = Vector3(velocity)
        self.orientation = Vector3(orientation)
        self.mass = mass
        self.radius = radius
        self.thrust = thrust

#just to distinguish between objects which impart significant gravity, and those that do not (for now)
class GravitationalBody(PhysicsObject):
    pass

class PhysShip(PhysicsObject):
    def __init__(self, uni, client):
        PhysicsObject.__init__(self)
        self.uni = uni
        self.client = client
        self.thrust = 0.0

    def handle(self):
        # The other end is trying to say something we weren't expecting.
        data = receive(self.client)

        if not data:
            self.uni.net.hangup(self.client)
            
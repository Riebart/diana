#!/usr/bin/env python

import sys
from math import sin, cos, pi, sqrt
from mimosrv import MIMOServer, send, receive
from message import Message, HelloMsg, PhysicalPropertiesMsg

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
    def __init__(self, universe,
                    position = [ 0.0, 0.0, 0 ],
                    velocity = [ 0.0, 0.0, 0 ],
                    orientation = [ 0.0, 0.0, 0.0 ],
                    mass = 10.0,
                    radius = 1.0,
                    thrust = [0.0, 0.0, 0.0]):
        self.phys_id = universe.get_id()
        self.position = Vector3(position)
        self.velocity = Vector3(velocity)
        self.orientation = Vector3(orientation)
        self.mass = mass
        self.radius = radius
        self.thrust = Vector3(thrust)

#just to distinguish between objects which impart significant gravity, and those that do not (for now)
class GravitationalBody(PhysicsObject):
    pass

class PhysShip(PhysicsObject):
    def __init__(self, universe, client,
                    position = [ 0.0, 0.0, 0 ],
                    velocity = [ 0.0, 0.0, 0 ],
                    orientation = [ 0.0, 0.0, 0.0 ],
                    mass = 10.0,
                    radius = 1.0,
                    thrust = [0.0, 0.0, 0.0]):
        PhysicsObject.__init__(self, universe, position, velocity, orientation, mass, radius, thrust)
        self.uni = universe
        self.client = client
        self.sim_id = None


    def handle(self, client):
        msg = Message.get_message(client)

        if isinstance(msg, HelloMsg):
            self.sim_id = msg.endpoint_id
            HelloMsg.send(client, self.phys_id)
        elif isinstance(msg, PhysicalPropertiesMsg):
            if msg.mass:
                self.mass = msg.mass

            if msg.position:
                self.position = Vector3(msg.position)

            if msg.velocity:
                self.velocity = Vector3(msg.velocity)

            if msg.orientation:
                self.orientation = Vector3(msg.orientation)

            if msg.thrust:
                self.thrust = Vector3(msg.thrust)

            if msg.radius:
                self.radius = msg.radius

            PhysicalPropertiesMsg.send(client, [self.mass, self.position.x, self.position.y, self.position.z, self.velocity.x, self.velocity.y, self.velocity.z, self.orientation.x, self.orientation.y, self.orientation.z, self.thrust.x, self.thrust.y, self.thrust.z, self.radius ])

        #data = self.client.recv(1024)
        #data = self.client.readline()
        
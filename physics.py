#!/usr/bin/env python

import sys
from math import sin, cos, pi, sqrt
from mimosrv import MIMOServer
from message import Message
from message import HelloMsg, PhysicalPropertiesMsg, VisualPropertiesMsg
from message import VisualDataEnableMsg, VisualMetaDataEnableMsg
from message import BeamMsg

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
        self.art_id = None

#just to distinguish between objects which impart significant gravity, and those that do not (for now)
class GravitationalBody(PhysicsObject):
    pass

class SmartPhysicsObject(PhysicsObject):
    def __init__(self, universe, client,
                    position = [ 0.0, 0.0, 0 ],
                    velocity = [ 0.0, 0.0, 0 ],
                    orientation = [ 0.0, 0.0, 0.0 ],
                    mass = 10.0,
                    radius = 1.0,
                    thrust = [0.0, 0.0, 0.0]):
        PhysicsObject.__init__(self, universe, position, velocity, orientation, mass, radius, thrust)
        self.client = client
        self.universe = universe
        self.sim_id = None
        self.vis_data = 0
        self.vis_meta_data = 0

    def handle(self, client):
        msg = Message.get_message(client)

        if isinstance(msg, HelloMsg):
            if msg.endpoint_id == None:
                return

            self.sim_id = msg.endpoint_id
            HelloMsg.send(client, self.phys_id)

        # If we don't have a sim_id by this point, we can't accept any of the
        # following in good conscience...
        if self.sim_id == None:
            return

        if isinstance(msg, PhysicalPropertiesMsg):
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

            #PhysicalPropertiesMsg.send(f, [self.mass, self.position.x, self.position.y, self.position.z, self.velocity.x, self.velocity.y, self.velocity.z, self.orientation.x, self.orientation.y, self.orientation.z, self.thrust.x, self.thrust.y, self.thrust.z, self.radius ])

        elif isinstance(msg, VisualPropertiesMsg):
            if self.art_id == None:
                self.art_id = self.universe.curator.register_art(msg.mesh, msg.texture)
            else:
                self.universe.curator.update_art(self.art_id, msg.mesh, msg.texture)
                self.universe.curator.attach_art_asset(self.art_id, self.phys_id)

        elif isinstance(msg, VisualDataEnableMsg):
            changed = msg.enabled - self.vis_data
            self.vis_data = msg.enabled

            if changed != 0:
                self.universe.register_for_vis_data(self, self.vis_data)

        elif isinstance(msg, VisualMetaDataEnableMsg):
            changed = msg.enabled - self.vis_meta_data
            self.vis_meta_data = msg.enabled

            if changed != 0:
                self.universe.curator.register_client(self, self.vis_meta_data)

        elif isinstance(msg, BeamMsg):
            pass

class Beam:
    def __init__(self, universe, beam_type,
                    origin, normals,
                    pri_range, falloff,
                    max_damage = 0):
        self.universe = universe
        self.beam_type = beam_type
        self.origin = origin
        self.normals = normals
        self.pri_range = pri_range
        self.falloff = falloff
        self.max_damage = max_damage

    def handle(self, client):
        msg = Message.get_message(client)

        # We only accept one kind of message here: a BEAM message.

        if isinstance(msg, HelloMsg):
            if msg.endpoint_id == None:
                return

            self.sim_id = msg.endpoint_id
            HelloMsg.send(f, self.phys_id)

        # If we don't have a sim_id by this point, we can't accept any of the
        # following in good conscience...
        if self.sim_id == None:
            return

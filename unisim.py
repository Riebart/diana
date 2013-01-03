#!/usr/bin/env python

import random
import time
import threading
import cPickle
import struct
from math import sin, cos, pi, sqrt
from mimosrv import MIMOServer

VERSION = 0

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

class Universe:
    class PhysicsObject:
        def __init__(self, position = [ 0.0, 0.0, 0 ],
                           velocity = [ 0.0, 0.0, 0 ],
                           orientation = [ 0.0, 0.0, 0.0 ],
                           mass = 10.0,
                           radius = 1.0):
            self.position = Vector3(position)
            self.velocity = Vector3(velocity)
            self.orientation = Vector3(orientation)
            self.mass = mass
            self.radius = radius
            
    class PhysShip(PhysicsObject):
        def __init__(self, uni, client):
            self.uni = uni
            self.client = client

        def handle(self):
            # The other end is trying to say something we weren't expecting.
            pass

    class GravitationalBody(PhysicsObject):
        pass

    def __init__(self):
        self.attractors = []
        self.phys_objects = []
        self.nonphys_objects = []
        self.ships = []
        self.phys_lock = threading.Lock()
        self.net = MIMOServer(self.register_ship, port = 5505)
        self.spawn = Vector3([0, 0, 0])

    def add_object(self, obj):
        if isinstance(obj, Universe.GravitationalBody):
            self.attractors.append(obj)
            return

        if isinstance(obj, Universe.PhysShip):
            self.ships.append(obj)

        self.phys_objects.append(obj)

    def register_ship(self, client):
        # Now we need to talk to the client.
        newship = PhysShip(self, client)
        return newship

    @staticmethod
    def gravity(big, small):
        m = 6.67384e-11 * big.mass * small.mass / big.position.dist2(small.position)
        r = small.position.ray(big.position)
        r.scale(m)
        return r

    def tick(self, dt):
        self.phys_lock.acquire()
        for o in self.phys_objects:
            # We don't consider interactions between attractors
            if isinstance(o, Universe.GravitationalBody):
                continue

            gforce = Vector3([0, 0, 0])
            if o.mass > 0:
                # First get the attraction between this object, and all of the attractors.
                for a in self.attractors:
                    gforce.add(Universe.gravity(a, o))
                if isinstance(o, Universe.PhysShip):
                    gforce.add(o.thrust)
                gforce.scale(1 / o.mass)

            # Verlet integration: http://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet
            o.position.x += o.velocity.x * dt + 0.5 * dt * dt * gforce.x
            o.position.y += o.velocity.y * dt + 0.5 * dt * dt * gforce.y
            o.position.z += o.velocity.z * dt + 0.5 * dt * dt * gforce.z

            o.velocity.x += dt * gforce.x
            o.velocity.y += dt * gforce.y
            o.velocity.z += dt * gforce.z
        self.phys_lock.release()

    # Number of real seconds and a rate of simulation.
    def sim(self, t = 1, r = 1):
        total_time = 0;
        dt = 0.01
        i = 0
        while t == 0 or total_time < r * t:
            t1 = time.clock()
            self.tick(r * dt)
            t2 = time.clock()
            # On my machine, 1.2 million clock-pairs with zero objects takes about 9.2s
            # This works out to about 8 microseconds
            dt = t2 - t1
            # if we're spinning too fast, wait up for a little bit. Cap things at 1000 FPS
            if dt < 0.001:
                go_time = t2 + 0.001
                while t2 < go_time:
                    t2 = time.clock()
                dt = t2 - t1
            total_time += r * dt
            i += 1
        return [total_time, i]
        

if __name__ == "__main__":
    rand = random.Random()
    rand.seed(0)

    uni = Universe()
    r = 1000
    t = 100

    for i in range(0, 1000):
        u = rand.random() * 2 * pi
        v = rand.random() * 2 * pi
        c = r + (rand.random() * 2 - 1) * t
        a = rand.random() * t

        obj = Universe.PhysicsObject(position = [ (c + a * cos(v)) * cos(u), (c + a * cos(v)) * sin(u), a * sin(v) ], mass = rand.random() * 75 + 25)
        uni.add_object(obj)

    r = 10000000
    t = 100000

    for i in range(0, 100):
        u = rand.random() * 2 * pi
        v = rand.random() * 2 * pi
        c = r + (rand.random() * 2 - 1) * t
        a = rand.random() * t
        
        obj = Universe.GravitationalBody(position = [ (c + a * cos(v)) * cos(u), (c + a * cos(v)) * sin(u), a * sin(v) ], mass = rand.random() * 100000 + 1e18)
        uni.add_object(obj)

    print len(uni.phys_objects)
    print len(uni.attractors)

    print uni.phys_objects[0].position.dist(uni.attractors[0].position)
    print uni.sim()
    print uni.phys_objects[0].position.dist(uni.attractors[0].position)

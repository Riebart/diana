#!/usr/bin/env python

import random
import time
import threading
from physics import Vector3, PhysicsObject, GravitationalBody, PhysShip
from math import sin, cos, pi, sqrt
from mimosrv import MIMOServer

VERSION = 0

class Universe:
    def __init__(self):
        self.attractors = []
        self.phys_objects = []
        self.nonphys_objects = []
        self.ships = [] #'ships' are all 'smart' objects that can interact with the server
        self.phys_lock = threading.Lock()
        self.net = MIMOServer(self.register_ship, port = 5505)
        self.net.start()

    def stop_net(self):
        self.net.stop()

    def add_object(self, obj):
        if isinstance(obj, GravitationalBody):
            self.attractors.append(obj)
            return

        if isinstance(obj, PhysShip):
            self.ships.append(obj)

        self.phys_objects.append(obj)

    def register_ship(self, client):
        # Now we need to talk to the client.
        newship = PhysShip(self, client)
        self.add_object(newship)
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
            if isinstance(o, GravitationalBody):
                continue

            gforce = Vector3([0, 0, 0])
            if o.mass > 0:
                # First get the attraction between this object, and all of the attractors.
                for a in self.attractors:
                    gforce.add(Universe.gravity(a, o))
                if isinstance(o, PhysShip):
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
        min_frametime = 0.001
        total_time = 0;
        dt = 0.01
        i = 0
        while t == 0 or total_time < r * t:
            t1 = time.clock()
            self.tick(r * dt)
            t2 = time.clock()
            # On my machine, 1.2 million clock-pairs with zero objects takes about 9.2s
            # This works out to about 8 microseconds per pair
            dt = t2 - t1

            # sleep to bring the frametimes down to the minimum if we're going too fast
            while dt < min_frametime:
                time.sleep(min_frametime - dt)
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

    #make 1000 random physics objects
    for i in range(0, 1000):
        u = rand.random() * 2 * pi
        v = rand.random() * 2 * pi
        c = r + (rand.random() * 2 - 1) * t
        a = rand.random() * t

        obj = PhysicsObject(position = [ (c + a * cos(v)) * cos(u), (c + a * cos(v)) * sin(u), a * sin(v) ], mass = rand.random() * 75 + 25)
        uni.add_object(obj)

    r = 10000000
    t = 100000

    #make 100 random gravitation objects
    for i in range(0, 100):
        u = rand.random() * 2 * pi
        v = rand.random() * 2 * pi
        c = r + (rand.random() * 2 - 1) * t
        a = rand.random() * t
        
        obj = GravitationalBody(position = [ (c + a * cos(v)) * cos(u), (c + a * cos(v)) * sin(u), a * sin(v) ], mass = rand.random() * 100000 + 1e18)
        uni.add_object(obj)

    print len(uni.phys_objects)
    print len(uni.attractors)

    print uni.phys_objects[0].position.dist(uni.attractors[0].position)
    print uni.sim(t=0)
    print uni.phys_objects[0].position.dist(uni.attractors[0].position)
    uni.stop_net()

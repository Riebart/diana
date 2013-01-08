#!/usr/bin/env python

import threading

from physics import Vector3, PhysicsObject, SmartPhysicsObject, Beam
from math import sin, cos, pi, sqrt
from mimosrv import MIMOServer
from message import VisualDataMsg, VisualMetaDataMsg

VERSION = 0

class ArtCurator:
    class ArtAsset:
        def __init__(self, art_id, mesh, texture):
            self.art_id = art_id
            self.mesh = mesh
            self.texture = texture

    def __init__(self, universe):
        self.universe = universe
        self.art_assets = dict()
        self.art_assignment = dict()
        self.art_asset_lock = threading.RLock()
        self.clients = []
        self.total_art = 0

    def register_client(self, client, yesno):
        if yesno == 0:
            self.clients.remove(client)
        else:
            self.clients.append(client)
            self.update_client(client)

    def register_art(self, mesh = None, texture = None):
        a = ArtCurator.ArtAsset(self.total_art, mesh, texture)
        self.art_asset_lock.acquire()
        self.art_assets[a.art_id] = a
        self.total_art += 1
        self.art_asset_lock.release()
        self.update_clients(a.art_id)
        return a.art_id

    def deregister_art(self, art_id):
        self.art_asset_lock.acquire()
        del self.art_assets[art_id]
        self.art_asset_lock.release()

    def update_art(self, art_id, mesh, texture):
        self.art_asset_lock.acquire()
        if mesh != None:
            self.art_assets[art_id].mesh = mesh

        if texture != None:
            self.art_assets[art_id].texture = texture
        self.art_asset_lock.release()

        if mesh != None or texture != None:
            self.update_clients(art_id)

    # This just updates clients on changed art elements
    # This should probably be asynchronous
    def update_clients(self, art_id):
        for client_obj in self.clients:
            self.update_client(client_obj, art_id)

    # Update a single client either with a specific art ID, or with everything.
    # This should probably be asynchronous
    def update_client(self, client_obj, art_id = None):
        if art_id == None:
            for k in self.art_assets:
                a = self.art_assets[k]
                VisualMetaDataMsg.send(client_obj.client, [a.art_id, a.mesh, a.texture])
        else:
            a = self.art_assets[art_id]
            VisualMetaDataMsg.send(client_obj.client, [art_id, a.mesh, a.texture])

    def attach_art_asset(self, art_id, phys_id):
        self.art_assignment[phys_id] = art_id

    def detach_art_asset(self, phys_id):
        del self.art_assignment[phys_id]

class Universe:
    # ======================================================================

    class ThreadSim(threading.Thread):
        def __init__(self, universe):
            threading.Thread.__init__(self)
            self.universe = universe

        def run(self):
            self.universe.sim(t = 0)

    # ======================================================================

    def __init__(self):
        self.attractors = []
        self.phys_objects = []
        self.beams = []
        self.smarties = [] # all 'smart' objects that can interact with the server
        self.phys_lock = threading.Lock()
        # ### PARAMETER ###  UNIVERSE TCP PORT
        self.net = MIMOServer(self.register_smarty, port = 5505)
        self.net.start()
        self.sim_thread = Universe.ThreadSim(self)
        self.curator = ArtCurator(self)
        self.vis_data_clients = []
        self.simulating = 0
        self.total_objs = 0
        self.frametime = 0

    def stop_net(self):
        self.net.stop()

    def start_net(self):
        self.net.start()

    def add_object(self, obj):
        self.phys_lock.acquire()
        if obj.emits_gravity:
            self.attractors.append(obj)

        if isinstance(obj, SmartPhysicsObject):
            self.smarties.append(obj)

        self.phys_objects.append(obj)
        self.phys_lock.release()

    def add_beam(self, beam):
        self.phys_lock.acquire()
        self.beams.append(beam)
        self.phys_lock.release()

    def destroy_object(self, obj):
        self.phys_lock.acquire()
        if obj.emits_gravity:
            self.attractors.remove(obj)

        if isinstance(obj, SmartPhysicsObject):
            self.smarties.remove(obj)

        self.phys_objects.remove(obj)
        self.phys_lock.release()

    def update_attractor(self, obj):
        self.phys_lock.acquire()
        if obj.emits_gravity:
            self.attractors.append(obj)
        else:
            self.attractors.remove(obj)
        self.phys_lock.release()

    def get_id(self):
        self.total_objs += 1
        return self.total_objs - 1

    def register_smarty(self, client):
        # Now we need to talk to the client.
        newsmarty = SmartPhysicsObject(self, client)
        self.add_object(newsmarty)
        return newsmarty

    def register_for_vis_data(self, obj, yesno):
        if yesno == 1:
            self.vis_data_clients.append(obj)
        else:
            self.vis_data_clients.remove(obj)

    def broadcast_vis_data(self):
        if len(self.vis_data_clients) == 0:
            return

        self.phys_lock.acquire()
        num_objects = len(self.phys_objects)
        msgs = []

        for o in self.phys_objects:
            msgs.append("VISDATA\n%d\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n" % (
                        o.phys_id, o.radius,
                        o.position.x, o.position.y, o.position.z,
                        o.orientation.x, o.orientation.y, o.orientation.z))

        self.phys_lock.release()

        for_removal = []
        for c in self.vis_data_clients:
            for m in msgs:
                ret = VisualDataMsg.send(c.client, m)

                if not ret:
                    c.vis_data = 0
                    for_removal.append(c)
                    break

        for c in for_removal:
            self.vis_data_clients.remove(c)

    @staticmethod
    def gravity(big, small):
        m = 6.67384e-11 * big.mass * small.mass / big.position.dist2(small.position)
        r = small.position.ray(big.position)
        r.scale(m)
        return r

    def move_object(self, obj, force, dt):
        # Verlet integration: http://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet
        obj.position.x += obj.velocity.x * dt + 0.5 * dt * dt * force.x
        obj.position.y += obj.velocity.y * dt + 0.5 * dt * dt * force.y
        obj.position.z += obj.velocity.z * dt + 0.5 * dt * dt * force.z
        
        obj.velocity.x += dt * force.x
        obj.velocity.y += dt * force.y
        obj.velocity.z += dt * force.z

    # Detect whether the two objects will collide with in the
    # given time delta into the future
    def phys_collide(self, obj1, obj2, dt):
        # ### TODO ### Properly handle forces here.
        
        pass

    def get_force(self, obj):
        force = Vector3([0, 0, 0])
        if obj.mass == 0:
            return force
            
        # First get the attraction between this object, and all of the attractors.
        for a in self.attractors:
            if obj == a:
                continue
            force.add(Universe.gravity(a, obj))

        if isinstance(obj, SmartPhysicsObject):
            force.add(obj.thrust)

        force.scale(1 / obj.mass)

        return force

    def tick(self, dt):
        self.phys_lock.acquire()

        # ### TODO ### Multithread this. It is pretty trivially parallelizable.
        N = len(self.phys_objects)
        for i in range(0, N):
            for j in range(i, N):
                ret = self.phys_collide(self.phys_objects[i], self.phys_objects[j], dt)

            for b in self.beams:
                b.collide(self.phys_objects[i], dt)

        for o in self.phys_objects:
            self.move_object(o, self.get_force(o), dt)

        for b in self.beams:
            b.tick(dt)
                
        self.phys_lock.release()

        self.broadcast_vis_data()

    def start_sim(self):
        if self.simulating == 0:
            self.simulating = 1
            self.sim_thread.start()

    def stop_sim(self):
        if self.simulating == 1:
            self.simulating = 0
            self.sim_thread.join()
            self.sim_thread = Universe.ThreadSim(self)

    # Number of real seconds and a rate of simulation together will rate-limit
    # the simulation
    def sim(self, t = 0, r = 1):
        # ### PARAMETER ###  MINIMUM FRAME TIME
        min_frametime = 0.001
        # ### PARAMETER ###  MAXIMUM FRAME TIME
        max_frametime = 0.2
        total_time = 0;
        dt = 0.01
        i = 0

        while self.simulating == 1 and (t == 0 or total_time < r * t):
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

            # shrink the frametime if it we are ticking too long.
            # This has the effect of slowing down time, but whatever.
            dt = min(max_frametime, dt)
            self.frametime = dt
            total_time += r * dt
            i += 1
        return [total_time, i]

    def get_frametime(self):
        return self.frametime

if __name__ == "__main__":
    import sys
    import random
    import time

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

        obj = PhysicsObject(uni, position = [ (c + a * cos(v)) * cos(u), (c + a * cos(v)) * sin(u), a * sin(v) ], mass = rand.random() * 75 + 25)
        uni.add_object(obj)

    r = 10000000
    t = 100000

    #make 100 random gravitation objects
    for i in range(0, 100):
        u = rand.random() * 2 * pi
        v = rand.random() * 2 * pi
        c = r + (rand.random() * 2 - 1) * t
        a = rand.random() * t

        obj = PhysicsObject(uni, position = [ (c + a * cos(v)) * cos(u), (c + a * cos(v)) * sin(u), a * sin(v) ], mass = rand.random() * 100000000 + 1e18, radius = 5000000 + rand.random() * 2000000)
        uni.add_object(obj)

    print len(uni.phys_objects)
    print len(uni.attractors)

    print uni.phys_objects[0].position.dist(uni.attractors[0].position)
    uni.start_sim()
    time.sleep(1)
    print "%f s per physics tick" % uni.get_frametime()
    print "Press Enter to continue..."
    sys.stdout.flush()
    raw_input()
    print uni.phys_objects[0].position.dist(uni.attractors[0].position)
    print "Stopping simulation"
    uni.stop_sim()
    print "Stopping network"
    uni.stop_net()
    print "Stopped"

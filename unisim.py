#!/usr/bin/env python

import threading

from physics import Vector3, PhysicsObject, GravitationalBody, SmartPhysicsObject
from math import sin, cos, pi, sqrt
from mimosrv import MIMOServer
from message import VisualDataMsg

VERSION = 0

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
        self.nonphys_objects = []
        self.smarties = [] # all 'smart' objects that can interact with the server
        self.phys_lock = threading.Lock()
        self.net = MIMOServer(self.register_smarty, port = 5505)
        self.net.start()
        self.sim_thread = Universe.ThreadSim(self)
        self.vis_meta_data_clients = []
        self.vis_data_clients = []
        self.simulating = 0
        self.next_id = 0

    def stop_net(self):
        self.net.stop()

    def add_object(self, obj):
        if isinstance(obj, GravitationalBody):
            self.attractors.append(obj)
            return

        if isinstance(obj, SmartPhysicsObject):
            self.smarties.append(obj)

        self.phys_objects.append(obj)

    def get_id(self):
        self.next_id += 1
        return self.next_id - 1

    def register_smarty(self, client):
        # Now we need to talk to the client.
        newsmarty = SmartPhysicsObject(self, client)
        self.add_object(newsmarty)
        return newsmarty

    def register_for_vis_data(self, obj, yesno):
        if yesno:
            self.vis_data_clients.append(obj)
        else:
            self.vis_data_clients.remove(obj)

    def broadcast_vis_data(self):
        #if len(self.vis_data_clients) == 0:
            #return
            
        from cStringIO import StringIO
        file_str = StringIO()
        
        self.phys_lock.acquire()
        # First we serialize the the objects with phys_id, position, orientation, radius

        num_objects = len(self.phys_objects)

        for o in self.phys_objects:
            file_str.write(`o.phys_id`)
            file_str.write("\n")
            #file_str.write(`"%.55f" % o.position.x`)
            file_str.write(`o.position.x`)
            file_str.write("\n")
            file_str.write(`o.position.y`)
            file_str.write("\n")
            file_str.write(`o.position.z`)
            file_str.write("\n")
            file_str.write(`o.orientation.x`)
            file_str.write("\n")
            file_str.write(`o.orientation.y`)
            file_str.write("\n")
            file_str.write(`o.orientation.z`)
            file_str.write("\n")

        for o in self.attractors:
            file_str.write(`o.phys_id`)
            file_str.write("\n")
            file_str.write(`o.position.x`)
            file_str.write("\n")
            file_str.write(`o.position.y`)
            file_str.write("\n")
            file_str.write(`o.position.z`)
            file_str.write("\n")
            file_str.write(`o.orientation.x`)
            file_str.write("\n")
            file_str.write(`o.orientation.y`)
            file_str.write("\n")
            file_str.write(`o.orientation.z`)
            file_str.write("\n")

        self.phys_lock.release()

        msg = "VISDATA\n%d\n" % num_objects
        msg += file_str.getvalue().replace("'","")

        for c in self.vis_data_clients:
            ret = VisualDataMsg.send(c.client, msg)

            if not ret:
                c.vis_data = 0
                self.vis_data_clients.remove(c)

    def register_for_vis_meta_data(self, obj, yesno):
        if yesno:
            self.vis_meta_data_clients.append(obj)
            self.send_meta_data(obj)
        else:
            self.vis_meta_data_clients.remove(obj)

    def notify_updated_vis_meta_data(self, obj):
        pass

    @staticmethod
    def gravity(big, small):
        m = 6.67384e-11 * big.mass * small.mass / big.position.dist2(small.position)
        r = small.position.ray(big.position)
        r.scale(m)
        return r

    def tick(self, dt):
        self.phys_lock.acquire()
        for o in self.phys_objects:
            gforce = Vector3([0, 0, 0])
            if o.mass > 0:
                # First get the attraction between this object, and all of the attractors.
                for a in self.attractors:
                    gforce.add(Universe.gravity(a, o))
                if isinstance(o, SmartPhysicsObject):
                    gforce.add(o.thrust)
                gforce.scale(1 / o.mass)

            # Verlet integration: http://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet
            o.position.x += o.velocity.x * dt + 0.5 * dt * dt * gforce.x
            o.position.y += o.velocity.y * dt + 0.5 * dt * dt * gforce.y
            o.position.z += o.velocity.z * dt + 0.5 * dt * dt * gforce.z

            o.velocity.x += dt * gforce.x
            o.velocity.y += dt * gforce.y
            o.velocity.z += dt * gforce.z

        for o in self.attractors:

            # Verlet integration: http://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet
            o.position.x += o.velocity.x * dt
            o.position.y += o.velocity.y * dt
            o.position.z += o.velocity.z * dt
            
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

    # Number of real seconds and a rate of simulation.
    def sim(self, t = 1, r = 1):
        min_frametime = 0.001
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

        obj = GravitationalBody(uni, position = [ (c + a * cos(v)) * cos(u), (c + a * cos(v)) * sin(u), a * sin(v) ], mass = rand.random() * 100000 + 1e18)
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

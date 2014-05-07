#!/usr/bin/python

import message
import objectsim
import unisim
import threading
import ship
from shiptypes import Firefly
from vector import Vector3
import random
import time
import spaceobj
import sys
import math

def testSimple():
    miss1 = spaceobj.Missile(osim)
    osim.spawn_object(miss1)

    print "miss1 osimid is: %d" % miss1.osim_id
    print "miss1 unisim is: %d" % miss1.phys_id

def testSimpleShip():
    ship1 = Firefly(osim)
    osim.spawn_object(ship1)

    print "ship1 osimid is: %d" % ship1.osim_id
    print "ship1 unisim is: %d" % ship1.phys_id
    

#test currently broken    
def testVisData():
    
    miss1 = spaceobj.Missile(osim)
    osim.spawn_object(miss1)
    
    miss2 = spaceobj.Missile(osim)
    miss2.position[0] = 100.0
    osim.spawn_object(miss2)
    
    print "miss1 osimid is: %d" % miss1.osim_id
    print "miss1 unisim is: %d" % miss1.phys_id
    
    print "miss2 osimid is: %d" % miss2.osim_id
    print "miss2 unisim is: %d" % miss2.phys_id
    
    
    miss1.enable_visdata()
    
    while True:
        print miss1.sock.recv(1024)
    
    pass


#spawns two stationary ships. The first fires a missile at the other
def testShip():
    
    ship1 = Firefly(osim, name="Ship 1")
    osim.spawn_object(ship1)

    print "ship1 osimid is: %d" % ship1.osim_id
    print "ship1 unisim is: %d" % ship1.phys_id
    sys.stdout.flush()

    ship2 = Firefly(osim, name="Ship 2")
    ship2.position = Vector3( (1000.0,10.0,0.0) )
    osim.spawn_object(ship2)

    print "ship2 osimid is: %d" % ship2.osim_id
    print "ship2 unisim is: %d" % ship2.phys_id
    sys.stdout.flush()

    direction = Vector3((1.0, 0.0,0.0))
    miss1 = ship1.fire_missile(direction, 50000.0)

    print "miss1 osimid is: %d" % miss1.osim_id
    print "miss1 unisim is: %d" % miss1.phys_id
    sys.stdout.flush()
    
#spawns two stationary ships. The first fires a beam at the other
def testBeam():
    
    ship1 = Firefly(osim, name="Ship 1")
    osim.spawn_object(ship1)

    print "ship1 osimid is: %d" % ship1.osim_id
    print "ship1 unisim is: %d" % ship1.phys_id
    sys.stdout.flush()

    ship2 = Firefly(osim, name="Ship 2")
    ship2.position = Vector3( (1000.0,0.0,0.0) )
    osim.spawn_object(ship2)

    print "ship2 osimid is: %d" % ship2.osim_id
    print "ship2 unisim is: %d" % ship2.phys_id
    sys.stdout.flush()

    dir = Vector3(1.0,0.0,0.0)
    ship1.fire_laser(dir, power = 1000)
    
    sys.stdout.flush()
    
    
def rand_vec(rand, rang):
    return Vector3((rand.random()*rang, rand.random()*rang, rand.random()*rang))


def stressTest(ships=1000, area=10000):
    
    for i in range(0,ships):
        ship1 = Firefly(osim)
        ship1.position = rand_vec(rand, area)
        osim.spawn_object(ship1)
        print "ship%d osimid is: %d" % (i, ship1.osim_id)
        print "ship%d unisim is: %d" % (i, ship1.phys_id)

        direction = Vector3((1.0, 0.0,0.0))
        miss1 = ship1.fire_missile(direction, 500)
    
        print "miss%d osimid is: %d" % (i, miss1.osim_id)
        print "miss%d unisim is: %d" % (i, miss1.phys_id)


def testHoming():
    ship1 = Firefly(osim)
    osim.spawn_object(ship1)

    print "ship1 osimid is: %d" % ship1.osim_id
    print "ship1 unisim is: %d" % ship1.phys_id
    sys.stdout.flush()

    ship2 = Firefly(osim)
    ship2.position = Vector3( (50000.0,0.0,0.0) )
    ship2.velocity = Vector3( (0, 1000, 0) )
    osim.spawn_object(ship2)

    print "ship2 osimid is: %d" % ship2.osim_id
    print "ship2 unisim is: %d" % ship2.phys_id
    sys.stdout.flush()

    direction = Vector3((1.0, 0.0,0.0))
    miss1 = ship1.fire_homing(direction, 50000.0)

    print "miss1 osimid is: %d" % miss1.osim_id
    print "miss1 unisim is: %d" % miss1.phys_id
    sys.stdout.flush()

#unism.test()

class TThread(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        
    def run(self):
        while True:
            time.sleep(500)
    

def test_threads():
    for i in range (0, 10000):
        t = TThread()
        t.start()
        print "Started thread %d" % i

osim = objectsim.ObjectSim()
rand = random.Random()
rand.seed(0)

#testVisData()
testShip()
#testSimpleShip()
#stressTest()
#test_threads()
#testBeam()
#testHoming()
#testBeam()

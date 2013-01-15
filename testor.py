#!/usr/bin/python

import message
import objectsim
import unisim
import threading
import ship
from vector import Vector3
import random
import time
import spaceobj
import sys
import math

def testSimple():
    miss1 = spaceobj.Missile(osim)
    osim.spawn_object(miss1)

    print "miss1 osimid is: %d" % miss1.osid
    print "miss1 unisim is: %d" % miss1.uniid

def testSimpleShip():
    ship1 = ship.Ship(osim)
    osim.spawn_object(ship1)

    print "ship1 osimid is: %d" % ship1.osid
    print "ship1 unisim is: %d" % ship1.uniid
    

#test currently broken    
def testVisData():
    
    miss1 = spaceobj.Missile(osim)
    osim.spawn_object(miss1)
    
    miss2 = spaceobj.Missile(osim)
    miss2.location[0] = 100.0
    osim.spawn_object(miss2)
    
    print "miss1 osimid is: %d" % miss1.osid
    print "miss1 unisim is: %d" % miss1.uniid
    
    print "miss2 osimid is: %d" % miss2.osid
    print "miss2 unisim is: %d" % miss2.uniid
    
    
    miss1.enable_visdata()
    
    while True:
        print miss1.sock.recv(1024)
    
    pass


#spawns two stationary ships. The first fires a missile at the other
def testShip():
    
    ship1 = ship.Ship(osim, type="Ship 1")
    osim.spawn_object(ship1)

    print "ship1 osimid is: %d" % ship1.osid
    print "ship1 unisim is: %d" % ship1.uniid
    sys.stdout.flush()

    ship2 = ship.Ship(osim, type="Ship 2")
    ship2.location = Vector3( (1000.0,10.0,0.0) )
    osim.spawn_object(ship2)

    print "ship2 osimid is: %d" % ship2.osid
    print "ship2 unisim is: %d" % ship2.uniid
    sys.stdout.flush()

    direction = Vector3((1.0, 0.0,0.0))
    miss1 = ship1.fire_missile(direction, 50000.0)

    print "miss1 osimid is: %d" % miss1.osid
    print "miss1 unisim is: %d" % miss1.uniid
    sys.stdout.flush()
    
#spawns two stationary ships. The first fires a beam at the other
def testBeam():
    
    ship1 = ship.Ship(osim, type="Ship 1")
    osim.spawn_object(ship1)

    print "ship1 osimid is: %d" % ship1.osid
    print "ship1 unisim is: %d" % ship1.uniid
    sys.stdout.flush()

    ship2 = ship.Ship(osim, type="Ship 2")
    ship2.location = Vector3( (1000.0,0.0,0.0) )
    osim.spawn_object(ship2)

    print "ship2 osimid is: %d" % ship2.osid
    print "ship2 unisim is: %d" % ship2.uniid
    sys.stdout.flush()

    dir = Vector3(1.0,0.0,0.0)
    ship1.fire_laser(dir, power = 1000)
    
    sys.stdout.flush()
    
    
def rand_vec(rand, rang):
    return Vector3((rand.random()*rang, rand.random()*rang, rand.random()*rang))


def stressTest(ships=1000, area=10000):
    
    for i in range(0,ships):
        ship1 = ship.Ship(osim)
        ship1.location = rand_vec(rand, area)
        osim.spawn_object(ship1)
        print "ship%d osimid is: %d" % (i, ship1.osid)
        print "ship%d unisim is: %d" % (i, ship1.uniid)

        direction = Vector3((1.0, 0.0,0.0))
        miss1 = ship1.fire_missile(direction, 500)
    
        print "miss%d osimid is: %d" % (i, miss1.osid)
        print "miss%d unisim is: %d" % (i, miss1.uniid)


def testHoming():
    ship1 = ship.Ship(osim, type="Ship 1")
    osim.spawn_object(ship1)

    print "ship1 osimid is: %d" % ship1.osid
    print "ship1 unisim is: %d" % ship1.uniid
    sys.stdout.flush()

    ship2 = ship.Ship(osim, type="Ship 2")
    ship2.location = Vector3( (50000.0,0.0,0.0) )
    ship2.velocity = Vector3( (0, 1000, 0) )
    osim.spawn_object(ship2)

    print "ship2 osimid is: %d" % ship2.osid
    print "ship2 unisim is: %d" % ship2.uniid
    sys.stdout.flush()

    direction = Vector3((1.0, 0.0,0.0))
    miss1 = ship1.fire_homing(direction, 50000.0)

    print "miss1 osimid is: %d" % miss1.osid
    print "miss1 unisim is: %d" % miss1.uniid
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
#testShip()
#testSimpleShip()
#stressTest()
#test_threads()
#testBeam()
testHoming()
#testBeam()

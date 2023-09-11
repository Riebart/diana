#!/usr/bin/python

from __future__ import print_function
import message
import objectsim
#import unisim
import threading
from spaceobjects.ship import ship
from spaceobjects.ship.shiptypes import Firefly
from spaceobjects.spaceobj import SpaceObject
from vector import Vector3
import random
import time
from spaceobjects import spaceobj
import sys
import math


def testSimple():
    print("\n***Begin simple test, spawning a missile directly into osim")
    miss1 = spaceobj.Missile(osim)
    osim.spawn_object(miss1)

    print("miss1 osimid is: %d" % miss1.osim_id)
    print("miss1 unisim is: %d" % miss1.phys_id)
    print("***Simple test complete")

def testSimpleShip():
    print("\n***Begin simple ship test, spawning a 'Firefly' type ship into the osim")
    ship1 = Firefly(osim)
    osim.spawn_object(ship1)

    print("ship1 osimid is: %d" % ship1.osim_id)
    print("ship1 unisim is: %d" % ship1.phys_id)

    print("Sleeping for 5 seconds, then disconnecting the ship.")
    time.sleep(5)
    print("Disconnecting ship.")
    print("***Simple ship test complete")


#test currently broken
def testVisData():

    miss1 = spaceobj.Missile(osim)
    osim.spawn_object(miss1)

    miss2 = spaceobj.Missile(osim)
    miss2.position[0] = 100.0
    osim.spawn_object(miss2)

    print("miss1 osimid is: %d" % miss1.osim_id)
    print("miss1 unisim is: %d" % miss1.phys_id)

    print("miss2 osimid is: %d" % miss2.osim_id)
    print("miss2 unisim is: %d" % miss2.phys_id)


    miss1.enable_visdata()

    while True:
        print(miss1.sock.recv(1024))

    pass


#spawns two stationary ships. The first fires a missile at the other
def testShip():
    print("\n***Begin ship test, spawning two ships into osim at different locations. Ship2 fires a missile at ship 1 using the class code. I don't remember if the missile is supposed to hit the ship")

    ship1 = Firefly(osim, name="Ship 1")
    osim.spawn_object(ship1)

    print("ship1 osimid is: %d" % ship1.osim_id)
    print("ship1 unisim is: %d" % ship1.phys_id)
    sys.stdout.flush()

    ship2 = Firefly(osim, name="Ship 2")
    #ship2 = SpaceObject(osim)
    ship2.position = Vector3( (1000.0,10.0,0.0) )
    osim.spawn_object(ship2)

    print("ship2 osimid is: %d" % ship2.osim_id)
    print("ship2 unisim is: %d" % ship2.phys_id)
    sys.stdout.flush()

    direction = Vector3((1.0, 0.0,0.0))
    miss1 = ship1.fire_missile(direction, 50000.0)

    print("miss1 osimid is: %d" % miss1.osim_id)
    print("miss1 unisim is: %d" % miss1.phys_id)
    sys.stdout.flush()
    print("***Ship test complete (output may continue)")

#spawns two stationary ships. The first fires a beam at the other
def testBeam():
    print("\n***Begin beam test, spawning two ships into osim at different locations. Ship2 fires a laser at ship 1 using the class code. I don't remember if the beam is supposed to hit the ship")

    ship1 = Firefly(osim, name="Ship 1")
    osim.spawn_object(ship1)

    print("ship1 osimid is: %d" % ship1.osim_id)
    print("ship1 unisim is: %d" % ship1.phys_id)
    sys.stdout.flush()

    ship2 = Firefly(osim, name="Ship 2")
    ship2.position = Vector3( (1000.0,0.0,0.0) )
    osim.spawn_object(ship2)

    print("ship2 osimid is: %d" % ship2.osim_id)
    print("ship2 unisim is: %d" % ship2.phys_id)
    sys.stdout.flush()

    dir = Vector3(1.0,0.0,0.0)
    ship1.fire_laser(bank_id=1, direction=dir, power = 1000)

    sys.stdout.flush()
    print("***Beam test complete (output may continue)")


def rand_vec(rand, rang):
    return Vector3((rand.random()*rang, rand.random()*rang, rand.random()*rang))


def stressTest(ships=1000, area=10000):
    print(f"\n***Begin stress test, spawning {ships} Fireflys into the osim, scattered across a spehere of radius? {area}. Each ship fires a missile.")
    for i in range(0,ships):
        ship1 = Firefly(osim)
        ship1.position = rand_vec(rand, area)
        osim.spawn_object(ship1)
        print("ship%d osimid is: %d" % (i, ship1.osim_id))
        print("ship%d unisim is: %d" % (i, ship1.phys_id))

        direction = Vector3((1.0, 0.0,0.0))
        miss1 = ship1.fire_missile(direction, 500)

        print("miss%d osimid is: %d" % (i, miss1.osim_id))
        print("miss%d unisim is: %d" % (i, miss1.phys_id))

    print(f"***Stress test complete")

def testHoming():
    print(f"\n***Begin homing test. Spawn two ships, one fires a homing missile at the other. See code for homing missile")
    print("Building ship ...")
    ship1 = Firefly(osim)
    print("Building ship ... Done")
    print("Spawning ship ...")
    osim.spawn_object(ship1)
    print("Spawning ship ... Done")

    print("ship1 osimid is: %d" % ship1.osim_id)
    print("ship1 unisim is: %d" % ship1.phys_id)
    sys.stdout.flush()

    ship2 = Firefly(osim)
    #ship2 = SpaceObject(osim)
    ship2.position = Vector3( (50000.0,0.0,0.0) )
    ship2.velocity = Vector3( (0.0, 1000.0, 0.0) )
    osim.spawn_object(ship2)

    print("ship2 osimid is: %d" % ship2.osim_id)
    print("ship2 unisim is: %d" % ship2.phys_id)
    sys.stdout.flush()

    direction = Vector3((1.0, 0.0,0.0))
    miss1 = ship1.fire_homing(direction, 50000.0)

    print("miss1 osimid is: %d" % miss1.osim_id)
    print("miss1 unisim is: %d" % miss1.phys_id)
    sys.stdout.flush()
    print(f"***Homing missile test complete")

#unism.test()

class TThread(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)

    def run(self):
        while True:
            time.sleep(500)


def test_threads():
    print(f"\n***Begin threads test, try to spawn 10000 threads for some reason (died at ~4k last time I tried) ")
    for i in range (0, 10000):
        t = TThread()
        t.start()
        print("Started thread %d" % i)
    print(f"***Threat test complete")


def test_pbs():
    pass
    #for now, this is exampe code
"""
    res_string = socket.doRead(size)
    msg = MessageWrapper()
    msg.ParseFromString(res_string)
    l
    if (msg.MessageType is PHYSPROPS):
        doStuff(msg.HelloMsg)
    else if (msg.MessageType is HELLO):
        doStuff(msg.PhysPropsMsg)
"""


print("Spawning OSIM ...")
osim = objectsim.ObjectSim()
print("Spawning OSIM ... Done")
rand = random.Random()
rand.seed(0)

#testVisData()
#testShip()
#testSimpleShip()
#stressTest()
#test_threads()
# testBeam()
testHoming()
#testBeam()

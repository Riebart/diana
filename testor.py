#!/usr/bin/python

import message
import objectsim
import unisim
import threading
import ship
from physics import Vector3

def testVisData():
    
    miss1 = objectsim.Missile(osim)
    osim.spawn_object(miss1)
    
    miss2 = objectsim.Missile(osim)
    miss2.location[0] = 100.0
    osim.spawn_object(miss2)
    
    print "miss1 osimid is: %d" % miss1.osid
    print "miss1 unisim is: %d" % miss1.uniid
    
    print "miss2 osimid is: %d" % miss2.osid
    print "miss2 unisim is: %d" % miss2.uniid
    
    
    osim.enable_visdata(miss1.osid)
    
    while True:
        print miss1.sock.recv(1024)
    
    pass



def testShip():
    
    ship1 = ship.Ship(osim)
    osim.spawn_object(ship1)

    print "ship1 osimid is: %d" % ship1.osid
    print "ship1 unisim is: %d" % ship1.uniid
        
    ship2 = ship.Ship(osim)
    osim.location = Vector3( (100.0,0.0,0.0) )
    osim.spawn_object(ship2)
    
    print "ship2 osimid is: %d" % ship2.osid
    print "ship2 unisim is: %d" % ship2.uniid
    
    direction = Vector3((1.0, 0.0,0.0))
    miss1 = ship1.fire_missile(direction, 500)
    
    print "miss1 osimid is: %d" % miss1.osid
    print "miss1 unisim is: %d" % miss1.uniid    
    
    

#unism.test()

osim = objectsim.ObjectSim()

#testVisData()
testShip()



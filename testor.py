#!/usr/bin/python

import message
import objectsim
import unisim
import threading

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





#unism.test()

osim = objectsim.ObjectSim()

testVisData()



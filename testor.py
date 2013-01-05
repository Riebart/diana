#!/usr/bin/python

import message
import objectsim
import unisim
import threading

def testVisData():
    
    miss = objectsim.Missile()
    osim.spawn_object(miss)
    
    print "My osimid id is: %d" % miss.osid
    print "My unisim id is: %d" % miss.uniid
    
    osim.enable_visdata(miss.osid)
    
    while True:
        print miss.sock.recv(1024)
    
    pass





#unism.test()

osim = objectsim.ObjectSim()

testVisData()



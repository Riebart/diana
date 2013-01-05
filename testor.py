#!/usr/bin/python

import message
import objectsim
import unisim
import threading

def testVisData():
    
    miss = objectsim.Missile()
    osim.spawn_object(miss)
    
    osim.enable_visdata(miss.osid)
    
    while True:
        print miss.sock.recv(1024)
    
    pass





#unism.test()

osim = objectsim.ObjectSim()

testVisData()



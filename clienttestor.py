#!/usr/bin/python

import ship
import message
import socket
import objectsim
import random
from vector import Vector3

def test_vis_data():
    direction = Vector3((1.0, 0.0,0.0))
    miss1 = ship1.fire_missile(direction, 50000.0)
    print ship1

    sock = socket.socket()
    sock.connect( ("localhost", 5511) )
    
    message.VisualDataEnableMsg.send(sock, 0, 0, 1)
    
    while True:
        print sock.recv(1024)
    
    
osim = objectsim.ObjectSim()
rand = random.Random()
rand.seed(0)

ship1 = ship.Ship(osim, type="Ship 1", port=5511)
ship1.name = "Ship 1"
osim.spawn_object(ship1)



#ship2 = ship.Ship(osim, type="Ship 2")
#osim.spawn_object(ship2)

test_vis_data()

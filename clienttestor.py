#!/usr/bin/python

import ship
import message
import socket
import objectsim
import random
import bson
import time
import math
from vector import Vector3

def pool_rack():
    sock = socket.socket()
    sock.connect( ("localhost", 5505) )

    import sys
    from math import sin, cos, pi, sqrt
    from message import SpawnMsg

    ball_mass = 15.0
    ball_radius = 1.0

    num_rows = 15
    C = 1.0
    y_scale = sqrt(3) / 2
    y_offset = 0.0

    sm = SpawnMsg()
    sm.srv_id = -1
    sm.cli_id = -1
    sm.is_smart = False
    sm.velocity = [0.0,0.0,0.0]
    sm.thrust = [0.0,0.0,0.0]
    sm.orientation = [0.0,0.0,0.0,0.0]

    for i in range(0, num_rows):
        for j in range(0, i+1):
            sm.radius = ball_radius
            sm.object_type = "Target Ball %d" % (i * (i + 1) / 2 + j + 1)
            sm.mass = ball_mass
            sm.position = [  C * (i - 2 * j) * ball_radius, y_offset - C * y_scale * (1 + 2 * i) * ball_radius, 0.0 ]
            print sm.position
            SpawnMsg.send(sock, None, -1, sm.build())

    # This makes us a cue ball
    sm.srv_id = -1
    sm.cli_id = -1
    sm.is_smart = False
    sm.object_type = "Cue ball"
    sm.mass = (17.0/15.0) * ball_mass
    sm.position = [0.0,50.0,0.0]
    sm.velocity = [0.0,-5.0,0.0]
    sm.radius = ball_radius
    SpawnMsg.send(sock, None, -1, sm.build())

    sock.shutdown(socket.SHUT_RDWR)
    sock.close()

def spawn_sol():
    sock = socket.socket()
    sock.connect( ("localhost", 5505) )

    objects = {}
    import message
    import json

    with open('sol.csv','r') as fp:
        for l in fp:
            parts = l.strip().split(",")
            obj_id = int(parts[1])
            # The units are in KM and KM/S, so scale to metres.
            if obj_id not in objects:
                objects[obj_id] = { "name": parts[2] + " (" + str(obj_id) + ")" }
            if parts[0] == "R":
                objects[obj_id]["radius"] = 1000*float(parts[3])
            if parts[0] == "M":
                objects[obj_id]["mass"] = float(parts[3])
            if parts[0] == "P":
                objects[obj_id]["date-J2000"] = float(parts[3])
                objects[obj_id]["date-str"] = parts[4]
                objects[obj_id]["position"] = [ 1000*float(parts[5]), 1000*float(parts[6]), 1000*float(parts[7]) ]
                objects[obj_id]["velocity"] = [ 1000*float(parts[8]), 1000*float(parts[9]), 1000*float(parts[10]) ]

    sm = message.SpawnMsg()
    sm.srv_id = None
    sm.cli_id = -1
    sm.is_smart = False
    sm.thrust = [0.0,0.0,0.0]
    sm.orientation = [0.0,0.0,0.0,0.0]

    phys_id = 1
    for k in sorted(objects.keys()):
        sm.object_type = objects[k]["name"]
        sm.mass = objects[k]["mass"]
        sm.radius = objects[k]["radius"]
        sm.position = objects[k]["position"]
        sm.velocity = objects[k]["velocity"]
        print str(phys_id) + ": " + sm.object_type
        message.SpawnMsg.send(sock, None, -1, sm.build())
        phys_id += 1

def signature_test():
    sock = socket.socket()
    sock.connect( ("localhost", 5505) )

    sm = message.SpawnMsg()
    sm.srv_id = None
    sm.cli_id = -1
    sm.is_smart = False
    sm.thrust = [0.0,0.0,0.0]
    sm.velocity = [0.0,0.0,0.0]
    sm.orientation = [0.0,0.0,0.0,0.0]
    sm.object_type = "NonRadiatorDumb"
    sm.mass = 1.0
    sm.radius = 1.0
    sm.position = [0.0,0.0,0.0]
    message.SpawnMsg.send(sock, None, -1, sm.build())

    sm.object_type = "RadiatorSmarty"
    sm.position = [10.0,0.0,0.0]
    sm.spectrum = message.Spectrum([550e-9],[1500.0])
    sm.is_smart = True
    message.SpawnMsg.send(sock, None, 1, sm.build())
    msg = message.Message.get_message(sock)
    smarty_id = msg.srv_id

    sm.object_type = "BIGRadiatorDumb"
    sm.position = [20.0,0.0,0.0]
    sm.spectrum = message.Spectrum([550e-9],[1.5e8])
    sm.is_smart = False
    message.SpawnMsg.send(sock, None, -1, sm.build())

    time.sleep(5)
    sqm = message.ScanQueryMsg()
    message.ScanQueryMsg.send(sock, smarty_id, 1, sqm.build())

    sbm = message.BeamMsg()
    sbm.beam_type = "SCAN"
    sbm.energy = 1e6
    sbm.srv_id = smarty_id
    sbm.cli_id = 1
    sbm.origin = [0.0,0.0,0.0]
    sbm.velocity = [0.0,0.0,5.0]
    sbm.up = [1.0,0.0,0.0]
    sbm.spread_h = 2 * math.pi
    sbm.spread_v = 2 * math.pi
    sbm.spectrum = message.Spectrum([550e-9],[1000])
    message.BeamMsg.send(sock, smarty_id, 1, sbm.build())

    while True:
        msg = message.Message.get_message(sock)
        print msg.__dict__


def dirmsg(sock, msg):
    message.DirectoryMsg.send(sock, 0,0, msg)    
    res = sock.recv(5000)
    newmsg = bson.loads(res)
    print newmsg
    return newmsg


def test_systems():
    sock = socket.socket()
    sock.connect( ("localhost", 5506) )

    objects = {}

    
    msg = {'\x03':"CLASS", '\x04':0, '\x05': {}, '\x06':{}}
    dirmsg(sock, msg)
    
    msg = {'\x03':"SHIP", '\x04':0, '\x05': {}, '\x06':{}}
    dirmsg(sock, msg)


    
    msg = {'\x03':"CLASS", '\x04':1, '\x05': [1], '\x06':[0]}
    newmsg = dirmsg(sock, msg)
    
    ship_id = newmsg['\x01']
    client_id = newmsg['\x02']
    
    #get all the systems info
    for i in range(0,5):
        msg = {'\x03':"SYSTEMS", '\x04':1, '\x05': [ship_id], '\x06':[i]}
        dirmsg(sock, msg)

    
    sock.close()


#osim = objectsim.ObjectSim()
#rand = random.Random()
#rand.seed(0)

#ship1 = ship.Ship(osim, type="Ship 1", port=5511)
#ship1.name = "Ship 1"
#osim.spawn_object(ship1)

#ship2 = ship.Ship(osim, type="Ship 2")
#osim.spawn_object(ship2)

if __name__ == "__main__":
    #pool_rack()
    spawn_sol()
    #signature_test()

    #test_systems()

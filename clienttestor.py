#!/usr/bin/python

import ship
import message
import socket
import objectsim
import random
import bson
import time
import math
from vector import Vector3, Vector4
from shiptypes import Firefly

def pool_rack(C = 1.0, num_rows = 5):
    sock = socket.socket()
    sock.connect( ("localhost", 5505) )

    import sys
    from math import sin, cos, pi, sqrt
    from message import SpawnMsg

    ball_mass = 15.0
    ball_radius = 1.0

#    num_rows = 50
#    C = 1.01
    y_scale = sqrt(3) / 2
    y_offset = 0.0
    nobjects = 0

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
            sm.position = [  C * (i - 2 * j) * ball_radius + random.random() * (C - 1.0), y_offset - C * y_scale * (1 + 2 * i) * ball_radius + random.random() * (C - 1.0), random.random() * (C - 1.0) ]
            print sm.position
            SpawnMsg.send(sock, None, -1, sm.build())
            nobjects += 1

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
    nobjects += 1

    print "Spawned", nobjects, "objects"
#    ret = raw_input('Press enter to continue...')

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

    #ret = raw_input('Press enter to continue...')

    sock.shutdown(socket.SHUT_RDWR)
    sock.close()

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
    print "SMARTY ID =", smarty_id

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

def flight_school(ball_radius = 1.0, num_balls = 10, z = 0.0, k = 2.0, vel_scale = 0.0):
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
    sm.radius = ball_radius

    # We need a circle of radius enough to fit the specified number of balls of the given size
    # k is some slush factor
    #
    # 2 * pi * R_c >= num_balls * ball_radius  * k
    circle_radius = k * num_balls * ball_radius

    theta = z
    for i in range(num_balls):
        sm.position = [ circle_radius * math.cos(theta), circle_radius * math.sin(theta), z ]
        sm.velocity = [ vel_scale * random.random() * math.cos(theta), vel_scale * random.random() * math.sin(theta), vel_scale * (2 * random.random() - 1) * ball_radius ]
        message.SpawnMsg.send(sock, None, -1, sm.build())
        theta += 2 * math.pi / num_balls

    sock.shutdown(socket.SHUT_RDWR)
    sock.close()


def dirmsg(sock, msg):
    message.DirectoryMsg.send(sock, 0,0, msg)
    res = sock.recv(5000)
    newmsg = bson.loads(res)
    print newmsg
    return newmsg


def test_systems():
    sock = socket.socket()
    sock.connect( ("localhost", 5506) )

    msg = {'\x03':"CLASS", '\x04':0, '\x05': {}, '\x06':{}}
    dirmsg(sock, msg)

    msg = {'\x03':"SHIP", '\x04':0, '\x05': {}, '\x06':{}}
    dirmsg(sock, msg)



    msg = {'\x03':"CLASS", '\x04':1, '\x05': [1], '\x06':[0]}
    newmsg = dirmsg(sock, msg)


    #get all the systems info
    for i in range(0,5):
        msg = {'\x03':"SYSTEMS", '\x04':1, '\x05': [ship_id], '\x06':[i]}
        dirmsg(sock, msg)


    sock.close()


def test_sensors():
    osim = objectsim.ObjectSim()
    osim.register_ship_class(Firefly)

    sock = socket.socket()
    sock.connect( ("localhost", 5506) )

    ## Create a ship instance, then join that instance, of a Firefly
    msg = message.DirectoryMsg()

    # Ask the list of classes
    msg.item_type = "CLASS"
    message.DirectoryMsg.send(sock, -1, 1, msg.build())

    # Get the list back, it'll be a DirectoryMsg
    rmsg = message.Message.get_message(sock)

    # "Select" an instance of the first item in the list.
    if len(rmsg.items) == 0:
        return
    msg.items = [ rmsg.items[0] ]
    message.DirectoryMsg.send(sock, -1, 1, msg.build())

    # The response HelloMsg is an anachronism, and we'll actually ignore it.
    rmsg = message.Message.get_message(sock)

    # Get the list of sips.
    msg.item_type = "SHIP"
    msg.items = None
    message.DirectoryMsg.send(sock, -1, 1, msg.build())

    # The response is the DirectoryMsg with the ships.
    rmsg = message.Message.get_message(sock)

    # Join the first ship.
    if len(rmsg.items) == 0:
        return
    msg.items = [ rmsg.items[0] ]
    message.DirectoryMsg.send(sock, -1, 1, msg.build())

    # The response is the HelloMsg with the server ID of the ship we joined.
    rmsg = message.Message.get_message(sock)
    ship_server_id = rmsg.srv_id

    # Now send a ready message, saying that we're ready, which will spawn the ship in the universe.
    msg = message.ReadyMsg()
    msg.ready = True
    message.ReadyMsg.send(sock, ship_server_id, 1, msg.build())

    # List the systems... We don't use this, but it's instructive.
    msg = message.DirectoryMsg()
    msg.item_type = "SYSTEMS"
    message.DirectoryMsg.send(sock, ship_server_id, 1, msg.build())

    # Now sign up for the sensors, which will come back with a full state of the system
    msg.items = [ (0, 'Sensors') ]
    message.DirectoryMsg.send(sock, ship_server_id, 1, msg.build())
    rmsg = message.Message.get_message(sock)

    msg = message.CommandMsg()
    msg.system_id = 0
    msg.system_command = "blah"
    message.CommandMsg.send(sock, ship_server_id, 1, msg.build())

    #continually return results of ping
    while (True):
        print "Waiting for results..."
        rmsg = message.Message.get_message(sock)
        print str(rmsg)
        print rmsg.__dict__



#osim = objectsim.ObjectSim()
#rand = random.Random()
#rand.seed(0)

#ship1 = ship.Ship(osim, type="Ship 1", port=5511)
#ship1.name = "Ship 1"
#osim.spawn_object(ship1)

#ship2 = ship.Ship(osim, type="Ship 2")
#osim.spawn_object(ship2)

if __name__ == "__main__":
    #pool_rack(C = 1.01, num_rows = 5)
    #spawn_sol()
    #signature_test()
    #for i in range(-30, 31, 1):
    #    flight_school(0.25, 50, i, 0.42, 0.3)

    #test_systems()
    test_sensors()

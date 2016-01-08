#!/usr/bin/python

import ship
import message
import socket
import objectsim
import random
import bson
from vector import Vector3

#def test_vis_data():
    #direction = Vector3((1.0, 0.0,0.0))
    #miss1 = ship1.fire_missile(direction, 50000.0)
    #print ship1

    #sock = socket.socket()
    #sock.connect( ("localhost", 5511) )

    #message.VisualDataEnableMsg.send(sock, 0, 0, 1)

    #while True:
        #print sock.recv(1024)

## Spawns some asteroids in a vacuum (just a socket open, no Smarty for reference)
#def spawn_some_asteroids(n = 25):
    #sock = socket.socket()
    #sock.connect( ("localhost", 5505) )

    #import sys
    #import random
    #from math import sin, cos, pi, sqrt
    #from physics import PhysicsObject
    #from message import SpawnMsg

    #rand = random.Random()
    ##rand.seed(0)

    #r = 1000
    #t = 100

    #for i in range(0, n):
        #u = rand.random() * 2 * pi
        #v = rand.random() * 2 * pi
        #c = r + (rand.random() * 2 - 1) * t
        #a = rand.random() * t

        #velocity = [ rand.random() * 5 - 2.5, rand.random() * 5 - 2.5, 0 * rand.random() * 5 - 2.5]
        #position = [ (c + a * cos(v)) * cos(u), (c + a * cos(v)) * sin(u), 0 * a * sin(v) ]
        #mass = rand.random() * 2500 + 7500
        #radius = 10 + rand.random() * 20
        #orientation = [0,0,0,0]
        #thrust = [0,0,0]
        #object_type = "Asteroid " + str(i)

        #args = [object_type, mass] + position + velocity + orientation + thrust + [radius]

        #SpawnMsg.send(sock, None, None, args)

    #sock.shutdown(socket.SHUT_RDWR)
    #sock.close()

## Spawns some asteroids in a vacuum (just a socket open, no Smarty for reference)
#def spawn_some_planets(n = 5):
    #sock = socket.socket()
    #sock.connect( ("localhost", 5505) )

    #import sys
    #import random
    #from math import sin, cos, pi, sqrt
    #from physics import PhysicsObject
    #from message import SpawnMsg

    #rand = random.Random()
    ##rand.seed(0)

    #r = 10000000
    #t = 100000

    ## These are massive enough to have gravity.
    #for i in range(0, n):
        #u = rand.random() * 2 * pi
        #v = rand.random() * 2 * pi
        #c = r + (rand.random() * 2 - 1) * t
        #a = rand.random() * t

        #velocity = [ rand.random() * 5 - 2.5, rand.random() * 5 - 2.5, rand.random() * 5 - 2.5]
        #position = [ (c + a * cos(v)) * cos(u), (c + a * cos(v)) * sin(u), a * sin(v) ]
        #mass = rand.random() * 100000000 + 1e15,
        #radius = 500000 + rand.random() * 2000000,
        #orientation = [0,0,0,0]
        #thrust = [0,0,0]
        #object_type = "Planet " + str(i)

        #args = [object_type, mass] + position + velocity + orientation + thrust + [radius]

        #SpawnMsg.send(sock, None, None, args)

    #sock.shutdown(socket.SHUT_RDWR)
    #sock.close()

#def basic_collision():
    #sock = socket.socket()
    #sock.connect( ("localhost", 5505) )

    #import sys
    #from math import sin, cos, pi, sqrt
    #from message import SpawnMsg

    #r = 10000000
    #t = 100000

    #ball_mass = 1
    #ball_radius = 1

    ## Collinear collisions
    #SpawnMsg.send(sock, None, None, [ "Cue ball", 2 * ball_mass,  10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ball_radius ])
    #SpawnMsg.send(sock, None, None, [ "Cue ball", ball_mass,  5, 0, 0, 0.5, 0, 0, 0, 0, 0, 0, 0, 0, 0, ball_radius ])
    #SpawnMsg.send(sock, None, None, [ "Cue ball", ball_mass, -10, 0, 0, 0.5, 0, 0, 0, 0, 0, 0, 0, 0, 0, ball_radius ])
    #SpawnMsg.send(sock, None, None, [ "Cue ball", 2 * ball_mass,  -5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ball_radius ])

    ## Slightly skewed
    #SpawnMsg.send(sock, None, None, [ "Cue ball", 2 * ball_mass,  10, 4.5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ball_radius ])
    #SpawnMsg.send(sock, None, None, [ "Cue ball", ball_mass,  5, 5, 0, 0.5, 0, 0, 0, 0, 0, 0, 0, 0, 0, ball_radius ])
    #SpawnMsg.send(sock, None, None, [ "Cue ball", ball_mass, -10, 5, 0, 0.5, 0, 0, 0, 0, 0, 0, 0, 0, 0, ball_radius ])
    #SpawnMsg.send(sock, None, None, [ "Cue ball", 2 * ball_mass,  -5, 4.5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ball_radius ])

    ## Some different sizes
    #SpawnMsg.send(sock, None, None, [ "Cue ball", 2 * ball_mass,  10, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 * ball_radius ])
    #SpawnMsg.send(sock, None, None, [ "Cue ball", ball_mass,  5, 10, 0, 0.5, 0, 0, 0, 0, 0, 0, 0, 0, 0, ball_radius ])
    #SpawnMsg.send(sock, None, None, [ "Cue ball", ball_mass, -10, 10, 0, 0.5, 0, 0, 0, 0, 0, 0, 0, 0, 0, ball_radius ])
    #SpawnMsg.send(sock, None, None, [ "Cue ball", 2 * ball_mass,  -5, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 * ball_radius ])

    #sock.shutdown(socket.SHUT_RDWR)
    #sock.close()

def pool_rack():
    sock = socket.socket()
    sock.connect( ("localhost", 5505) )

    import sys
    from math import sin, cos, pi, sqrt
    from message import SpawnMsg

    ball_mass = 10000.0
    ball_radius = 1.0

    num_rows = 5

    # This loop produces a trangle of balls that points down the negative y axis.
    #
    # Mathematica code.
    # numRows = 5;
    # radius = 1;
    # balls = Reap[For[i = 0, i < numRows, i++,
        # For[j = 0, j <= i, j++,
        # x = (i - 2 j) radius;
        # y = Sqrt[3]/2 (1 + 2 i) radius;
        # Sow[Circle[{x, y}, radius]]
        # ]
        # ]][[2]][[1]];
    # Graphics[balls]

    C = 1.0
    y_scale = sqrt(3) / 2
    y_offset = 100.0

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
            SpawnMsg.send(sock, None, -1, sm.build())

    # This makes us a cue ball
    sm.srv_id = -1
    sm.cli_id = -1
    sm.is_smart = False
    sm.object_type = "Cue ball"
    sm.mass = ball_mass
    sm.position = [0.0,-25.0,0.0]
    sm.velocity = [0.0,1.0,0.0]
    sm.radius = ball_radius
    SpawnMsg.send(sock, None, -1, sm.build())

    sock.shutdown(socket.SHUT_RDWR)
    sock.close()

#def all_moving():
    #sock = socket.socket()
    #sock.connect( ("localhost", 5505) )

    #num_objs = 50

    #import sys
    #import random
    #from math import sin, cos, pi, sqrt
    #from physics import PhysicsObject
    #from message import SpawnMsg

    #for i in range(0,num_objs):
        #SpawnMsg.send(sock, None, None, [ "Object %d" % i, 1000000,
        #num_objs * cos(2 * pi * i / num_objs), num_objs * sin(2 * pi * i / num_objs), 0,
        #-0.25 * num_objs * cos(2 * pi * i / num_objs), -0.25 * num_objs * sin(2 * pi * i / num_objs), 0,
        #1, 0, 0,
        #0, 0, 0,
        #1 ])

    #sock.shutdown(socket.SHUT_RDWR)
    #sock.close()

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
    for i in range(0,4):
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
    #test_vis_data()

    #spawn_some_asteroids()
    #spawn_some_planets()

    #basic_collision()
    #pool_rack()

    #all_moving()

    #spawn_sol()
    test_systems()

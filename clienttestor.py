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

# Spawns some asteroids in a vacuum (just a socket open, no Smarty for reference)
def spawn_some_asteroids(n = 25):
    sock = socket.socket()
    sock.connect( ("localhost", 5505) )

    import sys
    import random
    from math import sin, cos, pi, sqrt
    from physics import PhysicsObject
    from message import SpawnMsg

    rand = random.Random()
    #rand.seed(0)

    r = 1000
    t = 100

    for i in range(0, n):
        u = rand.random() * 2 * pi
        v = rand.random() * 2 * pi
        c = r + (rand.random() * 2 - 1) * t
        a = rand.random() * t

        velocity = [ rand.random() * 5 - 2.5, rand.random() * 5 - 2.5, rand.random() * 5 - 2.5]
        position = [ (c + a * cos(v)) * cos(u), (c + a * cos(v)) * sin(u), a * sin(v) ]
        mass = rand.random() * 2500 + 7500
        radius = 10 + rand.random() * 20
        orientation = [0,0,0]
        thrust = [0,0,0]
        object_type = "Asteroid " + str(i)

        args = [object_type, mass] + position + velocity + orientation + thrust + [radius]

        SpawnMsg.send(sock, None, None, args)

    sock.shutdown(socket.SHUT_RDWR)
    sock.close()

# Spawns some asteroids in a vacuum (just a socket open, no Smarty for reference)
def spawn_some_planets(n = 5):
    sock = socket.socket()
    sock.connect( ("localhost", 5505) )

    import sys
    import random
    from math import sin, cos, pi, sqrt
    from physics import PhysicsObject
    from message import SpawnMsg

    rand = random.Random()
    #rand.seed(0)

    r = 10000000
    t = 100000

    # These are massive enough to have gravity.
    for i in range(0, n):
        u = rand.random() * 2 * pi
        v = rand.random() * 2 * pi
        c = r + (rand.random() * 2 - 1) * t
        a = rand.random() * t

        velocity = [ rand.random() * 5 - 2.5, rand.random() * 5 - 2.5, rand.random() * 5 - 2.5]
        position = [ (c + a * cos(v)) * cos(u), (c + a * cos(v)) * sin(u), a * sin(v) ]
        mass = rand.random() * 100000000 + 1e15,
        radius = 500000 + rand.random() * 2000000,
        orientation = [0,0,0]
        thrust = [0,0,0]
        object_type = "Planet " + str(i)

        args = [object_type, mass] + position + velocity + orientation + thrust + [radius]

        SpawnMsg.send(sock, None, None, args)

    sock.shutdown(socket.SHUT_RDWR)
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
    spawn_some_asteroids()
    spawn_some_planets()

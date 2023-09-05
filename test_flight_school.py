#!/usr/bin/env python3

"""
Spawns a collection of rings of spheres around the origin, all with slow random movements, to test
the handling of collisions between them, and piloted objects.
"""

import math
import random
import socket

import message

random.seed(0)

def flight_school(ball_radius = 1.0, num_balls = 10, z = 0.0, k = 2.0, vel_scale = 0.0):
    num_spawned = 0
    sock = socket.socket()
    sock.connect( ("localhost", 5505) )

    sm = message.SpawnMsg()
    sm.srv_id = None
    sm.cli_id = -1
    sm.is_smart = False
    sm.thrust = [0.0,0.0,0.0]
    sm.velocity = [0.0,0.0,0.0]
    sm.orientation = [0.0,0.0,0.0,0.0]
    sm.object_type = "Ball"
    sm.mass = 1.0
    sm.radius = ball_radius

    # We need a circle of radius enough to fit the specified number of balls of the given size
    # k is some slush factor
    #
    # 2 * pi * R_c >= num_balls * ball_radius  * k
    circle_radius = k * num_balls * ball_radius

    theta = z + random.random()
    for i in range(num_balls):
        sm.object_type = "Ball " + str(z) + " " + str(i)
        sm.position = [ circle_radius * math.cos(theta), circle_radius * math.sin(theta), z + random.random() / 10.0 ]
        sm.velocity = [ vel_scale * random.random() * math.cos(theta), vel_scale * random.random() * math.sin(theta), vel_scale * (2 * random.random() - 1) * ball_radius ]
        sm.spectrum = message.Spectrum([i], [i])

        message.SpawnMsg.send(sock, None, -1, sm.build())
        num_spawned += 1
        theta += 2 * math.pi / num_balls

    sock.shutdown(socket.SHUT_RDWR)
    sock.close()

    return num_spawned

if __name__ == "__main__":
    num_objects = 0
    for i in range(-10, 10, 1):
        num_objects += flight_school(1.0, 20, i, 3.0, 0.3)

    print(f"Spawned {num_objects} objects")

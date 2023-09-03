#!/usr/bin/env python3

"""
Spawn a copy of Sol for gravitational simulation
"""

import socket


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
    sm.velocity = [0.0,0.0,0.0]

    phys_id = 1
    for k in sorted(objects.keys()):
        sm.object_type = objects[k]["name"]
        sm.mass = objects[k]["mass"]
        sm.radius = objects[k]["radius"]
        sm.position = objects[k]["position"]
        sm.velocity = objects[k]["velocity"]
        print(str(phys_id) + ": " + sm.object_type)
        message.SpawnMsg.send(sock, None, -1, sm.build())
        phys_id += 1

    #ret = raw_input('Press enter to continue...')

    sock.shutdown(socket.SHUT_RDWR)
    sock.close()

if __name__ == "__main__":
    spawn_sol()

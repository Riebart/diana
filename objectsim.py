#!/usr/bin/env python

import select
import socket
import sys
import thread

from mimosrv import MIMOServer

class SpaceObject:
    def __init__(self, osim, osid=0, uniid=0):
        self.osim = osim
        self.osid=osid      #the object sim id
        self.uniid=uniid    #the universe sim id
        pass

#a missile, for example
class Missile(SpaceObject):
    def __init__(self, osim, osid=0, uniid=0, thrust=0.0, typ="dummy", payload=0):
        SpaceObject.__init__(osim, osid, uniid)
        self.type = type      #annoyingly, 'type' is a python keyword
        self.thrust = thrust
        self.payload = payload

    #do a scan, for targetting purposes. Scan is a bad example, as we haven't decided yet
    #how we want to implement them
    def do_scan(self):
        pass

    def detonate(self):
        pass



class Client:
    def __init__(self, sock):
        self.sock = sock

class ObjectSim:
    def __init__(self, listen_port=5506, unisim_addr="localhost", unisim_port=5505):

        #connect to unisim
        self.unisim_sock = socket.socket()
        self.unisim_sock.connect( (unisim_addr, unisim_port) )

        #TODO:listen for clients
        self.client_net = MIMOServer(self.register_client, port = listen_port)

        self.object_list = []       #should this be a dict? using osids?
        self.ship_list = []         #likewise
        self.client_list = []
        pass


    def register_client(self, sock):
        #append new client
        self.client_list.append(Client(sock))

        #TODO: send new client some messages

    #assume object already constructed, with appropriate vals?
    def spawn_object(self, obj):
        self.object_list.append(obj)

        #TODO: give object its osid?


        #TODO: send object data to unisim
        pass


    #assumes object already 'destroyed', unregisters the object and removes it from unisim
    def destroy_object(self, osid):
        pass



if __name__ == "__main__":

    osim = ObjectSim()

    while True:
        pass

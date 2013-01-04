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
        self.typ = typ      #annoyingly, 'type' is a python keyword
        self.thrust = thrust
        self.payload = payload
        
    #do a scan, for targetting purposes
    def do_scan(self):
        #TODO: request a scan from the uni, via the os
        pass
        

class Client:
    def __init__(self, sock):
        self.sock = sock

class ObjectSim:
    def __init__(self, port=5506):
        
        #TODO:connect to unisim
        
        #TODO:listen for clients
        self.client_net = MIMOServer(self.register_client, port = port)
        
        self.object_list = []       #should this be a dict? using osids?
        self.ship_list = []         #likewise
        self.client_list = []
        pass
    
    
    def register_client(self, sock):
        #append new client
        self.client_list[] = Client(sock)
        
        #TODO: send new client some messages
        
    #assume object already constructed, with appropriate vals?
    def spawn_object(obj):
        self.object_list[] = obj
        
        #TODO: give object its osid?
        
        
        #TODO: send object data to unisim
        pass
    
    def destroy_object(obj, osid):        
        
        pass
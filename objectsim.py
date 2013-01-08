#!/usr/bin/env python

import select
import socket
import sys
import message
import threading
import time

from mimosrv import MIMOServer
from physics import Vector3

class SpaceObject:
    def __init__(self, osim, osid=0, uniid=0):
        self.osim = osim
        self.osid=osid      #the object sim id
        self.uniid=uniid    #the universe sim id
        self.mass = 0.0
        self.location = Vector3((0.0,0.0,0.0))
        self.velocity = Vector3((0.0,0.0,0.0))
        self.thrust = Vector3((0.0,0.0,0.0))
        self.orient = Vector3((0.0,0.0,0.0))
        self.radius = 0.0
        pass

#TODO: add handler for code for receiving messages asynchronously
#(I make it sound so easy!)
class SmartObject(SpaceObject, threading.Thread):
    def __init__(self, osim, osid=0, uniid=0):
        SpaceObject.__init__(self, osim, osid, uniid)
        threading.Thread.__init__(self)
        self.sock = socket.socket()
        pass
    
    
    def messageHandler(self):
        
        try:
            ret = message.Message.get_message(self.sock)
        except socket.timeout as e:
            ret = None
            
        return ret
    
    def run(self):
        while True:
            print self.messageHandler()
        





class Beam(SpaceObject):
    def __init__(self, osim, osid=0, uniid=0, type="laser", power=0.0, additional_falloff=0.0, direction=0, origin=0, focus=0.0, initial_radius=0, speed=299792458):
        SpaceObject.__init__(osim, osid, uniid)
        self.type=type
        self.power = power
        self.additional_falloff = additional_falloff
        self.direction = direction
        self.origin = origin
        self.focus = focus
        self.initial_radius = initial_radius
        self.speed = speed
        
        self.plane1 = Vector3( (0.0,0.0,0.0) )
        self.plane2 = Vector3( (0.0,0.0,0.0) )
        self.plane3 = Vector3( (0.0,0.0,0.0) )
        self.plane4 = Vector3( (0.0,0.0,0.0) )
        self.falloff = additional_falloff
        
        
    #recalculate planes and falloff, based on updated info
    #I think I need Mike to do this
    def update_vals(self):
        self.falloff = 0
        pass
        
        

#a missile, for example
class Missile(SmartObject):
    def __init__(self, osim, osid=0, uniid=0, typ="dummy", payload=0.0):
        SmartObject.__init__(self, osim, osid, uniid)
        self.type = type      #annoyingly, 'type' is a python keyword
        self.payload = payload
        self.radius = 1.0
        self.mass = 100.0
        self.tout_val = 10
        self.sock.settimeout(self.tout_val)

    #do a scan, for targetting purposes. Scan is a bad example, as we haven't decided yet
    #how we want to implement them
    def do_scan(self):
        print "Performing scan!"
        pass

    def detonate(self):
        pass
    
    def run(self):
        while True:
            
            val = self.messageHandler()
            
            #nothing happened, do a scan
            if (val == None):
                self.do_scan()
        



class Client:
    def __init__(self, sock):
        self.sock = sock

class ObjectSim:
    def __init__(self, listen_port=5506, unisim_addr="localhost", unisim_port=5505):

        #TODO:listen for clients
        self.client_net = MIMOServer(self.register_client, port = listen_port)

        self.object_list = dict()       #should this be a dict? using osids?
        self.ship_list = dict()         #likewise
        self.client_list = dict()
        self.total_objs = 0
        self.id_lock = threading.Lock()
        self.unisim = (unisim_addr, unisim_port)
        pass


    def register_client(self, sock):
        #append new client
        self.client_list.append(Client(sock))

        #TODO: send new client some messages

    #assume object already constructed, with appropriate vals?
    def spawn_object(self, obj):        
        #give object its osid
        self.id_lock.acquire()
        self.total_objs += 1
        obj.osid = self.total_objs
        self.id_lock.release()
        
        self.object_list[obj.osid] = obj

        #connect object to unisim
        if isinstance(obj, SmartObject):
            obj.sock.connect(self.unisim)
            
            message.HelloMsg.send(obj.sock, obj.osid)
            
            reply = message.Message.get_message(obj.sock)
            
            if not isinstance(reply, message.HelloMsg):
                #fail
                print "Fail!"
                pass
            
            else:
                obj.uniid = reply.endpoint_id
                        
            #TODO: send object data to unisim
            message.PhysicalPropertiesMsg.send(obj.sock, (
                obj.mass,
                obj.location[0], obj.location[1], obj.location[2],
                obj.velocity[0], obj.velocity[1], obj.velocity[2],                
                obj.orient[0], obj.orient[1], obj.orient[2],
                obj.thrust[0], obj.thrust[1], obj.thrust[2],
                obj.radius
                ) )
                
            #object is prepped, hand over message handling to new object
            obj.start()
        
        else:
            print "Fail!"
            #do what? If there's no connection, how do I send data?
            #will non-smart objects be multiplexed over a single osim connection (probably)
            pass
        

    #assumes object already 'destroyed', unregisters the object and removes it from unisim
    def destroy_object(self, osid):
        del self.object_list[osid]
        pass

    def enable_visdata(self, osid):
        return message.VisualDataEnableMsg.send(self.object_list[osid].sock, 1)

    def disable_visdata(self, osid):
        return message.VisualDataEnableMsg.send(self.object_list[osid].sock, 0)
    
    def set_thrust(self, osid, x, y=None, z=None):
        if (y==None):
            return self.set_thrust(osid, thrust[0], thrust[1], thrust[2])
        return message.PhysicalPropertiesMsg.send(obj.sock, ( 
            "",
            "", "", "",
            "", "", "",
            "", "", "",
            x, y, z,
            ""
            ) )
        pass
    
    def set_orientation(self, osid, x, y=None, z=None):
        if (y==None):
            return self.set_orientation(osid, orient[0], orient[1], orient[2])
        return message.PhysicalPropertiesMsg.send(obj.sock, ( 
            "",
            "", "", "",
            "", "", "",
            x, y, z,
            "", "", "",
            ""
            ) )
        pass


if __name__ == "__main__":

    osim = ObjectSim()

    while True:
        pass

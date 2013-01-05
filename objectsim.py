#!/usr/bin/env python

import select
import socket
import sys
import message
import threading

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


class SmartObject(SpaceObject):
    def __init__(self, osim, osid=0, uniid=0):
        SpaceObject.__init__(self, osim, osid, uniid)
        self.sock = socket.socket()
        pass


#a missile, for example
class Missile(SmartObject):
    def __init__(self, osim, osid=0, uniid=0, typ="dummy", payload=0.0):
        SmartObject.__init__(self, osim, osid, uniid)
        self.type = type      #annoyingly, 'type' is a python keyword
        self.payload = payload
        self.radius = 1.0
        self.mass = 100.0

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
        #return message.
        pass
    
    def set_orientation(self, osid, x, y=None, z=None):
        if (y==None):
            return self.set_orientation(osid, orient[0], orient[1], orient[2])
        #return message.
        pass


if __name__ == "__main__":

    osim = ObjectSim()

    while True:
        pass

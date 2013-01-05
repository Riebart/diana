#!/usr/bin/env python

import select
import socket
import sys
import message
import threading

from mimosrv import MIMOServer

class SpaceObject:
    def __init__(self, osim, osid=0, uniid=0):
        self.osim = osim
        self.osid=osid      #the object sim id
        self.uniid=uniid    #the universe sim id
        pass


class SmartObject(SpaceObject):
    def __init__(self, osim, osid=0, uniid=0):
        SpaceObject.__init__(self, osim, osid, uniid)
        self.sock = socket.socket()
        pass


#a missile, for example
class Missile(SmartObject):
    def __init__(self, osim=0, osid=0, uniid=0, thrust=0.0, typ="dummy", payload=0):
        SmartObject.__init__(self, osim, osid, uniid)
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
            #TODO: get reply, and parse unisim id
                        
        #TODO: send object data to unisim
            #message.PhysicalPropertiesMessage.send(obj.sock,
                #obj.
        
        else:
            #do what? If there's no connection, how do I send data?
            pass
        

    #assumes object already 'destroyed', unregisters the object and removes it from unisim
    def destroy_object(self, osid):
        del self.object_list[osid]
        pass

    def enable_visdata(self, osid):
        return message.VisualMetaDataEnableMsg.send(self.object_list[osid].sock, 1)
        #step 1: get 
        
    def disable_visdata(self, osid):
        return message.VisualMetaDataEnableMsg.send(self.object_list[osid].sock, 0)
    
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

#!/usr/bin/env python

import socket
import message
import threading

from mimosrv import MIMOServer
from physics import Vector3
from spaceobj import SmartObject

class Client:
    def __init__(self, sock):
        self.sock = sock

class ObjectSim:
    def __init__(self, listen_port=5506, unisim_addr="localhost", unisim_port=5505):

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
        sock.send("Hi There! Thanks for connecting to the object simulator! There is nothing to do here, yet.")


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
            
            message.HelloMsg.send(obj.sock, None, obj.osid, obj.osid)

            try:
                reply, uniid, osid = message.Message.get_message(obj.sock)
            except TypeError:
                print "Fail2!"
                return
            
            if not isinstance(reply, message.HelloMsg):
                #fail
                print "Fail!"
                pass
            
            else:
                obj.uniid = uniid
                        
            #TODO: send object data to unisim
            message.PhysicalPropertiesMsg.send(obj.sock, obj.uniid, obj.osid, (
                obj.type,
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




if __name__ == "__main__":

    osim = ObjectSim()

    while True:
        pass

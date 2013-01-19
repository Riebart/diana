#!/usr/bin/env python

import socket
import message
import threading

from mimosrv import MIMOServer
from vector import Vector3
from spaceobj import SmartObject

class Client:
    def __init__(self, sock):
        self.sock = sock

class ObjectSim:
    def __init__(self, listen_port=5506, unisim_addr="localhost", unisim_port=5505):

        self.client_net = MIMOServer(self.handler, self.hangup, port = listen_port)

        self.object_list = dict()       #should this be a dict? using osids?
        self.ship_list = dict()         #likewise
        self.client_list = dict()
        self.total_objs = 0
        self.id_lock = threading.Lock()
        self.unisim = (unisim_addr, unisim_port)
        pass

    # This is always called from a client's thread, so we don't actually need
    # to be concerned about how long this takes necessarily, as it will only hold
    # up one socket.
    def handler(self, client):
        try:
            msg = Message.get_message(client)
        except:
            return None

        osim_id = msg.srv_id
        client_id = msg.cli_id

        if osim_id == None:
            if client_id == None:
                # We have a brand new client
                client_id = len(self.client_list)
                c = Client(client, client_id)
                self.client_list[client_id] = c
                pass
            else:
                # We have a client that hasn't picked a ship yet
                pass
        else:
            # This client has picked a ship, so just pass that message off to it.
            pass

    def hangup(self, client):
        pass


    #assume object already constructed, with appropriate vals
    def spawn_object(self, obj):        
        #give object its osid
        self.id_lock.acquire()
        obj.osid = self.total_objs
        self.total_objs += 1
        self.id_lock.release()
        
        self.object_list[obj.osid] = obj

        #connect object to unisim
        if isinstance(obj, SmartObject):
            obj.sock.connect(self.unisim)
            
            message.HelloMsg.send(obj.sock, None, obj.osid)

            try:
                reply = message.Message.get_message(obj.sock)
            except TypeError:
                print "Fail2!"
                return

            uniid = reply.srv_id
            osid = reply.cli_id
            
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
                
            if obj.tout_val > 0:
                obj.sock.settimeout(obj.tout_val)
                
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

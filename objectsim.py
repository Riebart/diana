#!/usr/bin/env python

import socket
import message
import threading

from mimosrv import MIMOServer
from vector import Vector3
from spaceobj import SmartObject

from message import HelloMsg, DirectoryMsg

class Client:
    def __init__(self, sock, client_id):
        self.sock = sock
        self.client_id = client_id

class ObjectSim:
    def __init__(self, listen_port=5506, unisim_addr="localhost", unisim_port=5505):

        self.client_net = MIMOServer(self.handler, self.hangup, port = listen_port)

        self.object_list = dict()       #should this be a dict? using osids?
        self.ship_list = dict()         #likewise
        self.ship_classes = dict()
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

        # Client connecting up and saying hi.
        if isinstance(msg, HelloMsg):
            # Say Hello back with its client ID
            self.id_lock.acquire()
            client_id = self.get_id()
            c = Client(client, client_id)
            self.client_list[client_id] = c
            self.id_lock.release()
            HelloMsg.send(client, None, client_id)

        elif isinstance(msg, DirectoryMsg):
            # ### TODO ### Update clients sitting at the directories as joinable
            # ships become available?

            # A directory message with zero items means it wants an update on
            # joinable ships or ship classes
            if len(msg.items) == 0:
                DirectoryMsg.send(client, None, client_id,
                [ "SHIP" ] + self.get_joinable_ships() if msg.item_type == "SHIP" else
                [ "CLASS" ] + self.get_player_ship_classes())

            # If they send back one item, then they have made a choice.
            elif len(msg.items) == 1:
                if msg.item_type == "SHIP":
                    # They chose a ship, so take the ID, and hand off.
                    HelloMsg.send(client, msg.items[0][0], client_id)
                    self.ship_list[msg.items[0][0]].new_client(client, client_id)

                elif msg.item_type == "CLASS":
                    # They chose a class, so take the class ID and hand off.
                    newship = self.christen_ship(msg.items[0][0])
                    HelloMsg.send(client, newship.osim_id, client_id)
                    newship.new_client(client, client_id)

        elif osim_id != None and client_id != None:
            self.ship_list[osim_id].handle(client, msg)

    def hangup(self, client):
        pass

    def get_id(self):
        osim_id = self.total_objs
        self.total_objs += 1
        return osim_id

    def get_joinable_ships(self):
        joinables = []
        for s_key in self.ship_list:
            if self.ship_list[s_key].is_joinable():
                joinables.append([s_key, self.ship_list[s_key].name])

        return joinables

    def get_player_ship_classes(self):
        classes = []
        for c_key in self.ship_classes:
            classes.append([c_key, self.ship_classes[c_key].name])

        return classes

    def christen_ship(self, class_id):
        self.id_lock.acquire()

        osim_id = self.get_id()
        newship = self.ship_classes[class_id](self, osim_id)
        self.ship_list[osim_id] = newship
        self.object_list[osim_id] = newship

        self.id_lock.release()

        return newship

    #assume object already constructed, with appropriate vals
    def spawn_object(self, obj):
        self.id_lock.acquire()
        obj.osid = self.get_id()
        self.id_lock.release()
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

#!/usr/bin/env python

import socket
import message
import threading

from mimosrv import MIMOServer
from vector import Vector3
from spaceobj import SmartObject

from message import Message, HelloMsg, DirectoryMsg, GoodbyeMsg

class ObjectSim:
    def __init__(self, listen_port=5506, unisim_addr="localhost", unisim_port=5505):

        self.client_net = MIMOServer(self.handler, self.hangup, port = listen_port)

        self.object_list = dict()       #should this be a dict? using osim_ids?
        self.ship_list = dict()         #likewise
        self.ship_classes = dict()
        self.client_list = dict() # A dict on osim_id that is a list of its known clients
        self.total_objs = 0
        self.id_lock = threading.Lock()
        self.unisim = (unisim_addr, unisim_port)
        self.client_net.start()

    def stop_net(self):
        self.client_net.stop()

    def start_net(self):
        self.client_net.start()

    # This is always called from a client's thread, so we don't actually need
    # to be concerned about how long this takes necessarily, as it will only hold
    # up one socket.
    def handler(self, client):
        try:
            msg = Message.get_message(client)
        except:
            return None

        if msg == None:
            return None

        osim_id = msg.srv_id
        client_id = msg.cli_id

        # Client connecting up and saying hi.
        if isinstance(msg, HelloMsg):
            # Say Hello back with its client ID
            self.id_lock.acquire()
            client_id = self.get_id()
            self.id_lock.release()
            HelloMsg.send(client, None, client_id)

        elif isinstance(msg, DirectoryMsg):
            # ### TODO ### Update clients sitting at the directories as joinable
            # ships or classes become available?

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
                    self.client_list[msg.items[0][0]].append([client, client_id])
                    HelloMsg.send(client, msg.items[0][0], client_id)
                    self.ship_list[msg.items[0][0]].new_client(client, client_id)

                elif msg.item_type == "CLASS":
                    # They chose a class, so take the class ID and hand off.
                    newship = self.christen_ship(msg.items[0][0])
                    HelloMsg.send(client, newship.osim_id, client_id)
                    self.client_list[newship.osim_id] = [[client, client_id]]
                    newship.new_client(client, client_id)

        elif osim_id != None and client_id != None:
            self.ship_list[osim_id].handle(client, msg)

    def hangup(self, client):
        # ### TODO ### Really inefficient...
        for k in self.client_list:
            for c in self.client_list[k]:
                if c == client:
                    self.object_list[k].hangup(c)
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

    def register_ship_class(self, ship_class):
        self.id_lock.acquire()
        class_id = self.get_id()
        self.ship_classes[class_id] = ship_class
        self.id_lock.release()
        return class_id

    def get_player_ship_classes(self):
        classes = []
        for c_key in self.ship_classes:
            classes.append([c_key, self.ship_classes[c_key].name])

        return classes

    def christen_ship(self, class_id):
        newship = self.ship_classes[class_id](self)
        self.ship_list[newship.osim_id] = newship
        self.object_list[newship.osim_id] = newship

        return newship

    def get_phys_id(self, sock, osim_id):
        sock.connect(self.unisim)
        message.HelloMsg.send(sock, None, osim_id)

        try:
            reply = message.Message.get_message(sock)
        except TypeError:
            print "Fail2!"
            return None

        if not isinstance(reply, message.HelloMsg):
            print "Fail!"
            return None
        else:
            return reply.srv_id

    def send_physprops(self, obj):
        if (obj.object_type == None or obj.mass == None or
            obj.radius == None or obj.position == None or
            obj.velocity == None or obj.orientation == None or obj.thrust == None):
            return None

        ret = message.PhysicalPropertiesMsg.send(obj.sock, obj.phys_id, obj.osim_id, (
                obj.object_type,
                obj.mass,
                obj.position[0], obj.position[1], obj.position[2],
                obj.velocity[0], obj.velocity[1], obj.velocity[2],
                obj.orientation[0], obj.orientation[1], obj.orientation[2],
                obj.thrust[0], obj.thrust[1], obj.thrust[2],
                obj.radius
                ) )

        return ret

    #assume object already constructed, with appropriate vals
    def spawn_object(self, obj):
        if isinstance(obj, SmartObject):
            self.send_physprops(obj)

            if obj.tout_val > 0:
                obj.sock.settimeout(obj.tout_val)

            #object is prepped, hand over message handling to new object
            obj.start()

        else:
            print "Fail!"
            #do what? If there's no connection, how do I send data?
            #will non-smart objects be multiplexed over a single osim connection (probably)

    def destroy_object(self, osim_id):
        obj = self.object_list[osim_id]
        del self.object_list[osim_id]
        del self.ship_list[osim_id]

        obj.sock.shutdown(socket.SHUT_RDWR)
        obj.sock.close()

        for c in self.client_list[osim_id]:
            GoodbyeMsg.send(c[0], osim_id, c[1])
            if c[0].fileno() != -1:
                c[0].shutdown(socket.SHUT_RDWR)

            if c[0].fileno() != -1:
                c[0].close()

        del self.client_list[osim_id]

if __name__ == "__main__":
    from shiptypes import Firefly

    osim = ObjectSim(unisim_addr = "localhost")
    osim.register_ship_class(Firefly)

    print "Press Enter to close the server..."
    raw_input()
    print "Stopping network"
    osim.stop_net()
    print "Stopped"

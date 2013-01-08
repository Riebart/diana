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

    #create and launch a beam object. Assumes beam object is already populated with proper values
    def fire_beam(self, beam):
        beam.send_it()
    
    def handle_collision(self):
        pass
    
    def enable_visdata(self):
        return message.VisualDataEnableMsg.send(self.sock, 1)

    def disable_visdata(self):
        return message.VisualDataEnableMsg.send(self.sock, 0)
    
    def set_thrust(self, x, y=None, z=None):
        if (y==None):
            return self.set_thrust(thrust[0], thrust[1], thrust[2])
        return message.PhysicalPropertiesMsg.send(self.sock, ( 
            "",
            "", "", "",
            "", "", "",
            "", "", "",
            x, y, z,
            ""
            ) )
        pass
    
    def set_orientation(self, x, y=None, z=None):
        if (y==None):
            return self.set_orientation(osid, orient[0], orient[1], orient[2])
        return message.PhysicalPropertiesMsg.send(self.sock, ( 
            "",
            "", "", "",
            "", "", "",
            x, y, z,
            "", "", "",
            ""
            ) )
        pass    
    
    def run(self):
        #TODO: properly parse and branch wrt message recieved
        while True:
            mess = self.messageHandler()
            
            if isinstance(mess, message.CollisionMessage):
                print "Collision! " + mess
                
            else:
                print mess
        





class Beam(SpaceObject):
    def __init__(self, osim, osid=0, uniid=0, type="WEAP", power=0.0, velocity=None, origin=None, up=None, h_focus=0.0, v_focus=0.0):
        SpaceObject.__init__(self, osim, osid, uniid)
        self.type=type
        self.power = power
        if velocity != None:
            self.velocity = velocity
        else:
            self.velocity = Vector3((299792458.0, 0.0, 0.0))
        if origin != None:
            self.origin = origin
        else:
            self.origin = Vector3((0.0,0.0,0.0))
        if up != None:
            self.up = up
        else:
            self.up = Vector3((0.0,0.0,0.0))
        self.h_focus = h_focus
        self.v_focus = v_focus
        
    def build_common(self):
        return ([self.origin[0], self.origin[1], self.origin[2],
                self.velocity[0], self.velocity[1], self.velocity[2],
                self.up[0], self.up[1], self.up[2],
                self.h_focus,
                self.v_focus,
                self.power,
                self.type ])
                
         
    def send_it(self, sock):
        message.Beam.send(sock, self.build_common())   
        
        
class CommBeam(Beam):
    def __init__(self, osim, osid=0, uniid=0, type="COMM", power=0.0, velocity=None, origin=None, up=None, h_focus=0.0, v_focus=0.0, message=""):
        Beam.__init__(self, osim, osid, uniid, type, power, velocity, origin, up, h_focus, v_focus)
        self.message = message
    
    def send_it(self, sock):
        message.Beam.send(sock, self.build_common().append(self.message))
        
        
class WeaponBeam(Beam):
    def __init__(self, osim, osid=0, uniid=0, type="WEAP", power=0.0, velocity=None, origin=None, up=None, h_focus=0.0, v_focus=0.0, subtype="laser"):
        Beam.__init__(self, osim, osid, uniid, type, power, velocity, origin, up, h_focus, v_focus)
        self.subtype = subtype
        
    def send_it(self, sock):
        message.Beam.send(sock, self.build_common().append(self.subtype))
        
class ScanBeam(Beam):
    def __init__(self, osim, osid=0, uniid=0, type="SCAN", power=0.0, velocity=None, origin=None, up=None, h_focus=0.0, v_focus=0.0):
        Beam.__init__(self, osim, osid, uniid, type, power, velocity, origin, up, h_focus, v_focus)
            

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
        print str(self) + " Performing scan!"
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




if __name__ == "__main__":

    osim = ObjectSim()

    while True:
        pass

#!/usr/bin/env python

from vector import Vector3
from spaceobj import *
import math
from mimosrv import MIMOServer
import message
import time
from observer import Observable

import sys

class Sensors(Observable):
    def __init__(self, num_beams = 10, power = 10000.0, recharge_time=2.0):
        Observable.__init__(self)
        self.contacts = []
        self.scanners = []
        self.fade_time = 5.0
        
        for i in xrange(0,num_beams):
            self.scanners.append(Laser(i, power, 2*math.pi, 2*math.pi, Vector3(1,0,0), recharge_time))
            
    def perform_scan():
        pass
    
    def send_state(self, client):
        cur_time = time.time()
        
        for contact in contacts:
            if cur_time - contact.time_seen > self.fade_time:
                self.contacts.remove(contact)
            else:
                msg = "SENSORS\n"                
                #bug here that I don't want to fix. Using ';' as delimiter, plus an extra at front
                msg = msg + ";" + contact
                #TODO: figure out how cient and server ids are relevant here
                Message.InfoUpdateMsg.sendall(client, 0, 0, [msg])
                    
    def send_update(self, client, contact):
        msg = "SENSORS\n%s" % str(contact)
        #TODO: figure out how cient and server ids are relevant here
        Message.InfoUpdateMsg.sendall(client, 0, 0, [msg])
        
    def handle_scanresult(self, mess):
        #on reception of a scan result, check if contact is in the contact_list,
        #and add or update it
        #if (mess.object_type in self.contact):
            #pass
        #else:
        contact = Contact(mess.object_type, Vector3(mess.position), Vector3(mess.velocity), mess.radius )
        contact.other_data = mess.extra_parms
        self.contact_list[contact.name] = contact

        self.notify(contact)


class Comms(Observable):
    def __init__(self, power=10000.0, recharge_time=2.0):
        Observable.__init__(self)
        self.messages = []
        # What the hell was the 'i' supposed to mean? Why are there lasers in the
        # Comm observable object? Commenting this out so that I can continue
        # testing the new-ship code.
        #self.beam = Laser(i, power, 2*math.pi, 2*math,pi, Vector3(1,0,0), recharge_time)
        self.fade_time = 600.0
        
    def send_state(self, client):
        cur_time = time.time()
        
        for message in messages:
            if cur_time - message.time_seen > self.fade_time:
                self.contacts.remove(contact)
            else:
                msg = "COMMS\n"
                #bug here that I don't want to fix. Using ';' as delimiter, plus an extra at front
                msg = msg + ";" + message
                #TODO: figure out how cient and server ids are relevant here
                Message.InfoUpdateMsg.sendall(client, 0, 0, [msg])
                    
    
    def send_update(self, client, message):
        msg = "COMMS\n%s" % str(message)
        Message.InfoUpdateMsg.sendall(client, 0, 0, [msg])
        
        
    def handle_message(self, mess):
        self.messages.append(CommMessage(mess))
        self.notify(self.messages[-1])
    
#basically a struct. Better than organizing it otherwise
class CommMessage:
    def __init__(self, mess):
        self.energy = mess.energy
        self.direction = mess.direction
        self.msg = mess.msg
        self.position = mess.position
        self.time_seen = time.time()
        
    def __repr__(self):
        return ("%f,%f,%f,%f,%f,%s" % (self.time_seen, 
            self.direction[0], self.direction[1], self.direction[2],
            self.energy, self.msg))

class Contact:
    def __init__(self, name, position, velocity, radius, time_seen=0):
        self.name = name
        if (time_seen == 0):
            self.time_seen = time.time()
        else:            
            self.time_seen = time_seen
        self.position = position
        self.velocity = velocity
        self.radius = radius
        self.other_data = ""
        
    def __repr__(self):
        return (self.name + "," +
            str(time.time() - self.time_seen) + "," +
            self.position[0] + "," + self.position[1] + "," + self.position[2] + "," +
            self.velocity[0] + "," + self.velocity[1] + "," + self.velocity[2] + "," +
            self.radius + "," +
            self.other_data)
            

class Laser:
    def __init__(self, bank_id, power, h_arc, v_arc, direction, recharge_time):
        self.bank_id = bank_id
        self.power = power
        self.h_arc = h_arc
        self.v_arc = v_arc
        self.direction = direction
        self.recharge_time = recharge_time
        self.time_fired = time.time()-recharge_time
        
    #check if beam is ready to fire, and update the time_fired to current time
    def fire(self):
        cur_time = time.time()
        if self.time_fired+self.recharge_time < cur_time:
            self.time_fired = cur_time
            return True
        else:
            return False

class Ship(SmartObject):
    name = "Default Player Ship"

    #If we're going to make this a super-constructor, all non-default information
    #should be moved out. Alternatively, we leave it as-is and allow this class to
    #create a dummy ship type by default.
    def __init__(self, osim):
        SmartObject.__init__(self, osim)

        self.name = None
        self.object_type = None

        #Items not common to all ships. See shiptypes.py
        self.max_missiles = 10
        self.cur_missiles = self.max_missiles
        self.max_energy = 1000
        self.cur_energy = self.max_energy
        self.health = 0
        self.num_scan_beams = 10 #might need to abstract these into separate objects
        self.scan_beam_power = 10000.0 #10kJ?
        self.scan_beam_recharge = 5.0 #5s?

        self.sensors = Sensors(10)
        self.comms = Comms()
        self.spawned = 0

        self.laser_list = dict()
        self.laser_list[0] = Laser(0, 50000.0, pi/6, pi/6, Vector3(1,0,0), 10.0)
        self.laser_list[1] = Laser(1, 10000.0, pi/4, pi/4, Vector3(1,0,0), 5.0)
        self.laser_list[2] = Laser(2, 10000.0, pi/4, pi/4, Vector3(1,0,0), 5.0)
        self.laser_list[3] = Laser(3, 5000.0, pi/4, pi/4, Vector3(-1,0,0), 5.0)
        
        self.joinable = 1

    # ++++++++++++++++++++++++++++++++
    # These are the functions that are required by the object sim of any ships
    # that are going to be player-controlled

    # Return a 1 if there is room for some other player (even as a viz client)
    # Return 0 if there is no more room.
    def is_joinable(self):
        return self.joinable

    def new_client(self, client, client_id):
        pass

    def handle(self, client, msg):
        if isinstance(msg, message.NameMsg):
            if msg.name != None:
                self.name = msg.name

        elif isinstance(msg, message.ReadyMsg):
            if msg.ready == 1 and self.spawned == 0:
                self.osim.spawn_object(self)
                self.spawned = 1

        elif isinstance(msg, message.GoodbyeMsg):
            self.osim.destroy_object(self.osim_id)
            self.done = True

        elif isinstance(msg, message.PhysicalPropertiesMsg):
            msg.srv_id = self.phys_id
            msg.cli_id = self.osim_id
            msg.sendto(self.sock)

        elif isinstance(msg, message.VisualDataEnableMsg):
            if msg.enabled == 1:
                if client not in self.vis_clients:
                    self.vis_clients[client] = msg.cli_id
                if self.vis_enabled ==  False:
                    self.enable_visdata()
                    self.vis_enabled = True
            else:
                #delete from vis_clients
                if client in self.vis_clients:
                    del self.vis_clients[client]
                if self.vis_enabled and len(self.vis_clients) < 1:
                    self.disable_visdata()
                    self.vis_enabled = False
                    
        elif isinstance(msg, message.RequestUpdateMsg):
            if msg.type == "SENSORS":
                obs_type = self.sensors
            elif msg.type == "COMMS":
                obs_type = self.comms
                
            if msg.continuous == 1:
                obs_type.add_observer(client)
            else:
                obs_type.notify_once(client)
                
        


    # ++++++++++++++++++++++++++++++++
    # Now the rest of the handler functions
    # ++++++++++++++++++++++++++++++++
    def handle_scanresult(self, mess):
        self.Sensors.handle_scanresult(mess)

    def handle_visdata(self, mess):
        mess.srv_id = self.osim_id
        for_removal = []

        for client in self.vis_clients:
            mess.cli_id = self.vis_clients[client]
            ret = mess.sendto(client)

            if ret == 0:
                for_removal.append(client)

        for client in for_removal:
            del self.vis_clients[client]

        if len(self.vis_clients) == 0:
            self.vis_enabled = False
            self.disable_visdata()

    def hangup(self, client):
        if client in self.vis_clients:
            self.vis_clients.remove(client)

            if len(self.vis_clients) == 0:
                self.disable_visdata()

    # ++++++++++++++++++++++++++++++++
    
    def handle_comm(self, mess):
        self.comms.handle_message(mess)

    def do_scan(self):
        pass

    def run(self):
        self.vis_clients = dict()
        self.vis_enabled = False

        SmartObject.run(self)

    def fire_new_laser(self, direction, h_focus, v_focus, power):

        laser = WeaponBeam(self.osim)
        self.init_beam(laser, power, 299792458.0, direction, h_focus, v_focus)
        
        self.fire_beam(laser)
        
    def fire_laser(self, bank_id, direction, h_focus, v_focus, power = None):
        if bank_id not in self.laser_list:
            return None
        
        cur_time = time.time()
        
        if self.laser_list[bank_id].time_fired + self.laser_list[bank_id].recharge_time > cur_time:
            return None
            
        if power == None:
            power = self.laser_list[bank_id].power
    
        #TODO: other checks that the beam is appropriate
        
        self.laser_list[bank_id].time_fired = cur_time
        
        lsr = WeaponBeam(self.osim)        
        self.init_beam(lsr, power, 299792458.0, direction, h_focus, v_focus)
        
        self.fire_beam(lsr)

    #fire a dumb-fire missile in a particular direction. thrust_power is a scalar
    def fire_missile(self, direction, thrust_power):
        if (self.cur_missiles > 0):
            missile = Missile(self.osim)

            #set the initial position of the missile some small distance outside the ship
            tmp = direction.ray(Vector3((0.0,0.0,0.0)))
            tmp.scale((self.radius + missile.radius) * -1.1)
            missile.position = self.position + tmp
            
            #should missile have our initial velocity?
            missile.velocity = self.velocity
            
            tmp = direction.ray(Vector3((0.0,0.0,0.0)))
            tmp.scale(thrust_power * -1)                
            missile.thrust = tmp        
            missile.orientation = [ direction.x, direction.y, 0 ]
                    
            self.osim.spawn_object(missile)
            
            self.cur_missiles -= 1
            
        
            #shouldn't really return this, but for now, testing, etc
            return missile
        
        return None
        
    def fire_homing(self, direction, thrust_power):
        missile = HomingMissile1(self.osim, direction.unit())

        #set the initial position of the missile some small distance outside the ship
        tmp = direction.ray(Vector3((0.0,0.0,0.0)))
        tmp.scale((self.radius + missile.radius) * -1.1)
        missile.position = self.position + tmp
        
        #should missile have our initial velocity?
        missile.velocity = self.velocity
        
        tmp = direction.unit()
        tmp.scale(thrust_power)                
        missile.thrust = tmp        
        missile.orientation = [ direction.x, direction.y, 0 ]
                
        self.osim.spawn_object(missile)
        
        return missile
        #self.cur_missiles -= 1
    
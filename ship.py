#!/usr/bin/env python

from vector import Vector3
from spaceobj import *
import math
from mimosrv import MIMOServer
import message
import time
import observer

import sys

class Sensors(observer.Observable):
    def __init__(self, num_beams = 10, power = 10000.0, recharge_time=2.0):
        observer.Observable.__init__(self)
        self.contacts = []
        self.scanners = []
        
        for i in xrange(0,num_beams):
            self.scanners.append(Laser(i, power, 2*math.pi, 2*math.pi, Vector3(1,0,0), recharge_time))
            

    def perform_scan():
        pass
    
    def send_state(self, client):
        data = ""
        for contact in contacts:
            data = data + "," + contact
            
        #TODO: figure out how cient and server ids are relevant here
        Message.SensorUpdateMsg.sendall(client, 0, 0, [msg])
        
    def handle_scanresult(self, mess):
        #on reception of a scan result, check if contact is in the contact_list,
        #and add or update it
        if (mess.object_type in self.contact):
            pass
        else:
            contact = Contact(mess.object_type, Vector3(mess.position), Vector3(mess.velocity), mess.radius )
            contact.other_data = mess.extra_parms
            self.contact_list[contact.name] = contact
        
        self.notify()


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
        return self.name + "," +
            (time.time() - self.time_seen) + "," +
            self.position[0] + "," + self.position[1] + "," + self.position[2] + "," +
            self.velocity[0] + "," + self.velocity[1] + "," + self.velocity[2] + "," +
            self.radius + "," +
            self.other_data
            

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

    def __init__(self, osim):
        SmartObject.__init__(self, osim)

        # ### TODO ### Random ship names on instantiation?
        self.name = None
        self.object_type = None

        self.max_missiles = 10
        self.cur_missiles = self.max_missiles
        self.max_energy = 1000
        self.cur_energy = self.max_energy
        self.health = 0
        self.num_scan_beams = 10 #might need to abstract these into separate objects
        self.scan_beam_power = 10000.0 #10kJ?
        self.scan_beam_recharge = 5.0 #5s?

        self.sensors = Sensors(10)

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
                
        elif isinstance(mess, message.VisualDataEnableMsg):
            if msg.enabled == 1:
                if client not in self.vis_clients:
                    self.vis_clients.append(client)
                    print str(len(self.vis_clients))
                if self.vis_enabled ==  False:
                    self.enable_visdata()
                    self.vis_enabled = True
            else:
                #delete from vis_clients
                if client in self.vis_clients:
                    self.vis_clients.remove(client)
                if self.vis_enabled and len(self.vis_clients) < 1:
                    self.disable_visdata()
                    self.vis_enabled = False
                    
        elif isinstance(mess, message.RequestUpdateMsg):
            if mess.type == "SENSORS":
                obs_type = self.sensors
                
            if mess.continuous == 1:
                obs_type.add_observer(client)
            else:
                obs_type.notify_once(client)


    # ++++++++++++++++++++++++++++++++
    # Now the rest of the handler functions
    # ++++++++++++++++++++++++++++++++
    def handle_scanresult(self, mess):
        self.Sensors.handle_scanresult(mess)

    def handle_visdata(self, mess):
        #as though there is no way of getting the original string...
        new_mess = "VISDATA\n%d\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n" % (
            mess.phys_id,
            mess.radius,
            mess.position[0], mess.position[1], mess.position[2],
            mess.orientation[0], mess.orientation[1], mess.orientation[2] )
        for client in self.vis_clients:
            client.send(new_mess)
    # ++++++++++++++++++++++++++++++++

    def do_scan(self):
        pass

    def run(self):
        self.vis_clients = []
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
            missile.orientation = direction
                    
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
        missile.orientation = direction
                
        self.osim.spawn_object(missile)
        
        return missile
        #self.cur_missiles -= 1
    
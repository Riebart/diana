#!/usr/bin/env python

from vector import Vector3
from spaceobj import *
import math
from mimosrv import MIMOServer
import message
import time
        
import sys

class Contact:
    def __init__(self, name, location, velocity, radius, age=0.0):
        self.name = name
        self.age = age #should it be age (time *since* last seen), or just time last seen?
        self.location = location
        self.velocity = velocity
        self.radius = radius
        self.other_data = ""
        
class Laser:
    def __init__(self, bank_id, power, h_arc, v_arc, direction, recharge_time):
        self.bank_id = bank_id
        self.power = power
        self.h_arc = h_arc
        self.v_arc = v_arc
        self.direction = direction
        self.recharge_time = recharge_time
        self.time_fired = 0

class Ship(SmartObject):
    def __init__(self, osim, osid=0, uniid=0, type="dummy-ship", port=None):
        SmartObject.__init__(self, osim, osid, uniid)
        self.name = "Unknown"
        self.type = type
        self.radius = 20.0 #20m?
        self.mass = 100000.0 #100 Tonnes?
        
        self.max_missiles = 10
        self.cur_missiles = self.max_missiles
        self.max_energy = 1000
        self.cur_energy = self.max_energy
        self.health = self.mass*100
        self.num_scan_beams = 10 #might need to abstract these into separate objects
        self.scan_beam_power = 10000.0 #10kJ?
        self.scan_beam_recharge = 5.0 #5s?
        self.contact = []
        
        self.contact_list = dict() #The ship keeps a list of contacts, with ageing?
        self.laser_list = dict()
        
        self.laser_list[0] = Laser(0, 50000.0, pi/6, pi/6, Vector3(1,0,0), 10.0)
        self.laser_list[1] = Laser(1, 10000.0, pi/4, pi/4, Vector3(1,0,0), 5.0)
        self.laser_list[2] = Laser(2, 10000.0, pi/4, pi/4, Vector3(1,0,0), 5.0)
        self.laser_list[3] = Laser(3, 5000.0, pi/4, pi/4, Vector3(-1,0,0), 5.0)
        
        self.listen_port = port

        
    def do_scan(self):
        pass
    
    def handle_scanresult(self, mess):
        #on reception of a scan result, check if contact is in the contact_list,
        #and add or update it
        if (mess.object_type in self.contact):
            pass
        else:
            contact = Contact(mess.object_type, Vector3(mess.position), Vector3(mess.velocity), mess.radius )
            contact.other_data = mess.extra_parms
            self.contact_list[contact.name] = contact
    
    def handle_visdata(self, mess):
        #as though there is no way of getting the original string...
        new_mess = "VISDATA\n%d\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n" % (
            mess.phys_id,
            mess.radius,
            mess.position[0], mess.position[1], mess.position[2],
            mess.orientation[0], mess.orientation[1], mess.orientation[2] )
        for client in self.vis_clients:
            client.send(new_mess)
    
    #might need some locking here
    def handle_client(self, client):
        if client not in self.clients:
            self.clients.append(client)
        
        mess = message.Message.get_message(client)

        
        if isinstance(mess, message.VisualDataEnableMsg):
            if mess.enabled == 1:
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
                    
        sys.stdout.flush()
        
    #def handle_phys(self, mess):
        #print "Ship collided with something! %d, %d" % (self.uniid, self.osid)
        #pass
        
    def run(self):
        if (self.listen_port != None):
            self.client_net = MIMOServer(self.handle_client, port = self.listen_port)
            self.clients = []
            self.vis_clients = []
            self.vis_enabled = False
            
            self.client_net.start()
            
        SmartObject.run(self)
        
    def fire_laser(self, direction, h_focus=math.pi/6, v_focus=math.pi/6, power=100.0):
        
        laser = WeaponBeam(self.osim)        
        self.init_beam(laser, power, 299792458.0, direction, h_focus, v_focus)
        
        self.fire_beam(laser)
        
    def fire_new_laser(self, bank_id, direction, h_focus, v_focus, power=-1):
        if bank_id not in self.laser_list:
            return None
        
        cur_time = time.time()
        
        if self.laser_list[bank_id].time_fired + self.laser_list[bank_id].recharge_time > cur_time:
            return None
            
        if power < 0 or power > self.laser_list[bank_id].power:
            power = self.laser_list[bank_id].power
    
        #TODO: other checks that the beam is appropriate
        
        self.laser_list[bank_id].time_fired = cur_time
        
        lsr = WeaponBeam(self.osim)        
        self.init_beam(lsr, power, 299792458.0, direction, h_focus, v_focus)
        
        self.fire_beam(lsr)
        
        pass
    
    #fire a dumb-fire missile in a particular direction. thrust_power is a scalar
    def fire_missile(self, direction, thrust_power):
        if (self.cur_missiles > 0):
            missile = Missile(self.osim)

            #set the initial position of the missile some small distance outside the ship
            tmp = direction.ray(Vector3((0.0,0.0,0.0)))
            tmp.scale((self.radius + missile.radius) * -1.1)
            missile.location = self.location + tmp
            
            #should missile have our initial velocity?
            missile.velocity = self.velocity
            
            tmp = direction.ray(Vector3((0.0,0.0,0.0)))
            tmp.scale(thrust_power * -1)                
            missile.thrust = tmp        
            missile.orient = direction
                    
            self.osim.spawn_object(missile)
            
            self.cur_missiles -= 1
            
        
            #shouldn't really return this, but for now, testing, etc
            return missile
        
        return None
        
    def fire_homing(self, direction, thrust_power):
        missile = HomingMissile1(self.osim)

        #set the initial position of the missile some small distance outside the ship
        tmp = direction.ray(Vector3((0.0,0.0,0.0)))
        tmp.scale((self.radius + missile.radius) * -1.1)
        missile.location = self.location + tmp
        
        #should missile have our initial velocity?
        missile.velocity = self.velocity
        
        tmp = direction.unit()
        tmp.scale(thrust_power)                
        missile.thrust = tmp        
        missile.orient = direction
                
        self.osim.spawn_object(missile)
        
        return missile
        #self.cur_missiles -= 1
    
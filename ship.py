#!/usr/bin/env python

from physics import Vector3
from spaceobj import *
import math

class Ship(SmartObject):
    def __init__(self, osim, osid=0, uniid=0):
        SmartObject.__init__(self, osim, osid, uniid)
        self.name = "Unkown"
        self.type = "dummy-ship"
        self.max_missiles = 10
        self.cur_missiles = self.max_missiles
        self.radius = 20.0 #20m?
        self.mass = 100000.0 #100 Tonnes?
        
    def do_scan(self):
        pass
    
    #def handle_phys(self, mess):
        #print "Ship collided with something! %d, %d" % (self.uniid, self.osid)
        #pass
        
    def fire_laser(self, direction, h_focus=math.pi/6, v_focus=math.pi/6, power=100.0):
        
        laser = WeaponBeam(self.osim)
        
        self.init_beam(laser, power, 50000.0, direction, h_focus, v_focus)
        
        self.fire_beam(laser)
    
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
        
        tmp = direction.ray(Vector3((0.0,0.0,0.0)))
        tmp.scale(thrust_power * -1)                
        missile.thrust = tmp        
        missile.orient = direction
                
        self.osim.spawn_object(missile)
        
        #self.cur_missiles -= 1
    
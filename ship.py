#!/usr/bin/env python

from physics import Vector3
from spaceobj import *

class Ship(SmartObject):
    def __init__(self, osim, osid=0, uniid=0):
        SmartObject.__init__(self, osim, osid, uniid)
        self.name = "Unkown"
        self.type = "dummy-ship"
        self.max_missiles = 10
        self.cur_missiles = self.max_missiles
        self.radius = 20
        
    def do_scan():
        pass
    
    
    def fire_beam():
        pass
    
    #fire a dumb-fire missile in a particular direction. thrust_power is a scalar
    def fire_missile(self, direction, thrust_power):
        if (self.cur_missiles > 0):
            missile = Missile(self.osim)
            
            #TODO: set the initial location of the missile some small distance of the ship,
            #to avoid collisions. Distance must be in the direction the missile wants to go
            tmp = direction.ray(Vector3((0.0,0.0,0.0)))
            tmp.scale(self.radius * -1.2)
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
        
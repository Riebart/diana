#!/usr/bin/env python

from objectsim import SmartObject, ObjectSim, Missile
from physics import Vector3

class Ship(SmartObject):
    def __init__(self, osim, osid=0, uniid=0):
        SmartObject.__init__(self, osim, osid, uniid)
        self.name = "Unkown"
        self.type = "dummy-ship"
        
    def do_scan():
        pass
    
    #fire a dumb-fire missile in a particular direction. thrust_power is a scalar
    def fire_missile(self, direction, thrust_power):
        missile = Missile(self.osim)
        
        #TODO: set the initial location of the missile some small distance of the ship,
        #to avoid collisions. Distance must be in the direction the missile wants to go
        missile.location = self.location + direction.ray(Vector3((0.0,0.0,0.0))).scale(self.radius * -1)
        
        #should missile have our initial velocity?
        missile.velocity = self.velocity
                
        missile.thrust = direction.ray(Vector3((0.0,0.0,0.0))).scale(self.thrust_power * -1)
        
        missile.orient = direction
        
        
        osim.spawn_object(missile)
        
        #shouldn't really return this, but for now, testing, etc
        return missile
        
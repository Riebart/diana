#!/usr/bin/env python

from vector import Vector3
from . ship import Ship

class Firefly(Ship):
    name = "Firefly class transport"

    def __init__(self, osim, name="Unnamed Firefly"):
        Ship.__init__(self, osim)

        self.name = "Serenity"
        self.object_type = "Ship"

        self.mass = 1841590.0
        self.radius = 37.5
        
        
        #This information should all be handled server-side,
        #and so is omitted
        self.up = Vector3([0.0, 0.0, 1.0])
        self.position = Vector3([0.0, 0.0, 0.0])
        self.velocity = Vector3([0.0, 0.0, 0.0])
        self.thrust = Vector3([0.0, 0.0, 0.0])
       
class CueBall(Ship):
    name = "CueBall"

    def __init__(self, osim, name="Unnamed CueBall"):
        Ship.__init__(self, osim)
        self.name = "Baldie"
        self.object_type = "Ship"
        self.mass = 2.0
        self.radius = 1.0

        #This information should all be handled server-side,
        #and so is omitted
        self.up = Vector3([0.0, 0.0, 1.0])
        self.position = Vector3([0.0, 0.0, 0.0])
        self.velocity = Vector3([0.0, 0.0, 0.0])
        self.thrust = Vector3([0.0, 0.0, 0.0])

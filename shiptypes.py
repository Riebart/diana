#!/usr/bin/env python

from vector import Vector3
from ship import Ship

class Firefly(Ship):
    name = "Firefly class transport"

    def __init__(self, osim):
        Ship.__init__(self, osim)

        self.name = "Serenity"
        self.object_type = "Ship"

        self.mass = 1841590.0
        self.radius = 37.5
        self.up = Vector3([0.0, 0.0, 1.0])
        self.position = Vector3([0.0, 0.0, 0.0])
        self.velocity = Vector3([0.0, 0.0, 0.0])
        self.orientation = Vector3([1.0, 0.0, 0.0])
        self.thrust = Vector3([0.0, 0.0, 0.0])

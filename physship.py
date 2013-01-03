#!/usr/bin/env python

from unisim import Universe

class PhysShip(PhysicsObject):
    def __init__(self, uni, client):
        self.uni = uni
        self.client = client

    def handle(self):
        # The other end is trying to say something we weren't expecting.
        pass

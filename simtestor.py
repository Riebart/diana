#!/usr/bin/python

from __future__ import print_function
import message
import objectsim
#import unisim
import threading
from spaceobjects.ship import ship
from spaceobjects.ship.shiptypes import Firefly
from vector import Vector3
import random
import time
from spaceobjects import spaceobj
from spaceobjects.objectmanager import SmartObjectManager
from spaceobjects.planet.planet import Planet
import sys
import math
import yaml
from pathlib import Path


print("Spawning OSIM ...")
osim = objectsim.ObjectSim()
print("Spawning OSIM ... Done")

print("Starting SOM")
st = SmartObjectManager(osim)
osim.connect_manager(st)

planet1 = Planet(osim)

planet1.parse_in(yaml.safe_load(Path('gamefiles/planets/planets.yml').read_text())["planets"][0], "Earth")
planet1.position = Vector3(0,0,0)

print(planet1)
st.add_object(planet1)

st.start()

while True:
    pass

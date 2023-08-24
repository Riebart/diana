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
import sys
import math



print("Spawning OSIM ...")
osim = objectsim.ObjectSim()
print("Spawning OSIM ... Done")

print("Starting SOM")
st = SmartObjectManager(osim)
osim.connect_manager(st)
st.start()

while True:
    pass

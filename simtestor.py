#!/usr/bin/python

from __future__ import print_function
import objectsim
from spaceobjects import spaceobj
from spaceobjects.ship import ship
from spaceobjects.objectmanager import SmartObjectManager
from spaceobjects.planet.planet import Planet
from vector import Vector3
import random
import yaml
import time
from pathlib import Path


random.seed(5)

def random_vector(rng):
    return Vector3(random.random()*rng, random.random()*rng, random.random()*rng)

def load_data(osim, key):

    #can probably do away with the list/dict distinction now
    print(f"\nLoading {key}...")
    dictdata = dict()
    listdata = []

    for res_file in Path(f'gamefiles/{key}/').glob('*.yml'):
        tmp = yaml.safe_load(res_file.read_text())[key]
        if isinstance(tmp, dict):
            dictdata = dictdata | tmp
        else:
            listdata = listdata + tmp

    if len(dictdata) > 0:
        osim.data[key] = dictdata
    else:
        osim.data[key] = listdata

    print(osim.data[key])
    print("Done!\n")



print("Spawning OSIM ...")
osim = objectsim.ObjectSim()
print("Spawning OSIM ... Done")

print("Starting SOM")
st = SmartObjectManager(osim)
osim.connect_manager(st)

osim.data = dict()

load_data(osim, 'resources')
load_data(osim, 'industries')
load_data(osim, 'races')


print("\nLoading planets...")
for res_file in Path('gamefiles/planets/').glob('*.yml'):
    planets = yaml.safe_load(res_file.read_text())['planets']
    for name, planet_data in planets.items():
        print(planet_data)
        planet = Planet(osim)
        planet.position = random_vector(1000000)
        planet.parse_in(planet_data, name)
        planet.init_econ()
        print(planet)
        st.add_object(planet)

print("Done\n")



st.start()

while True:
    time.sleep(1)
    user_input = input('Press enter to end simulation...: ')
    if user_input == '':
        break


st.done = True        
osim.stop_net()
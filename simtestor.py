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
import math
import string
from pathlib import Path


random.seed(5)

def random_vector(rng):
    return Vector3(random.random()*rng, random.random()*rng, random.random()*rng)

def load_data(dest, key):

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
        dest[key] = dictdata
    else:
        dest[key] = listdata

    print(dest[key])
    print("Done!\n")

#check to make sure that resources are defined for each industry's inputs and outputs
def validate_industries(osim):
    good = True
    for industry, values in osim.data["industries"].items():
        if values["input"] is not None:
            for input in values["input"]:
                if input not in osim.data["resources"]:
                    print(f"ERROR! Input {input} from {industry} not in resources!")
                    good = False
        for output in values["output"]:
            if output not in osim.data["resources"]:
                print(f"ERROR! Output {output} from {industry} not in resources!")
                good = False
    if not good:
        raise Exception("One or more industries have invalid inputs")

def random_planet(template_data):
    planet = Planet(osim)
    for item, value in {i: v for i, v in template_data.items() if 'min' in v}.items():
        setattr(planet, item, round(random.uniform(value['min'], value['max']), 2))

    #maybe should set some of these defaults in the constructor
    #this needs to be smarter, as it can currently not add up to 100, or exceed it
    setattr(planet, "atmosphere", dict())
    for item, value in {i: v for i, v in template_data['atmosphere'].items() if 'min' in v}.items():
        planet.atmosphere[item] = round(random.uniform(value['min'], value['max']), 2)

    setattr(planet, "mass", round((planet.radius ** 3) * math.pi * 4/3 * planet.density, 2) )
    setattr(planet, "gravity", 6.67430e-11 * planet.mass / (planet.radius**2))

    #Give it a name
    setattr(planet, "object_name", random.choice(string.ascii_uppercase) + ''.join(random.choices(string.ascii_lowercase, k=random.randrange(2, 7)) ))

    planet.position = random_vector(1000000000)

    return planet

def random_habitable(template_data, osim):
    planet = random_planet(template_data)

    total_pop = random.randrange(template_data['population']['min'], template_data['population']['max'])
    planet.population = {"human" : {
                         "lower_class" : total_pop/2,
                         "middle_class" : total_pop/3,
                         "upper_class" : total_pop/6} } #an arbitrary formula
    
    planet.trade_style = {"sharing" : random.choice(["free", "reciprocal", "closed"]),
                          "all_data" : random.choice([True, False])}
    
    #choose 5 random industries that aren't maintenance or power
    for i in random.sample([i for i in osim.data["industries"] if not set(osim.data["industries"][i]["output"]) & {"energy", "maintenance"}], 5):
        planet.industries[i] = {"quantity": random.randrange(1, 20)}

    #Add some power
    power_type = random.choice([i for i in osim.data["industries"] if set(osim.data["industries"][i]["output"]) & {"energy"}])
    planet.industries[power_type] = {"quantity" : random.randrange(1,5)}

    planet.industries["maintenance_depot"] = {"quantity" : random.randrange(1,5)}

    planet.init_econ()

    return planet

print("Spawning OSIM ...")
osim = objectsim.ObjectSim()
print("Spawning OSIM ... Done")

print("Starting SOM")
st = SmartObjectManager(osim)
osim.connect_manager(st)

osim.data = dict()

load_data(osim.data, 'resources')
load_data(osim.data, 'industries')
load_data(osim.data, 'races')

validate_industries(osim)

template_data = dict()
load_data(template_data, 'templates')

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


for i in range(0, 100):
    planet = random_planet(template_data['templates']['planet'])
    print(planet)
    st.add_object(planet)
    planet = random_habitable(template_data['templates']['habitable_planet'], osim)
    print(planet)
    st.add_object(planet)
    planet = random_habitable(template_data['templates']['station'], osim)
    planet.object_name = planet.object_name + "-station"
    print(planet)
    st.add_object(planet)
    pass

print("Done\n")



st.start()

while True:
    time.sleep(1)
    user_input = input('Press enter to end simulation...: ')
    if user_input == '':
        break


st.done = True        
osim.stop_net()
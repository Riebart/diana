from .. spaceobj import SmartObject, CommBeam
from collections import defaultdict
import math
import time
import json
from vector import Vector3

#Pulling out some constants
WAREHOUSE_DISCOUNT = 0.25
NOTIFICATION_FREQUENCY = 1

class Planet(SmartObject):
    def __init__(self, osim):
        SmartObject.__init__(self, osim, independent = False)
        self.ticks_done = 0
        self.industries = dict()
        self.population = dict()
        self.warehouse = defaultdict(lambda: 0)
        self.delayed_resources = defaultdict(lambda: 0)
        self.supplied_resources = defaultdict(lambda: 0)
        self.demanded_resources = defaultdict(lambda: 0)
        self.local_price_list = defaultdict(lambda: 1.0)
        self.known_price_list = dict()
        self.known_planets = dict()


    def init_econ(self):
        for industry, values in self.industries.items():
            values["done"] = False

            #start off with the warehouse containing enough material for each industry to 'tick' 10 times
            # if "input" in self.osim.data["industries"][industry] and self.osim.data["industries"][industry]["input"] is not None:
            #     for input, value in self.osim.data["industries"][industry]["input"].items():
            #         self.delayed_resources[input] = self.delayed_resources[input] + (float(value) * 10)
        
        # #give some resources to the pops, too
        # for pop, values in self.population.items():
        #     for pop_class, pop_count in values.items():
        #         for resource, count in self.osim.data["races"][pop]["resource_demands"].items():
        #             self.warehouse[resource] = self.warehouse[resource] + 1000

             
        print(f"Industries: {self.industries}")
        print(f"Warehouse: {self.warehouse}")
        print(f"Local prices: {self.local_price_list}")
    

    #####
    # Loop-code (code related to running the loops)
    #####
    
    #reset the economy for the next tick
        
    def do_tick(self):
        if not hasattr(self, "population"):
            return
        
        print(f"Doing econ for {self.object_name}")
        self.reset_econ()
        self.do_industries()
        self.do_populations()
        self.adjust_prices()
        print(f" Supplied resources of {self.object_name}: { {i: v for i, v in self.supplied_resources.items() if v > 0.0} }")
        print(f" Demanded resources of {self.object_name}: { {i: v for i, v in self.demanded_resources.items() if v > 0.0} }")

        print(f" Warehouse of {self.object_name}: { {i: v for i, v in self.warehouse.items() if v > 0.0} }")
        print(f" Price list of {self.object_name}: { {i: v for i, v in self.local_price_list.items() if v != 1.0} }")
        
        if self.ticks_done % NOTIFICATION_FREQUENCY == 0:
            self.alert_neighbors()
        
        self.ticks_done = self.ticks_done + 1
        

    def reset_econ(self):
        self.supplied_resources.clear()
        self.demanded_resources.clear()

        for industry, values in self.industries.items():
            values["done"] = False

        #reset the quantity of non-stock-pilable resources to zero
        for industry in { i: v for i, v in self.osim.data["resources"].items() if v and "storable" in v and v["storable"] == False}:
            self.warehouse[industry] = 0

        #need to produce a base amount of these
        # self.warehouse["energy"] = 20
        # self.warehouse["maintenance"] = 1
        # self.supplied_resources["energy"] = 20
        # self.supplied_resources["maintenance"] = 1

        #move delayed resources into the warehouse, so industries can access them
        for resource, count in self.delayed_resources.items():
            self.warehouse[resource] = self.warehouse[resource] + count

        self.delayed_resources.clear()

    def do_industries(self):
        print(f" Doing industries for {self.object_name}")

        #Can maybe improve this conditional
        while len([i for i in self.industries if self.industries[i]["done"] == False]) > 0:
        
            #1. determine which industry would generate the most wealth per
            max_industry = max({i: v for i, v in self.industries.items() if v["done"] == False}, key=self.calc_value)
            print(f" Max industry {max_industry} can produce {self.calc_value(max_industry)}")
            if self.calc_value(max_industry) <= 0:
                #no industry can produce positive value, so we're done here
                break

            #2. consume the resource(s) for that industry and produce the results
            #first, what is the maximum we can produce?
            max_ticks = self.industries[max_industry]["quantity"]
            if self.osim.data["industries"][max_industry]["input"] is not None:
                for input, quantity in self.osim.data["industries"][max_industry]["input"].items():
                    max_ticks = min(max_ticks, int(self.warehouse[input]/quantity))

                #second, consume the resources
                for resource, count in self.osim.data["industries"][max_industry]["input"].items():
                    demand = count * max_ticks
                    self.warehouse[resource] = max(self.warehouse[resource] - demand, 0)
                    self.demanded_resources[resource] = count * self.industries[max_industry]["quantity"]

            #produce the outputs
            for resource, count in self.osim.data["industries"][max_industry]["output"].items():
                supply = count * max_ticks
                self.supplied_resources[resource] = self.supplied_resources[resource] + supply
                if isinstance(self.osim.data["resources"][resource], dict) and self.osim.data["resources"][resource].get("delayed", False):
                    self.delayed_resources[resource] = self.delayed_resources[resource] + supply
                else:
                    self.warehouse[resource] = self.warehouse[resource] + supply


            #3. mark that industry as 'done'
            self.industries[max_industry]["done"] = True
            #4. repeat until industry done
        pass


    def do_populations(self):
        #for each pop, consume goods as they exist
        for pop, values in self.population.items():
            print(f" Doing pop for {pop}")
            for pop_class, pop_count in values.items():
                #TODO: Clean this up by putting it in a function, so code is not duplicated
                #print(f"  Doing class {pop_class}")
                for resource, count in self.osim.data["races"][pop]["resource_demands"].items():
                    if isinstance(count, (int, float)):
                        demand = count * pop_count
                        self.warehouse[resource] = max(self.warehouse[resource] - demand, 0)
                        self.demanded_resources[resource] = self.demanded_resources[resource] + demand

                    #do subtypes
                    elif isinstance(count, dict):
                        remaining_demand = count["demand"] * pop_count
                        while remaining_demand > 0:
                            #which ones are the cheapest_resource? Consume those
                            subtype_prices = {i: self.local_price_list[i] for i in count["subtypes"] if self.warehouse[i] > 0}
                            if len(subtype_prices) < 1:
                                break
                            cheapest_resource = min(subtype_prices)
                            print(f"   Cheapest is {cheapest_resource}")
                            consumed = min(self.warehouse[cheapest_resource], remaining_demand)
                            self.warehouse[cheapest_resource] = self.warehouse[cheapest_resource] - consumed
                            remaining_demand = remaining_demand - consumed

                        for subtype in count["subtypes"]:
                            self.demanded_resources[subtype] = self.demanded_resources[subtype] + count["demand"] * pop_count / len(count["subtypes"])

                        
                #do class-specific needs
                if self.osim.data["races"][pop]["classes"][pop_class]["resource_demands"] is not None:
                    for resource, count in self.osim.data["races"][pop]["classes"][pop_class]["resource_demands"].items():
                        if isinstance(count, (int, float)):
                            demand = count * pop_count
                            self.warehouse[resource] = max(self.warehouse[resource] - demand, 0)
                            self.demanded_resources[resource] = self.demanded_resources[resource] + demand

                        elif isinstance(count, dict):
                            remaining_demand = count["demand"] * pop_count
                            while remaining_demand > 0:
                                #which ones are the cheapest_resource? Consume those
                                subtype_prices = {i: self.local_price_list[i] for i in count["subtypes"] if self.warehouse[i] > 0}
                                if len(subtype_prices) < 1:
                                    break
                                cheapest_resource = min(subtype_prices)
                                print(f"   Cheapest is {cheapest_resource}")
                                consumed = min(self.warehouse[cheapest_resource], remaining_demand)
                                self.warehouse[cheapest_resource] = self.warehouse[cheapest_resource] - consumed
                                remaining_demand = remaining_demand - consumed

                            for subtype in count["subtypes"]:
                                self.demanded_resources[subtype] = self.demanded_resources[subtype] + count["demand"] * pop_count / len(count["subtypes"])
            
        
    def adjust_prices(self):
        for resource, value in self.osim.data["resources"].items():
            supplied = self.supplied_resources[resource]
            supplied = supplied + self.warehouse[resource] * WAREHOUSE_DISCOUNT #warehouse supplies count as 1/4, because why not
            demanded = self.demanded_resources[resource]
            
            #TODO: some sort of more sophisticated algorithm
            self.local_price_list[resource] = max(self.local_price_list[resource] - ((supplied - demanded) * 0.001), 0.1)


    #calculate the value the industry would generate, based on current prices
    def calc_value(self, industry):
        #first, what is the maximum we can produce?
        max_ticks = self.industries[industry]["quantity"]
        costs = 0
        if self.osim.data["industries"][industry]["input"] is not None:
            for input, quantity in self.osim.data["industries"][industry]["input"].items():
                max_ticks = min(max_ticks, int(self.warehouse[input]/quantity))

            for input, quantity in self.osim.data["industries"][industry]["input"].items():
                costs = costs + quantity * self.local_price_list[input] * max_ticks
            
        revenue = 0
        for output, quantity in self.osim.data["industries"][industry]["output"].items():
            revenue = revenue + quantity * self.local_price_list[output] * max_ticks
        
        return revenue - costs

    """For reference, the format of the known_planets list, in yaml:
    known_planets:
        planetX:
            bearing: Vector3
            last_communication:
                time: date
                power: power
            resources:
                res1:
                    price:
                    quantity:
                    demand:
    """
        
    #periodically update other planets in range of our current price situation
    def alert_neighbors(self):
        for planet, values in self.known_planets.items():
            comm_beam = self.init_beam(CommBeam, energy = 10000000, speed = 300000000, direction = values["bearing"], up = self.up, h_focus = math.pi *2*0.25, v_focus = math.pi *2*0.05)
            print(f"{self.object_name} Sending message to {planet} at {values['bearing']}")
            msg_contents = {"name": self.object_name, "time": time.time(), "my_prices": self.local_price_list}
            if True or self.trade_style["all_data"]:
                msg_contents["known_planets"] = self.known_price_list

            comm_beam.message = f"PRICE UPDATE {json.dumps(msg_contents)}"
            comm_beam.send_it(self.sock)

    ####
    # Handler code
    ####
        
    def handle_comm(self, msg):
        if msg.comm_msg[:len("PRICE UPDATE ")] == "PRICE UPDATE ":
            planet_update = json.loads(msg.comm_msg[len("PRICE UPDATE "):])
            planet_name = planet_update["name"]
            print(f"* Price update received at {self.object_name}: {planet_update}")

            if planet_name != self.object_name:

                #update existing info
                if planet_name in self.known_planets and self.known_planets[planet_name]["last_communication"]["time"] < time.time():
                    self.known_planets[planet_name]["last_communication"] = { "time": time.time(), "energy": msg.energy }
                    self.known_planets[planet_name]["bearing"] = Vector3(msg.direction[0], msg.direction[1], msg.direction[2])
                    self.known_planets[planet_name]["bearing"].scale(-1)
                    for resource in planet_update["my_prices"]:
                        pass

                #a new planet!
                elif planet_name not in self.known_planets:
                    self.known_planets[planet_name] = { "bearing": Vector3(msg.direction[0], msg.direction[1], msg.direction[2]), "last_communication": { "time": time.time(), "energy": msg.energy }, "resources" : planet_update["my_prices"] }


                for planet, values in planet_update["known_planets"].items():
                    if planet != self.object_name and (planet not in self.known_planets or self.known_planets[planet]["time_updated"] < time.time()):
                        self.known_planets[planet] = values
                        self.known_planets[planet]["time_updated"] = time.time()

        else:
            print(f"{self.object_name} Recieved message: {msg.comm_msg}")

    def handle_phys(self, msg):
        print(f"*** {self.object_name} Collided with something! {msg}")

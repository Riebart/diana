from .. spaceobj import SmartObject, CommBeam
from collections import defaultdict
from sortedcontainers import SortedList
from . structure import *
import math
import time
import json
import re
from vector import Vector3

#Pulling out some constants
WAREHOUSE_DISCOUNT = 0.25
NOTIFICATION_FREQUENCY = 1

class Planet(SmartObject):
    def __init__(self, osim):
        SmartObject.__init__(self, osim, independent = False)
        self.ticks_done = 0
        self.industries = dict()
        self.populations = dict()
        # self.warehouse = defaultdict(lambda: 0)
        # self.delayed_resources = defaultdict(lambda: 0)
        # self.supplied_resources = defaultdict(lambda: 0)
        # self.demanded_resources = defaultdict(lambda: 0)
        self.buy_orders = defaultdict(lambda: SortedList(key=lambda x: x.price * -1)) #The -1 reverse-sorts the list. I'm sure this hack will never come back to bite me
        self.sell_orders = defaultdict(lambda: SortedList(key=lambda x: x.price))
        self.local_price_list = defaultdict(lambda: 1.0)
        self.known_price_list = dict()
        self.known_planets = dict()

    def parse_in(self, data, name=None):
        super().parse_in(data, name)
        # print(self.industries)
        self.industries = {k: Industry(v) for k, v in self.industries.items()}
        #print( quant for quant in (classes for race, classes in self.population.items()).values() )
        print(self.population)
        #self.populations = {k: Population(v) for k, v in {a:b.items() for a, b in self.population.items()}}
        self.populations = {
            f"{outer_key},{inner_key}": Population(value)
            for outer_key, inner_dict in self.population.items()
            for inner_key, value in inner_dict.items()
        }
        print(self.populations)
        

    def init_econ(self):
        for industry, values in self.industries.items():
            values.done = False
            values.cur_tickcount = self.osim.data["industries"][industry]["ticks"]

            #start off with the warehouse containing enough material for each industry to 'tick' 10 times
            # if "input" in self.osim.data["industries"][industry] and self.osim.data["industries"][industry]["input"] is not None:
            #     for input, value in self.osim.data["industries"][industry]["input"].items():
            #         self.delayed_resources[input] = self.delayed_resources[input] + (float(value) * 10)
        
        #give some initial wealth to the pops, too
        # for pop, values in self.population.items():
        #     for pop_class, values in values.items():
        #         values.wealth = 1000
        #         values.done = False

        print(f"Industries: {self.industries}")
        #print(f"Warehouse: {self.warehouse}")

    #####
    # Loop-code (code related to running the loops)
    #####
    
    #reset the economy for the next tick
        
    def do_tick(self):
        if not hasattr(self, "populations"):
            return
        
        print(f"Doing econ for {self.object_name}")
        self.reset_econ()
        self.do_industries()
        self.do_populations()
        #self.adjust_prices()
        # print(f" Supplied resources of {self.object_name}: { {i: v for i, v in self.supplied_resources.items() if v > 0.0} }")
        # print(f" Demanded resources of {self.object_name}: { {i: v for i, v in self.demanded_resources.items() if v > 0.0} }")

        # print(f" Warehouse of {self.object_name}: { {i: v for i, v in self.warehouse.items() if v > 0.0} }")
        # print(f" Price list of {self.object_name}: { {i: v for i, v in self.local_price_list.items() if v != 1.0} }")
        re_pattern = r'\[(.*?)\]'
        print(f" Buy orders: { {i: '['+re.search(re_pattern, str(v)).group(1)+']' for i, v in self.buy_orders.items()} }")
        print(f" Sell orders: { {i: '['+re.search(re_pattern, str(v)).group(1)+']' for i, v in self.sell_orders.items()} }")

        if self.ticks_done % NOTIFICATION_FREQUENCY == 0:
            pass
            #self.alert_neighbors()
        
        self.ticks_done = self.ticks_done + 1
        

    def reset_econ(self):
        # self.supplied_resources.clear()
        # self.demanded_resources.clear()

        for industry, values in self.industries.items():
            values.done = False

        #reset the quantity of non-stock-pilable resources to zero
        # for resource in { i: v for i, v in self.osim.data["resources"].items() if v and "storable" in v and v["storable"] == False}:
        #     self.warehouse[resource] = 0

        #need to produce a base amount of these
        # self.warehouse["energy"] = 20
        # self.warehouse["maintenance"] = 1
        # self.supplied_resources["energy"] = 20
        # self.supplied_resources["maintenance"] = 1

        #move delayed resources into the warehouse, so industries can access them
        # for resource, count in self.delayed_resources.items():
        #     self.warehouse[resource] = self.warehouse[resource] + count

        # self.delayed_resources.clear()

    def do_industries(self):
        print(f" Doing industries for {self.object_name}")
        #self.do_industries_aggressive()
        self.do_industries_conservative()


    def do_industries_conservative(self):

        for industry, values in self.industries.items():
            
            total_revenue = 0
            #2a. for each output, determine how much value can be made from existing buy orders
            for output, count in self.get_industry_outputs(industry):
                total_revenue = total_revenue + self.calc_sellval(output, count)[0]

            total_cost = 0
            #2c. Determine the cost to produce from creating new buy orders
            for input, count in self.get_industry_inputs(industry):
                total_cost = total_cost + self.calc_sellval(input, count)[0] + count #The +count is because we'd have to outbid existing buy orders

            if total_revenue > total_cost:
                #Attempt to Buy inputs at market rate
                quantity_produceable = values.quantity
                for input, count in self.get_industry_inputs(industry):
                    if count == 0:
                        continue
                    #buy the necessary quantity, ensuring that we outbid current outstanding buy orders
                    target_value = 1 if len(self.buy_orders[input]) == 0 else self.buy_orders[input][0].price +1
                    self.create_buy_order(input, (count*values.quantity - values.inventory[input]), target_value, industry)
                    quantity_produceable = min(quantity_produceable, math.floor(values.inventory[input]/count) )

                if quantity_produceable > 0:
                        
                    for input, count in self.get_industry_inputs(industry):
                        values.inventory[input] = values.inventory[input] - quantity_produceable*count
                    for output, count in self.get_industry_outputs(industry):
                        values.inventory[output] = values.inventory[output] + quantity_produceable*count

            #sell our output at the market rate
            for output, count in self.get_industry_outputs(industry):
                if values.inventory[output] > 0:
                    #cancel any outstanding sell orders
                    for order in self.sell_orders[output]:
                        if order.agent == industry:
                            self.sell_orders[output].discard(order)
                    #create new ones at market rate
                    self.create_sell_order(output, values.inventory[output], self.buy_orders[output][0].price, industry)


    def do_industries_aggressive(self):

        for industry, values in self.industries.items():

            quantity_produced = values.quantity
            #check our inventory and issue buy orders for anything missing
            for input, count in self.get_industry_inputs(industry):
                if count == 0:
                    continue
                if values.inventory[input] < (count * values.quantity):
                    #buy the necessary quantity, ensuring that we outbid current outstanding buy orders
                    target_value = 1 if len(self.buy_orders[input]) == 0 else self.buy_orders[input][0].price +1
                    self.create_buy_order(input, (count*values.quantity - values.inventory[input]), target_value, industry)

                #After purchases, see how much we can produce
                quantity_produced = min(quantity_produced, math.floor(values.inventory[input]/count) )

            if quantity_produced > 0:

                for input, count in self.get_industry_inputs(industry):
                    values.inventory[input] = values.inventory[input] - quantity_produced*count
                for output, count in self.get_industry_outputs(industry):
                    values.inventory[output] = values.inventory[output] + quantity_produced*count

            #attempt to sell our inventory, if any
            for output, count in self.get_industry_outputs(industry):
                if values.inventory[output] > 0:
                    #TODO Estimate target_value based on input cost
                    target_value = 1 if len(self.sell_orders[output]) == 0 else self.sell_orders[output][0].price -1
                    self.create_sell_order(output, values.inventory[output], target_value, industry)


    def do_industries_old(self):
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


    def do_populations(self):
        for pop, attrs in self.populations.items():
            pop_race = pop.split(',')[0]
            #determine how much of each resource we want to buy
            for resource, count in self.osim.data["races"][pop_race]["resource_demands"].items():
                if isinstance(count, (int, float)):
                    demand = count * attrs.quantity
                    target_value = 1 if len(self.buy_orders[resource]) == 0 else self.buy_orders[resource][0].price +1
                    self.create_buy_order(resource, demand, target_value, pop)

    def do_populations_old(self):
        for pop, values in self.populations.items():
            print(f" Doing pop for {pop}")
            for pop_class, pop_count in values.items():
                #determine how much of each resource we want to buy
                for resource, count in self.osim.data["races"][pop]["resource_demands"].items():
                    if isinstance(count, (int, float)):
                        demand = count * pop_count
                        target_value = 1 if len(self.buy_orders[resource]) == 0 else self.buy_orders[resource][0].price +1
                        self.create_buy_order(resource, demand, target_value, pop)
                #split our wealth across the resources with buy orders


    def do_populations_old_old(self):
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


    def adjust_prices_old(self):
        for resource, value in self.osim.data["resources"].items():
            supplied = self.supplied_resources[resource]
            supplied = supplied + self.warehouse[resource] * WAREHOUSE_DISCOUNT #warehouse supplies count as 1/4, because why not
            demanded = self.demanded_resources[resource]
            
            #TODO: some sort of more sophisticated algorithm
            self.local_price_list[resource] = max(self.local_price_list[resource] - ((supplied - demanded) * 0.001), 0.1)


    #calculate the value the industry would generate, based on current prices
    def calc_value_old(self, industry):
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

    def calc_buycost(self, resource, quantity):
        return self.calc_sellval(resource, quantity, selling=False)
    
    # for a given resource and quantity, calculate how much could be earned by selling at the current prices
    # returns a tuple of (value, quantity_fulfilled)
    def calc_sellval(self, resource, quantity, selling=True):
        orig_quantity = quantity
        current_val = 0
        orders = iter(self.buy_orders[resource] if selling else self.sell_orders[resource])

        for order in orders:
            current_val = current_val + (order.price * min(quantity, order.quantity))
            quantity = quantity - min(quantity, order.quantity)
            if quantity == 0:
                break

        return (current_val, orig_quantity - quantity)

    def create_sell_order(self, resource, quantity, price, agent) -> int:
        return self.create_buy_order(resource, quantity, price, agent, buying = False)

    def create_buy_order(self, resource, quantity, price, agent, buying = True) -> int:
        orders_filled = 0
        orders = self.sell_orders[resource] if buying else self.buy_orders[resource]
        #1. Try to match as many orders at specified price
        for order in orders:
            #TODO: logic for sell orders
            if buying and order.price > price:
                break
            if price > order.price:
                break

            #Can't buy from yourself
            if order.agent == agent:
                continue

            seller = self.industries[order.agent] if order.agent in self.industries else self.populations[order.agent]
            buyer = self.industries[agent] if agent in self.industries else self.populations[agent]

            if not buying:
                buyer = self.industries[order.agent] if order.agent in self.industries else self.populations[order.agent]
                seller = self.industries[agent] if agent in self.industries else self.populations[agent]

            #remove old order from list
            orders.remove(order)

            outstanding_order = quantity - orders_filled

            print(f'  Order matched! {agent if buying else order.agent} ({buyer}) buying {min(outstanding_order, order.quantity)} {resource} from {order.agent if buying else agent} ({seller}) at {order.price}')

            #They have enough for us
            if order.quantity >= outstanding_order:

                #update the inventories. TODO invert the operation if selling
                buyer.inventory[resource] = buyer.inventory[resource] + outstanding_order
                buyer.wealth = buyer.wealth - (outstanding_order * order.price)

                seller.inventory[resource] = seller.inventory[resource] - outstanding_order
                seller.wealth = seller.wealth + (outstanding_order * order.price)

                orders_filled = quantity

                #if necessary, repost the old order with the new, reduced quantity
                if order.quantity > outstanding_order:
                    if buying:
                        orders.add(SellOrder(order.quantity - outstanding_order, order.price, order.agent))
                    else:
                        orders.add(BuyOrder(order.quantity - outstanding_order, order.price, order.agent))

            #they don't have enough
            if order.quantity < outstanding_order:
                buyer.inventory[resource] = buyer.inventory[resource] + outstanding_order
                buyer.wealth = buyer.wealth - (outstanding_order * order.price)

                orders_filled = orders_filled + order.quantity
                seller.inventory[resource] = seller.inventory[resource] - outstanding_order
                seller.wealth = seller.wealth + (outstanding_order * order.price)
            
        #2. create new orders
        if orders_filled < quantity:
            if buying:
                self.buy_orders[resource].add(BuyOrder(quantity - orders_filled, price, agent))
            else:
                self.sell_orders[resource].add(SellOrder(quantity - orders_filled, price, agent))

        return orders_filled

    #should maybe be a method on industry instead
    def get_industry_inputs(self, industry):
        return self.osim.data["industries"][industry]["input"].items()

    def get_industry_outputs(self, industry):
        return self.osim.data["industries"][industry]["output"].items()        

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
            print(f"{self.object_name} Received message: {msg.comm_msg}")

    def handle_phys(self, msg):
        print(f"*** {self.object_name} Collided with something! {msg}")



#don't really do anything at this time
class Order:
    def __init__(self, quantity=0, price=0, agent=None) -> None:
        self.quantity = quantity
        self.price = price
        self.agent = agent

    def __repr__(self):
        return f'{self.quantity} @ ${self.price} by {self.agent}'

class BuyOrder(Order):
    def __init__(self) -> None:
        super().__init__()

    def __init__(self, quantity=0, price=0, agent=None) -> None:
        super().__init__(quantity, price, agent)

class SellOrder(Order):
    def __init__(self) -> None:
        super().__init__()

    def __init__(self, quantity=0, price=0, agent=None) -> None:
        super().__init__(quantity, price, agent)

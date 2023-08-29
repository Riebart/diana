from .. spaceobj import SmartObject
from collections import defaultdict

#Pulling out some constants
WAREHOUSE_DISCOUNT = 0.25
NOTIFICATION_FREQUENCY = 5

class Planet(SmartObject):
    def __init__(self, osim):
        SmartObject.__init__(self, osim, independent = False)
        self.ticks_done = 0
        self.industries = dict()
        self.population = dict()
        self.warehouse = defaultdict(lambda: 0)
        self.local_price_list = defaultdict(lambda: 1.0)
        self.known_price_list = dict()
        self.known_planets = dict()

    def init_econ(self):
        for industry, values in self.industries.items():
            values["done"] = False

            #start off with the warehouse containing enough material for each industry to 'tick' 10 times
            if "input" in self.osim.data["industries"][industry]:
                for input, value in self.osim.data["industries"][industry]["input"].items():
                    self.warehouse[input] = self.warehouse[input] + (float(value) * 10)
        
        #give some resources to the pops, too
        for pop, values in self.population.items():
            for pop_class, pop_count in values.items():
                for resource, count in self.osim.data["races"][pop]["resource_demands"].items():
                    self.warehouse[resource] = self.warehouse[resource] + 1000

             
        print(f"Industries: {self.industries}")
        print(f"Warehouse: {self.warehouse}")
        print(f"Local prices: {self.local_price_list}")
    

    #####
    # Loop-code (code related to running the loops)
    #####
    
    #reset the economy for the next tick
        
    def do_tick(self):
        print(f"Doing econ for {self.name}")
        self.reset_econ()
        self.do_industries()
        self.do_populations()
        self.adjust_prices()
        print(f" Supplied resources of {self.object_name}: {self.supplied_resources}")
        print(f" Demanded resources of {self.object_name}: {self.demanded_resources}")

        print(f" Warehouse of {self.object_name}: {self.warehouse}")
        print(f" Price list of {self.object_name}: {self.local_price_list}")
        
        if self.ticks_done % NOTIFICATION_FREQUENCY == 0:
            self.alert_neighbors()
        
        self.ticks_done = self.ticks_done + 1
        
        
    def reset_econ(self):
        self.supplied_resources = defaultdict(lambda:0)
        self.demanded_resources = defaultdict(lambda:0)
        
        for industry, values in self.industries.items():
            values["done"] = False

        #reset the quantity of non-stock-pilable resources to zero
        for industry, values in { i: v for i, v in self.osim.data["resources"].items() if v and "storable" in v and v["storable"] == False}:
            self.warehouse[industry] = 0
            

    def do_industries(self):
        print(f" Doing industries for {self.object_name}")

        #Can maybe improve this conditional
        while len([i for i in self.industries if self.industries[i]["done"] == False]) > 0:
        
            #1. determine which industry would generate the most wealth per
            max_industry = max({i: v for i, v in self.industries.items() if v["done"] == False}, key=self.calc_value)
            print(f" Max industry {max_industry} can produce {self.calc_value(max_industry)}")
                
            #2. consume the resource(s) for that industry and produce the results
            #first, what is the maximum we can produce?
            max_ticks = self.industries[max_industry]["quantity"]
            for input, quantity in self.osim.data["industries"][max_industry]["input"].items():
                max_ticks = min(max_ticks, int(self.warehouse[input]/quantity))
            
            for resource, count in self.osim.data["industries"][max_industry]["input"].items():
                demand = count * max_ticks
                self.warehouse[resource] = max(self.warehouse[resource] - demand, 0)
                self.demanded_resources[resource] = self.demanded_resources[resource] + demand
            for resource, count in self.osim.data["industries"][max_industry]["output"].items():
                supply = count * max_ticks
                self.warehouse[resource] = self.warehouse[resource] + supply
                self.supplied_resources[resource] = self.supplied_resources[resource] + supply                
            
            #3. mark that industry as 'done'
            self.industries[max_industry]["done"] = True
            #4. repeat until industry done
        pass
        
                    
    def do_populations(self):
        #for each pop, consume goods as they exist
        for pop, values in self.population.items():
            print(f" Doing pop for {pop}")
            for pop_class, pop_count in values.items():
                #print(f"  Doing class {pop_class}")
                for resource, count in self.osim.data["races"][pop]["resource_demands"].items():
                    if isinstance(count, (int, float)):
                        demand = count * pop_count
                        self.warehouse[resource] = max(self.warehouse[resource] - demand, 0)
                        self.demanded_resources[resource] = self.demanded_resources[resource] + demand

                    else:
                        pass #how to deal with subtypes?
                        
                #do class-specific needs
                if self.osim.data["races"][pop]["classes"][pop_class]["resource_demands"] is not None:
                    for resource, count in self.osim.data["races"][pop]["classes"][pop_class]["resource_demands"].items():
                        if isinstance(count, (int, float)):
                            demand = count * pop_count
                            self.warehouse[resource] = max(self.warehouse[resource] - demand, 0)
                            self.demanded_resources[resource] = self.demanded_resources[resource] + demand
                        else:
                            pass #how to deal with subtypes?
            
        
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
        for input, quantity in self.osim.data["industries"][industry]["input"].items():
            max_ticks = min(max_ticks, int(self.warehouse[input]/quantity))      
        
        costs = 0
        for input, quantity in self.osim.data["industries"][industry]["input"].items():
            costs = costs + quantity * self.local_price_list[input] * max_ticks
        
        revenue = 0
        for output, quantity in self.osim.data["industries"][industry]["output"].items():
            revenue = revenue + quantity * self.local_price_list[input] * max_ticks
        
        return revenue - costs

        
    #periodically update other planets in range of our current price situation
    def alert_neighbors(self):
        for planet, values in self.known_planets.items():
            pass

    ####
    # Handler code
    ####
        
    def handle_comm(self, msg):
        pass


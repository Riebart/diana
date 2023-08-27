from .. spaceobj import SmartObject

class Planet(SmartObject):
    def __init__(self, osim):
        SmartObject.__init__(self, osim, independent = False)
        self.industries = dict()
        self.population = dict()
        self.warehouse = dict()
        self.local_price_list = dict()
        self.known_price_list = dict()

    def init_econ(self):
        for industry, values in self.industries.items():
            values["done"] = False

            #start off with the warehouse containing enough material for each industry to 'tick' 10 times
            if "input" in self.osim.data["industries"][industry]:
                for input, value in self.osim.data["industries"][industry]["input"].items():
                    if input in self.warehouse:
                        self.warehouse[input] = self.warehouse[input] + (float(value) * 10)
                    else:
                        self.warehouse[input] = (float(value) * 10)
        
        #give some resources to the pops, too
        for pop, values in self.population.items():
            for pop_class, pop_count in values.items():
                for resource, count in self.osim.data["races"][pop]["resource_demands"].items():
                    if resource in self.warehouse:
                        self.warehouse[resource] = self.warehouse[resource] + 1000
                    else:
                        self.warehouse[resource] = 1000
        
        #may as well initialize prices as well        
        for resource in self.osim.data["resources"]:
            if isinstance(resource, dict):
                self.local_price_list[next(iter(resource))] = 1.0
            elif isinstance(resource, str):
                self.local_price_list[resource] = 1.0
        
        print(f"Industries: {self.industries}")
        print(f"Warehouse: {self.warehouse}")
        print(f"Local prices: {self.local_price_list}")
        
    #reset the economy for the next tick    
    def reset_econ(self):
        self.supplied_resources = dict()
        self.demanded_resources = dict()
        
        for industry, values in self.industries.items():
            values["done"] = False

        pass

    def do_industries(self):
        print(f"Doing industries for {self.object_name}")

        #1. determine which industry would generate the most wealth per
        for industry in self.industries:
            print(industry)
            pass
        #2. consume the resource(s) for that industry and produce the results
        #3. mark that industry as 'done'
        #4. repeat until industry done
        pass
        
                    
    def do_populations(self):
        #for each pop, consume goods as they exist
        for pop, values in self.population.items():
            print(f"Doing pop for {pop}")
            for pop_class, pop_count in values.items():
                print(f"Doing class {pop_class}")
                for resource, count in self.osim.data["races"][pop]["resource_demands"].items():
                    if isinstance(count, (int, float)):
                        demand = count * pop_count
                        self.warehouse[resource] = max(self.warehouse[resource] - demand, 0)
                        if resource in self.demanded_resources:
                            self.demanded_resources[resource] = self.demanded_resources[resource] + demand
                        else:
                            self.demanded_resources[resource] = demand
                    else:
                        pass #how to deal with subtypes?
        
        
    def adjust_prices(self):
        #compare the supplied_resources with the consumed resources and adjust
        pass
        
    def handle_comm(self, msg):
        pass


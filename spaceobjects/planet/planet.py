from .. spaceobj import SmartObject

class Planet(SmartObject):
    def __init__(self, osim):
        SmartObject.__init__(self, osim, independent = False)
        self.industries = dict()
        self.population = dict()
        self.warehouse = dict()
        self.local_price_list = dict()
        self.known_price_list = dict()

    def do_industries(self):
        print(f"Doing industries for {self.object_name}")
        
        #0. reset industries
        self.supplied_resources = dict()
        self.demanded_resources = dict()
        
        for industry in self.industries:
            industry["done"] = False
        
        #1. determine which industry would generate the most wealth per
        for industry in self.industries:
            print(industry)
            pass
        #2. consume the resource(s) for that industry and produce the results
        #3. mark that industry as 'done'
        #4. repeat until industry done
        pass

    def init_industries(self):
        for industry, values in self.industries.items():
            values["done"] = False

            #start off with the warehouse containing enough material for each industry to 'tick' 10 times
            if "input" in self.osim.data["industries"][industry]:
                for input, value in self.osim.data["industries"][industry]["input"].items():
                    if input in self.warehouse:
                        self.warehouse[input] = self.warehouse[input] + (float(value) * 10)
                    else:
                        self.warehouse[input] = (float(value) * 10)
                    
                    
        print(self.industries)
        print(self.warehouse)
                    
                    
    def do_pop():
        #for each pop, consume goods as they exist
        pass
        
        
    def adjust_prices():
        #compare the supplied_resources with the consumed resources and adjust
        pass
        
    def handle_comm(self, msg):
        pass


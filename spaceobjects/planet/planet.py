from .. spaceobj import SmartObject

class Planet(SmartObject):
    def __init__(self, osim):
        SmartObject.__init__(self, osim, independent = False)
        self.industries = dict()
        self.population = dict()
        self.resource_inventory = dict()
        self.local_price_list = dict()
        self.known_price_list = dict()

    def do_industries(self):
        print(f"Doing industries for {self.object_name}")
        #1. determine which industry would generate the most wealth per
        for industry in self.industries:
            print(industry)
            pass
        #2. consume the resource(s) for that industry and produce the results
        #3. update the prices
        #4. mark that industry as 'done'
        #5. repeat until industry done
        pass

    def handle_comm(self, msg):
        pass

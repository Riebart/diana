from .. import SmartObject

class Planet(SmartObject):
    def __init__(self):
        SmartObject.__init__(self, indepedent = False)
        self.industries = dict()
        self.population = dict()
        self.resource_inventory = dict()
        self.local_price_list = dict()
        self.known_price_list = dict()

    def do_industries():
        pass

    def handle_comm(self, msg):
        pass


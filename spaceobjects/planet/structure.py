from collections import defaultdict

class Structure():
    def __init__(self, items = None):
        self.quantity = 1
        self.done = False
        self.wealth = 0
        self.inventory = defaultdict(lambda: 0)

    def __repr__(self):
        return f'Quantity:{self.quantity}, Wealth:{self.wealth}'


class Industry(Structure):
    def __init__(self, items = None):
        Structure.__init__(self, items)
        self.cur_tickcount = 0

        if items:
            self.quantity = items["quantity"]

    def __repr__(self):
        return f'{super().__repr__()}, Curticks:{self.cur_tickcount}'


class Population(Structure):
    def __init__(self, quantity, items=None):
        Structure.__init__(self, items)

        self.quantity = quantity
        
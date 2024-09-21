

class Structure():
    def __init__(self):
        self.quantity = 1
        self.done = False
        self.wealth = 0

    def __init__(self, items):
        self.done = False
        self.quantity = items["quantity"]

    def __str__(self):
        return f'Quantity:{self.quantity}, Wealth:{self.wealth}'


class Industry(Structure):
    def __init__(self):
        Structure.__init__(self)

    def __init__(self, items):
        Structure.__init__(self, items)


class Population(Structure):
    def __init__(self):
        Structure.__init__(self)

    def __init__(self, items):
        Structure.__init__(self, items)
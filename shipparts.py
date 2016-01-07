import math
import time

##The objects in this file are used for keeping track of the state of parts of the ship,
##in a way that facilitates BSON messaging. Very little logic is performed here.


class ShipPart():
    def __init__(self):
        self.damage_level = 0.0

    
class PowerPlant(ShipPart):
    def __init__(self, max_power = 1000000):
        ShipPart.__init__(self)
        self.max_power = max_power
    
class SublightEngine(ShipPart):
    #TODO: Add thrust direction restrictions?
    def __init__(self, max_thrust = 1000.0):
        self.max_thrust = max_thrust

#currently no different than SublightEngine    
class WarpEngine(ShipPart):
    def __init__(self, max_thrust = 1000.0):
        self.max_thrust = max_thrust

class JumpEngine(ShipPart):
    def __init__(self, jump_range = 1000.0, recharge_time = 100):
        self.jump_range = jump_range
        self.recharge_time = recharge_time
        self.time_fired = time.time()-recharge_time
        
    def jump(self):
        cur_time = time.time()
        if self.time_fired+self.recharge_time < cur_time:
            self.time_fired = cur_time
            return True
        else:
            return False


class ShieldEmitter(ShipPart):
    def __init__(self, max_power = 10000.0):
        ShipPart.__init__(self)
        self.is_up = False
        
class Weapon(ShipPart):
    def __init__(self, recharge_time):
        ShipPart.__init__(self)
        self.recharge_time = recharge_time
        self.time_fired = time.time()-recharge_time

        
    #check if beam is ready to fire, and update the time_fired to current time
    def fire(self):
        cur_time = time.time()
        if self.time_fired+self.recharge_time < cur_time:
            self.time_fired = cur_time
            return True
        else:
            return False

        
class Launcher(Weapon):
    def __init__(self, recharge_time = 10):
        Weapon.__init__(self, recharge_time)
        self.type = "Launcher"
        self.loaded_with = "None"


class BeamDevice(Weapon):
    def __init__(self, bank_id = 0, max_power = 5000, h_min = 0.0, v_min = 0.0, h_max = 2*math.pi, v_max = 2*math.pi, recharge_time=10):
        Weapon.__init__(self, recharge_time)
        self.type = "Laser"
        self.bank_id = bank_id
        self.max_power = max_power
        self.h_min = h_min
        self.v_min = v_min
        self.h_max = h_max
        self.v_max = v_max

        
class Laser(BeamDevice):
    def __init__(self, bank_id = 0, max_power = 5000, h_min = 0.0, v_min = 0.0, h_max = 2*math.pi, v_max = 2*math.pi, recharge_time=10):
        BeamDevice.__init__(self, bank_id, max_power, h_min, v_min, h_max, v_max, recharge_time)
        self.type = "Laser"


class CommEmitter(BeamDevice):
    def __init__(self, bank_id = 0, max_power = 5000, h_min = 0.0, v_min = 0.0, h_max = 2*math.pi, v_max = 2*math.pi, recharge_time=10):
        BeamDevice.__init__(self, bank_id, max_power, h_min, v_min, h_max, v_max, recharge_time)
        self.type = "Comms Emitter"


class TractorEmitter(BeamDevice):
    def __init__(self, bank_id = 0, max_power = 5000, h_min = 0.0, v_min = 0.0, h_max = 2*math.pi, v_max = 2*math.pi, recharge_time=10):
        BeamDevice.__init__(self, bank_id, max_power, h_min, v_min, h_max, v_max, recharge_time)
        self.type = "Tractor Emitter"
        
class ScanEmitter(BeamDevice):
    def __init__(self, bank_id = 0, max_power = 5000, h_min = 0.0, v_min = 0.0, h_max = 2*math.pi, v_max = 2*math.pi, recharge_time=10):
        BeamDevice.__init__(self, bank_id, max_power, h_min, v_min, h_max, v_max, recharge_time)
        self.type = "Scan Emitter"        
        
if __name__ == "__main__":
    print Laser().__dict__
    print Launcher().__dict__
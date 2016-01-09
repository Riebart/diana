import spaceobj, message
from math import pi
from vector import Vector3

class SimpleBot(spaceobj.SmartObject):
    def __init__(self, osim):
        spaceobj.SmartObject.__init__(self, osim)
        self.object_type = "Bot"
        self.done = False
        self.target_acquired = False
        self.enemy_pos = None
        
        self.states = {0: "Accelerating", 1: "Coasting", 2: "Hunting-accelerating", 3:"Hunting-coasting", 4:"Engaging"}
        self.cur_state = 0


    def run(self):
        while not self.done:

            val = self.messageHandler()

            #nothing happened, do a scan
            if (val == None):
                self.do_scan()
            elif isinstance(val, message.CollisionMsg):
                self.handle_collision(val)
            elif isinstance(val, message.ScanResultMsg):
                self.handle_scanresult(val)
            elif isinstance(mess, message.ScanQueryMsg):
                self.handle_query(mess)
                
            self.do_fsm()
                

    #do a scan in all directions
    def do_scan(self):
        scan = self.init_beam(spaceobj.ScanBeam, 10000.0, spaceobj.Beam.speed_of_light, self.forward, self.up, h_focus=2*pi, v_focus=2*pi)
        scan.send_it(self.sock)
        
    def do_fsm(self):
        pass
        
    def handle_scanresult(self, mess):
        if ("ship" in mess.object_type or "Ship" in mess.object_type or "SHIP" in mess.object_type):
            enemy_pos = Vector3(mess.position)
            enemy_vel = Vector3(mess.velocity)
            distance = enemy_pos.length()
            if distance < self.fuse+mess.radius:
                self.detonate()
            else:
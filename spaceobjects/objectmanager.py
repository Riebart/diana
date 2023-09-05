from __future__ import print_function
import threading
import multiprocessing
import threading #multiprocessing on Windows is a headache
import message
from vector import Vector3
import socket
import time

class SmartObjectManager(threading.Thread):
    def __init__(self, osim, tick_rate = 5.0):
        self.sock = socket.socket()
        self.objects = dict()
        self.done = False
        self.osim = osim
        self.tick_rate = tick_rate
        self.ticks_done = 0

        #multiprocessing.Process.__init__(self, target=self.run)
        threading.Thread.__init__(self, target=self.run)

    #add an already constructed (but not connected) SmartObject to our portfolio
    def add_object(self, obj):
        #maybe not the best place to init these, but an extra check never hurts
        if not hasattr(obj, "thrust"):
            obj.thrust = Vector3(0,0,0)
        if not hasattr(obj, "velocity"):
            obj.velocity = Vector3(0,0,0)
        if not hasattr(obj, "orientation"):
            obj.orientation = Vector3(0,0,0)

        obj.sock = self.sock
        self.objects[obj.osim_id] = obj
        self.osim.spawn_object(obj, self.sock)

    def run(self):
        print("SOM running!")
        #self.sock.settimeout(self.tick_rate/10)
        self.sock.settimeout(0.1)
        t_start = time.perf_counter()
        while not self.done:
            msg = self.messageHandler()

            #the socket timedout, either do a tick if it's time
            if msg != 0:
                self.handle_message(msg)
            if time.perf_counter() - t_start > self.tick_rate:
                print(f"\nSOM tick: {self.ticks_done}")
                self.do_ticks()
                self.ticks_done = self.ticks_done +1
                t_start = time.perf_counter()


    def messageHandler(self):
        try:
            ret = message.Message.get_message(self.sock)
        except socket.timeout as e:
            ret = 0

        return ret


    def handle_message(self, msg):
        self.objects[msg.cli_id].handle_message(msg)


    def do_ticks(self):
        for name, obj in self.objects.items():
            if hasattr(obj, "do_tick"):
                obj.do_tick()



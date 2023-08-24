from __future__ import print_function
import threading
import multiprocessing
import message
#from vector import Vector3
import socket
import time

class SmartObjectManager(multiprocessing.Process):
    def __init__(self, osim, tick_rate = 1.0):
        self.sock = socket.socket()
        self.objects = dict()
        self.done = False
        self.osim = osim
        self.tick_rate = tick_rate
        self.sock.settimeout(self.tick_rate)

        multiprocessing.Process.__init__(self, target=self.run)

    #add an already constructed (but not connected) SmartObject to our portfolio
    def add_object(self, obj):
        obj.sock = self.sock
        self.objects[obj.osim_id] = obj
        osim.spawn_object(obj)

    def run(self):
        print("SOM running!")
        while not self.done:
            msg = self.messageHandler()

            #the socket timedout, do a tick
            if msg == 0:
                self.do_industries()
            else:
                self.handle_message(msg)

    def messageHandler(self):
        try:
            ret = message.Message.get_message(self.sock)
        except socket.timeout as e:
            ret = 0

        return ret


    def handle_message(self, msg):
        self.objects[msg.cli_id].handle_message(msg)


    def do_industries(self):
        print("SOM doing industries...")
        for obj in self.objects:
            if callable(getattr(obj, do_industry, None)):
                obj.do_industry()


#!/usr/bin/env python

class Observable:
    def __init__(self):
        self.observers = []

    def notify(self, data = None):
        for observer in observers:
            if (data == None):
                self.send_state(observer)
            else:
                self.send_update(observer, data)
            
    def send_update(self, observer, data):
        pass
            
    def send_state(self, observer):
        pass
    
    def add_observer(self, observer):
        self.observers.append(observer)
        self.notify_once(observer)
        
    def remove_observer(self, observer):
        if observer in self.observers:
            self.observers.remove(observer)
            
    def notify_once(self, client):
        send_state(self, client)
        
        
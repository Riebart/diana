#!/usr/bin/env python

import bson
import message


def nest_dict(obj, classkey=None):
    if isinstance(obj, dict):
        data = {}
        for (k, v) in obj.items():
            data[k] = nest_dict(v, classkey)
        return data
    elif hasattr(obj, "_ast"):
        return nest_dict(obj._ast())
    elif hasattr(obj, "__iter__"):
        return [nest_dict(v, classkey) for v in obj]
    elif hasattr(obj, "__dict__"):
        data = dict([(key, nest_dict(value, classkey)) 
            for key, value in obj.__dict__.iteritems() 
            if not callable(value) and not key.startswith('_')])
        if classkey is not None and hasattr(obj, "__class__"):
            data[classkey] = obj.__class__.__name__
        return data
    else:
        return obj


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
        #for now, just re-send everything
        self.send_state(observer)
            
    def send_state(self, observer):
        #print nest_dict(self.__dict__)
        message.SystemUpdateMsg.send(observer, 0, 0, (nest_dict(self.__dict__)))
    
    def add_observer(self, observer):
        self.observers.append(observer)
        self.notify_once(observer)
        
    def remove_observer(self, observer):
        if observer in self.observers:
            self.observers.remove(observer)
            
    def notify_once(self, client):
        self.send_state(client)
        
        
#!/usr/bin/env python

import bson
import message
import time

#ripped this off from somwhere on StackOverflow
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
    def __init__(self, osim_id, delayed_updates = False):
        self.__observers = []
        self.__osim_id = osim_id
        self._time_updated = 0
        self._delayed_updates = delayed_updates
        self._cur_batch = 0
        self._max_batch = 10

    def notify(self, data = None):
        for observer in self.__observers:
            if (data == None):
                self.send_state(observer)
            else:
                self.send_update(observer, data)

    def send_update(self, observer, data):
        #for now, just re-send everything
        self.send_state(observer)

    def send_state(self, observer):
        cur_time = time.gmtime()
        self._cur_batch = self._cur_batch + 1
        if (not self._delayed_updates or cur_time > self._time_updated or self._cur_batch >= self._max_batch ):
            #print nest_dict(self.__dict__)
            d = dict(self.__dict__)
            #remove undesirable values
            d.pop("_Observable__observers", None)
            d.pop("_ship", None)
            d.pop("_time_updated", None)
            d.pop("_delayed_updates", None)
            d.pop("_max_batch", None)
            d.pop("_cur_batch", None)
            d.pop("_Observable__osim_id", None)
            msg = message.SystemUpdateMsg()
            msg.properties = nest_dict(d)
            message.SystemUpdateMsg.send(observer[0], self.__osim_id, observer[1], msg.build())
            self._time_updated = cur_time
            self._cur_batch = 0
        

    def add_observer(self, observer):
        self.__observers.append(observer)
        self.notify_once(observer)

    def remove_observer(self, observer):
        if observer in self.__observers:
            self.__observers.remove(observer)

    def notify_once(self, client):
        self.send_state(client)

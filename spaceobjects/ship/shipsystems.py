from ... lib.vector import Vector3, Vector4
from ... lib.observer import Observable
from .. spaceobj import ScanBeam, Beam
from . shipparts import *

import math
import json

class System(Observable):
    def __init__(self, ship, delayed_updates = False):
        Observable.__init__(self, ship.osim_id, delayed_updates)
        self._ship = ship
        self.system_id = -1
        self.controlled = 0
        self.name = "ERROR: Default system object"

    def add_observer(self, observer):
        self.controlled = self.controlled + 1
        Observable.add_observer(self, observer)

    def handle_command(self, msg):
        print("Error: Undefined ", msg)


class Contact:
    def __init__(self, name, mass, position, velocity, orientation, thrust, radius, data, rad_sig, time_seen=0):
        self.name = name
        self.mass = mass
        self.position = position
        self.velocity = velocity
        self.orientation = orientation
        self.thrust = thrust
        self.radius = radius
        self.data = data
        self.rad_sig = rad_sig

        if (time_seen == 0):
            self.time_seen = time.time()
        else:
            self.time_seen = time_seen
"""
    def __repr__(self):
        return (self.name + "," +
            str(time.time() - self.time_seen) + "," +
            self.position[0] + "," + self.position[1] + "," + self.position[2] + "," +
            self.velocity[0] + "," + self.velocity[1] + "," + self.velocity[2] + "," +
            self.radius + "," +
            self.other_data)
"""

class Sensors(System):
    def __init__(self, ship):
        System.__init__(self, ship, delayed_updates = True)
        self.name = "Sensors"
        self.contacts = dict()
        self.scanners = []
        self.fade_time = 15.0
        self.num_contacts = 0 #for the messaging

    def perform_scan():
        pass

    def send_state(self, client):
        self.update_contacts()
        System.send_state(self, client)

    def update_contacts(self):
        cur_time = time.time()

        new_contacts = dict(self.contacts)
        #remove any expired contacts before sending the current list
        for contact in self.contacts.iterkeys():
            if cur_time - self.contacts[contact].time_seen > self.fade_time:
                del new_contacts[contact]

        self.contacts = new_contacts
        self.num_contacts = len(self.contacts)

    #for now, just send complete state
    def send_update(self, client, contact):
        self.send_state(client)

    def handle_scanresult(self, mess):
        #on reception of a scan result, check if contact is in the contact_list,
        #and add or update it

        #key = (mess.object_type, mess.obj_spectrum)
        key = str(mess.object_type) + str(mess.obj_spectrum)

        if ( key in self.contacts):
            del self.contacts[key]

        contact = Contact(mess.object_type, mess.mass, Vector3(mess.position), Vector3(mess.velocity), Vector4(mess.orientation), Vector3(mess.thrust), mess.radius, mess.data, mess.obj_spectrum )
        self.contacts[key] = contact

        #in the future, perhaps just notify about what's changed
        self.notify()

    #for now, send a general 360 scan ("One. Ping. Only.")
    def handle_command(self, msg):
        print("Command received: ", msg)
        sb = self._ship.init_beam(ScanBeam, 10000, Beam.speed_of_light, self._ship.forward, self._ship.up, h_focus=2*math.pi, v_focus=math.pi)
        sb.send_it(self._ship.sock)
        print("Scan ping sent...")

class Comms(System):
    def __init__(self, ship, power=10000.0, recharge_time=2.0):
        System.__init__(self, ship)
        self.messages = []
        # What the hell was the 'i' supposed to mean? Why are there lasers in the
        # Comm observable object? Commenting this out so that I can continue
        # testing the new-ship code.
        #self.beam = Laser(i, power, 2*math.pi, 2*math,pi, Vector3(1,0,0), recharge_time)
        self.fade_time = 600.0
        self.name = "Comms"

    def send_state(self, client):
        cur_time = time.time()

        #expire old messages
        for message in self.messages:
            if cur_time - message.time_seen > self.fade_time:
                self.messages.remove(message)

        System.send_state(self, client)


    def send_update(self, client, message):
        self.send_state(client)


    def handle_message(self, mess):
        self.messages.append(CommMessage(mess))
        self.notify(self.messages[-1])

    #TODO: handle ack-ing messages, so that they are removed from the list
    def ack_message(self, message):
        pass


class Weapons(System):
    def __init__(self, ship):
        System.__init__(self, ship)
        self.name = "Weapons"
        self.cur_target = None

class Helm(System):
    def __init__(self, ship):
        System.__init__(self, ship)
        self.name = "Helm"
        self.cur_thrust = Vector3(0,0,0)
        self.throttle = 0.0

class Engineering(System):
    def __init__(self, ship):
        System.__init__(self, ship)
        self.name = "Engineering"

class Shields(System):
    def __init__(self, ship):
        System.__init__(self, ship)
        self.name = "Shields"

if __name__ == "__main__":
    print(nest_dict(Sensors()))

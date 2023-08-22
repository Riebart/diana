#!/usr/bin/env python

from __future__ import print_function
from vector import Vector3
from spaceobj import *
import math
from mimosrv import MIMOServer
import message
import time
from shipparts import *
from shipsystems import *

import sys


#basically a struct. Better than organizing it otherwise
class CommMessage:
    def __init__(self, mess):
        self.energy = mess.energy
        self.direction = mess.direction
        self.msg = mess.msg
        self.position = mess.position
        self.time_seen = time.time()

    def __repr__(self):
        return ("%f,%f,%f,%f,%f,%s" % (self.time_seen,
            self.direction[0], self.direction[1], self.direction[2],
            self.energy, self.msg))


class Ship(SmartObject):
    name = "Default Player Ship"

    #If we're going to make this a super-constructor, all non-default information
    #should be moved out. Alternatively, we leave it as-is and allow this class to
    #create a dummy ship type by default.
    def __init__(self, osim):
        SmartObject.__init__(self, osim)

        self.name = None
        self.object_type = None

        self.sensors = Sensors()
        self.comms = Comms()
        self.helm = Helm()
        self.weapons = Weapons()
        self.engineering = Engineering()
        self.spawned = 0
        
        #Currently a static dict mapping
        self.systems = { 0:self.sensors, 1:self.comms, 2:self.helm, 3:self.weapons, 4:self.engineering}
        
        self.init_weapons()
        self.init_helm()
        self.init_engineering()

        self.sensors.scanners.append(ScanEmitter(max_power = 10000, h_max=2*math.pi, v_max=2*math.pi, recharge_time = 2.0))

        self.joinable = True

    def init_engineering(self):
        self.engineering.power_plants = dict()
        self.engineering.power_plants = PowerPlant()


    def init_helm(self):
        self.helm.sub_light_engines = dict()
        self.helm.sub_light_engines = SubLightEngine()
        self.helm.warp_engines = dict()
        self.helm.warp_engines = WarpEngine()
        self.helm.jump_engines = dict()
        self.helm.jump_engines = JumpEngine()


    def init_weapons(self):
        
        #Items not common to all ships. See shiptypes.py
        self.weapons.max_missiles = 10
        self.weapons.cur_missiles = self.weapons.max_missiles
        
        self.weapons.launchers = dict()
        self.weapons.launchers[0] = Launcher()
        
        #TODO: Fix Laser() constructor and decide on methof for defining firing arcs
        self.weapons.laser_list = dict()
        self.weapons.laser_list[0] = Laser(0, 50000.0, pi/6, pi/6, Vector3(1,0,0), 10.0)
        self.weapons.laser_list[1] = Laser(1, 10000.0, pi/4, pi/4, Vector3(1,0,0), 5.0)
        self.weapons.laser_list[2] = Laser(2, 10000.0, pi/4, pi/4, Vector3(1,0,0), 5.0)
        self.weapons.laser_list[3] = Laser(3, 5000.0, pi/4, pi/4, Vector3(-1,0,0), 5.0)


    # ++++++++++++++++++++++++++++++++
    # These are the functions that are required by the object sim of any ships
    # that are going to be player-controlled
    
    #TODO: Inform client about available systems
    #NOTTODO - this is now done via directory messages
    def new_client(self, client, client_id):
        pass


    #this function handles messages from the clients
    def handle(self, client, msg):
        print("SHIP HANDLING", msg, client)
        if isinstance(msg, message.NameMsg):
            if msg.name != None:
                self.name = msg.name

        elif isinstance(msg, message.ReadyMsg):
            if msg.ready == 1 and self.spawned == 0:
                self.osim.spawn_object(self)
                self.spawned = 1

        elif isinstance(msg, message.GoodbyeMsg):
            self.osim.destroy_object(self.osim_id)
            self.done = True

        elif isinstance(msg, message.PhysicalPropertiesMsg):
            msg.srv_id = self.phys_id
            msg.cli_id = self.osim_id
            msg.sendto(self.sock)

        elif isinstance(msg, message.VisualDataEnableMsg):
            if msg.enabled:
                if client not in self.vis_clients:
                    self.vis_clients[client] = msg.cli_id
                if self.vis_enabled ==  False:
                    self.enable_visdata()
                    self.vis_enabled = True
            else:
                #delete from vis_clients
                if client in self.vis_clients:
                    del self.vis_clients[client]
                if self.vis_enabled and len(self.vis_clients) < 1:
                    self.disable_visdata()
                    self.vis_enabled = False

        elif isinstance(msg, message.RequestUpdateMsg):
            if msg.type == "SENSORS":
                obs_type = self.sensors
            elif msg.type == "COMMS":
                obs_type = self.comms

            if msg.continuous == 1:
                obs_type.add_observer(client)
            else:
                obs_type.notify_once(client)




    # ++++++++++++++++++++++++++++++++
    # Now the rest of the handler functions
    # ++++++++++++++++++++++++++++++++
    
    #these are defined by the handler function in SmartObject (spaceobj.py)
    def handle_scanresult(self, mess):
        pass
        #self.Sensors.handle_scanresult(mess)

    def handle_visdata(self, mess):
        mess.srv_id = self.osim_id
        for_removal = []

        for client in self.vis_clients:
            mess.cli_id = self.vis_clients[client]
            ret = message.VisualDataMsg.send(client, mess.srv_id, mess.cli_id, mess.build())

            if ret == 0:
                for_removal.append(client)

        for client in for_removal:
            del self.vis_clients[client]

        if len(self.vis_clients) == 0:
            self.vis_enabled = False
            self.disable_visdata()

    def hangup(self, client):
        if client in self.vis_clients:
            self.vis_clients.remove(client)

            if len(self.vis_clients) == 0:
                self.disable_visdata()

    # ++++++++++++++++++++++++++++++++

    def handle_comm(self, mess):
        self.comms.handle_message(mess)

    def do_scan(self):
        pass

    def run(self):
        self.vis_clients = dict()
        self.vis_enabled = False

        SmartObject.run(self)

    def fire_new_laser(self, direction, h_focus, v_focus, power):

        laser = WeaponBeam(self.osim)
        self.init_beam(laser, power, 299792458.0, direction, h_focus, v_focus)

        self.fire_beam(laser)

    def fire_laser(self, bank_id, direction, h_focus, v_focus, power = None):
        if bank_id not in self.laser_list:
            return None

        cur_time = time.time()

        if self.laser_list[bank_id].time_fired + self.laser_list[bank_id].recharge_time > cur_time:
            return None

        if power == None:
            power = self.laser_list[bank_id].power

        #TODO: other checks that the beam is appropriate

        self.laser_list[bank_id].time_fired = cur_time

        lsr = WeaponBeam(self.osim)
        self.init_beam(lsr, power, 299792458.0, direction, h_focus, v_focus)

        self.fire_beam(lsr)

    #fire a dumb-fire missile in a particular direction. thrust_power is a scalar
    def fire_missile(self, direction, thrust_power):
        if (self.cur_missiles > 0):
            missile = Missile(self.osim)

            #set the initial position of the missile some small distance outside the ship
            tmp = direction.ray(Vector3((0.0,0.0,0.0)))
            tmp.scale((self.radius + missile.radius) * -1.1)
            missile.position = self.position + tmp

            #should missile have our initial velocity?
            missile.velocity = self.velocity

            tmp = direction.ray(Vector3((0.0,0.0,0.0)))
            tmp.scale(thrust_power * -1)
            missile.thrust = tmp
            missile.orientation = [ direction.x, direction.y, 0 ]

            self.osim.spawn_object(missile)

            self.cur_missiles -= 1


            #shouldn't really return this, but for now, testing, etc
            return missile

        return None

    def fire_homing(self, direction, thrust_power):
        missile = HomingMissile1(self.osim, direction.unit())

        #set the initial position of the missile some small distance outside the ship
        tmp = direction.ray(Vector3((0.0,0.0,0.0)))
        tmp.scale((self.radius + missile.radius) * -1.1)
        missile.position = self.position + tmp

        #should missile have our initial velocity?
        missile.velocity = self.velocity

        tmp = direction.unit()
        tmp.scale(thrust_power)
        missile.thrust = tmp
        missile.orientation = [ direction.x, direction.y, 0 ]

        self.osim.spawn_object(missile)

        return missile
        #self.cur_missiles -= 1

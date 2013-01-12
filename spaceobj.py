import threading
import message
from physics import Vector3
import socket
from math import pi

import time

class SpaceObject:
    def __init__(self, osim, osid=0, uniid=0):
        self.type = "Dummy SpaceObject (Error!)"
        self.osim = osim
        self.osid=osid      #the object sim id
        self.uniid=uniid    #the universe sim id
        self.mass = 0.0
        self.location = Vector3((0.0,0.0,0.0))
        self.velocity = Vector3((0.0,0.0,0.0))
        self.thrust = Vector3((0.0,0.0,0.0))
        self.orient = Vector3((0.0,0.0,0.0))
        self.up = Vector3((0.0,0.0,0.0))
        self.radius = 1.0
        pass


class SmartObject(SpaceObject, threading.Thread):
    def __init__(self, osim, osid=0, uniid=0):
        SpaceObject.__init__(self, osim, osid, uniid)
        threading.Thread.__init__(self)
        self.type = "Dummy SmartObject (Error!)"
        self.sock = socket.socket()
        self.done = False
        pass
    
    
    def make_explosion(self, location, power):
        message.BeamMsg.send(self.sock, [location[0], location[1], location[2],
                299792458.0, 0.0, 0.0,
                0.0, 0.0, 0.0,
                2*pi,
                pi,
                power,
                "WEAP" ])  
    
    def init_beam(self, beam, power, speed, direction, h_focus = pi/6, v_focus = pi/6):
        direction = direction.unit()
        
        vel = direction.clone()
        #beams will currently move at 50km/s
        vel.scale(speed)
        beam.velocity=vel
        
        beam.h_focus = h_focus
        beam.v_focus = v_focus
        beam.power = power
        
        direction.scale(self.radius*1.1)
        beam.origin = self.location + direction
        
        beam.osid = self.osid
        beam.uniid = self.uniid

    
    def messageHandler(self):
        
        try:
            ret = message.Message.get_message(self.sock)
        except socket.timeout as e:
            ret = None
            
        return ret

    #create and launch a beam object. Assumes beam object is already populated with proper values
    def fire_beam(self, beam):
        beam.send_it(self.sock)
        
    def handle_phys(self, mess):
        pass
    
    def handle_weap(self, mess):
        pass
        
    def handle_comm(self, mess):
        pass
    
    def handle_scan(self, mess):
        pass
    
    def handle_scanresult(self, mess):
        pass
    
    def handle_collision(self, collision):
        if collision.collision_type == "PHYS":
            #hit by a physical object, take damage
            print "%d suffered a Physical collision!" % self.osid
            self.handle_phys(collision)
        elif collision.collision_type == "WEAP":
            #hit by a weapon, take damage
            print "%d suffered a weapon collision!" % self.osid
            self.handle_weap(collision)
        elif collision.collision_type == "COMM":
            #hit by a comm beam, perform apropriate action
            self.handle_comm(collision)
        elif collision.collision_type == "SCAN":
            #hit by a scan beam
            self.handle_scan(collision)
        elif collision.collision_type == "SCANRESULT":
            self.handle_scanresult(collision)
        pass
    
    def take_damage(self, amount):
        pass
    
    def enable_visdata(self):
        return message.VisualDataEnableMsg.send(self.sock, 1)

    def disable_visdata(self):
        return message.VisualDataEnableMsg.send(self.sock, 0)
    
    def set_thrust(self, x, y=None, z=None):
        if (y==None):
            return self.set_thrust(thrust[0], thrust[1], thrust[2])
        self.thrust = Vector3(x,y,z)
        return message.PhysicalPropertiesMsg.send(self.sock, self.uniid, self.osid( 
            "",
            "",
            "", "", "",
            "", "", "",
            "", "", "",
            x, y, z,
            ""
            ) )
        pass
    
    def set_orientation(self, x, y=None, z=None):
        if (y==None):
            return self.set_orientation(osid, orient[0], orient[1], orient[2])
        self.orient = Vector3(x,y,z)
        return message.PhysicalPropertiesMsg.send(self.sock, self.uniid, self.osid, ( 
            "",
            "",
            "", "", "",
            "", "", "",
            x, y, z,
            "", "", "",
            ""
            ) )
        pass    
    
    def die(self):
        message.GoodbyeMsg.send(self.sock, self.uniid)
        self.done = True
        self.sock.close()
    
    def run(self):
        #TODO: properly parse and branch wrt message recieved
        while not self.done:
            mess = self.messageHandler()
            
            if isinstance(mess, message.CollisionMsg):
                self.handle_collision(mess)
            elif isinstance(mess, message.VisualDataMsg):
                print str(mess)
            elif isinstance(mess, message.ScanResultMsg):
                self.handle_scanresult(mess)
                
            else:
                print str(mess)
        


    
    


class Beam(SpaceObject):
    def __init__(self, osim, osid=0, uniid=0, type="WEAP", power=0.0, velocity=None, origin=None, up=None, h_focus=0.0, v_focus=0.0):
        SpaceObject.__init__(self, osim, osid, uniid)
        self.type=type
        self.power = power
        if velocity != None:
            self.velocity = velocity
        else:
            self.velocity = Vector3((299792458.0, 0.0, 0.0))
        if origin != None:
            self.origin = origin
        else:
            self.origin = Vector3((0.0,0.0,0.0))
        if up != None:
            self.up = up
        else:
            self.up = Vector3((0.0,0.0,0.0))
        self.h_focus = h_focus
        self.v_focus = v_focus
        
    def build_common(self):
        return ([self.origin[0], self.origin[1], self.origin[2],
                self.velocity[0], self.velocity[1], self.velocity[2],
                self.up[0], self.up[1], self.up[2],
                self.h_focus,
                self.v_focus,
                self.power,
                self.type ])
                
         
    def send_it(self, sock):
        message.Beam.send(sock, self.uniid, self.osid, self.build_common())   
        
        
class CommBeam(Beam):
    def __init__(self, osim, osid=0, uniid=0, type="COMM", power=0.0, velocity=None, origin=None, up=None, h_focus=0.0, v_focus=0.0, message=""):
        Beam.__init__(self, osim, osid, uniid, type, power, velocity, origin, up, h_focus, v_focus)
        self.message = message
    
    def send_it(self, sock):
        ar = self.build_common()
        ar.append(self.message)
        message.BeamMsg.send(sock, ar, self.uniid, self.osid, ar)
        
        
class WeaponBeam(Beam):
    def __init__(self, osim, osid=0, uniid=0, type="WEAP", power=0.0, velocity=None, origin=None, up=None, h_focus=0.0, v_focus=0.0, subtype="laser"):
        Beam.__init__(self, osim, osid, uniid, type, power, velocity, origin, up, h_focus, v_focus)
        self.subtype = subtype
        
    def send_it(self, sock):
        ar = self.build_common()
        ar.append(self.subtype)
        message.BeamMsg.send(sock, self.uniid, self.osid, ar)
        
class ScanBeam(Beam):
    def __init__(self, osim, osid=0, uniid=0, type="SCAN", power=0.0, velocity=None, origin=None, up=None, h_focus=0.0, v_focus=0.0):
        Beam.__init__(self, osim, osid, uniid, type, power, velocity, origin, up, h_focus, v_focus)
            

#a dumbfire missile, for example
class Missile(SmartObject):
    def __init__(self, osim, osid=0, uniid=0, typ="dummy", payload=0.0):
        SmartObject.__init__(self, osim, osid, uniid)
        self.type = type      #annoyingly, 'type' is a python keyword
        self.payload = payload
        self.radius = 1.0
        self.mass = 100.0
        #self.sock.settimeout(self.tout_val)

    def handle_phys(self, mess):
        self.detonate()

    def detonate(self):
        self.make_explosion(self.location, self.payload)
        self.die()
    
    def run(self):
        while not self.done:
            
            val = self.messageHandler()

            if isinstance(val, message.CollisionMsg):
                self.handle_collision(val)


class HomingMissile1(Missile):
    def __init__(self, osim, osid=0, uniid=0, payload=0.0, direction=[1,0,0]):
        Missile.__init__(self, osim, osid, uniid, "HomingMissile", payload)
        self.direction = direction
        self.tout_val = 0.5
        self.sock.settimeout(self.tout_val)
        self.fuse = 20.0    #distance in meters to explode from target

    #so this is wrong. Only works if there is a single target 'in front of' the missile
    def handle_scanresult(self, mess):
        enemy_pos = Vector3(mess.position)
        distance = self.location.distance(enemy_pos)
        if distance < fuse:
            self.detonate()
        else:
            new_dir = self.location.ray(enemy_pos)
            new_dir.scale(self.thrust.length())
            self.set_thrust(new_dir)
    
    def do_scan(self, mess):
        scan = ScanBeam(self.osim)
        self.init_beam(scan, 100.0, 50000.0, self.velocity)
        scan.send_it(self.sock)

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




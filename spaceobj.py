#import threading
import multiprocessing
import message
from vector import Vector3
import socket
from math import pi, sqrt

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


#class SmartObject(SpaceObject, threading.Thread):
class SmartObject(SpaceObject, multiprocessing.Process):
    def __init__(self, osim, osid=0, uniid=0):
        SpaceObject.__init__(self, osim, osid, uniid)
        #threading.Thread.__init__(self)
        multiprocessing.Process.__init__(self)
        self.type = "Dummy SmartObject (Error!)"
        self.sock = socket.socket()
        self.done = False
        self.tout_val = 0
        pass
    
    
    def make_explosion(self, location, power):
        message.BeamMsg.send(self.sock, self.uniid, self.osid, [
                location[0], location[1], location[2],
                299792458.0, 0.0, 0.0,
                0.0, 0.0, 0.0,
                2*pi,
                2*pi,
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
        ## From Mike: This would be where you
        ## alert to the fact that "I just got my skirt looked up!"
        pass
    
    def handle_scanresult(self, mess):
        print "Got a scanresult for %s" % mess.object_type
        pass
    
    def handle_collision(self, collision):
        if collision.collision_type == "PHYS":
            #hit by a physical object, take damage
            print "%d suffered a Physical collision! %fJ!" % (self.osid, collision.energy)
            self.handle_phys(collision)
        elif collision.collision_type == "WEAP":
            #hit by a weapon, take damage
            print "%d suffered a weapon collision! %fJ!" % (self.osid, collision.energy)
            self.handle_weap(collision)
        elif collision.collision_type == "COMM":
            #hit by a comm beam, perform apropriate action
            self.handle_comm(collision)
        elif collision.collision_type == "SCAN":
            #hit by a scan beam
            self.handle_scan(collision)
        elif collision.collision_type == "SCANRESULT":
            self.handle_scanresult(collision)
    
    def make_response(self, power):
        return self.type
    
    def handle_query(self, mess):
        # This is where we respond to SCANQUERY messages.
        # return message.ScanResponseMsg.send(self.sock, self.uniid, self.osid, [ mess.scan_id, self.make_response(), mess.power ])
        return message.ScanResponseMsg.send(self.sock, self.uniid, self.osid, [ mess.scan_id, self.make_response(mess.scan_power) ])
        pass
    
    def take_damage(self, amount):
        pass
    
    def enable_visdata(self):
        return message.VisualDataEnableMsg.send(self.sock, 1)

    def disable_visdata(self):
        return message.VisualDataEnableMsg.send(self.sock, 0)
    
    def set_thrust(self, x, y=None, z=None):
        if (y==None):
            return self.set_thrust(x[0], x[1], x[2])
        self.thrust = Vector3(x,y,z)
        return message.PhysicalPropertiesMsg.send(self.sock, self.uniid, self.osid, ( 
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
        message.GoodbyeMsg.send(self.sock, self.uniid, self.osid)
        self.done = True
        self.sock.close()
    
    def run(self):
        #TODO: properly parse and branch wrt message recieved
        while not self.done:
            mess = self.messageHandler()[0]
            
            if isinstance(mess, message.CollisionMsg):
                self.handle_collision(mess)
            elif isinstance(mess, message.VisualDataMsg):
                print str(mess)
            elif isinstance(mess, message.ScanResultMsg):
                self.handle_scanresult(mess)
            elif isinstance(mess, message.ScanQueryMsg):
                self.handle_query(mess)
                
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
            self.up = Vector3((0.0,0.0,1.0))
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
        message.BeamMsg.send(sock, self.uniid, self.osid, self.build_common())   
        
        
class CommBeam(Beam):
    def __init__(self, osim, osid=0, uniid=0, type="COMM", power=0.0, velocity=None, origin=None, up=None, h_focus=0.0, v_focus=0.0, message=""):
        Beam.__init__(self, osim, osid, uniid, type, power, velocity, origin, up, h_focus, v_focus)
        self.message = message
    
    def send_it(self, sock):
        ar = self.build_common()
        ar.append(self.message)
        message.BeamMsg.send(sock, self.uniid, self.osid, ar)
        
        
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
    def __init__(self, osim, osid=0, uniid=0, type="dummy", payload=10000000.0):
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
            
            val = self.messageHandler()[0]

            if isinstance(val, message.CollisionMsg):
                self.handle_collision(val)


class HomingMissile1(Missile):
    def __init__(self, osim, osid=0, uniid=0, payload=10000000.0, direction=[1,0,0]):
        Missile.__init__(self, osim, osid, uniid, "HomingMissile", payload)
        self.direction = direction
        self.tout_val = 1
        #self.sock.settimeout(self.tout_val)
        self.fuse = 100.0    #distance in meters to explode from target

    #so this is not great. Only works if there is a single target 'in front of' the missile
    #def handle_scanresult(self, mess):
        #enemy_pos = Vector3(mess.position)
        #enemy_vel = Vector3(mess.velocity)
        ##distance = self.location.distance(enemy_pos)
        #distance = enemy_pos.length()
        #if distance < self.fuse+mess.radius:
            #self.detonate()
        #else:
            #new_dir = enemy_pos.unit()
            #new_dir.scale(self.thrust.length())
            #print ("Homing missile %d setting new thrust vector " % self.osid) + str(new_dir) + (". Distance to target: %2f" % (distance-mess.radius))
            #self.set_thrust(new_dir)
            
            
    def handle_scanresult(self, mess):
        enemy_pos = Vector3(mess.position)
        enemy_vel = Vector3(mess.velocity)
        distance = enemy_pos.length()
        if distance < self.fuse+mess.radius:
            self.detonate()
        else:
            time_to_target = sqrt(2*distance/(self.thrust.length()/self.mass))
            enemy_vel.scale(time_to_target/2) #dampen new position by 2, to prevent overshoots
            tar_new_pos = enemy_vel+enemy_pos
            #print ("Cur enemy pos: " + str(enemy_pos) + " Cur enemy velocity " + str(mess.velocity) + " new enemy pos " + str(tar_new_pos))
            new_dir = tar_new_pos.unit()
            new_dir.scale(self.thrust.length())
            print ("Homing missile %d setting new thrust vector " % self.osid) + str(new_dir) + (". Distance to target: %2f" % (distance-mess.radius))
            self.set_thrust(new_dir)
    
    def do_scan(self):
        scan = ScanBeam(self.osim)
        if (self.velocity.length() > 0):
            tmp_dir = self.velocity.unit()
        else:
            tmp_dir = self.orient
        self.init_beam(scan, 1000.0, 299792458.0, tmp_dir, h_focus=pi/3, v_focus=pi/3)
        scan.send_it(self.sock)

    def run(self):
        while not self.done:
            
            val = self.messageHandler()
            
            #nothing happened, do a scan
            if (val == None):
                self.do_scan()
            elif isinstance(val[0], message.CollisionMsg):
                self.handle_collision(val[0])
            elif isinstance(val[0], message.ScanResultMsg):
                self.handle_scanresult(val[0])




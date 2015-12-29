import threading
#import multiprocessing
import message
from vector import Vector3
import socket
from math import pi, sqrt

import time

class SpaceObject:
    def __init__(self, osim, osim_id):
        self.object_type = None
        self.osim = osim
        if osim_id == None:
            self.osim_id = self.osim.get_id()
        else:
            self.osim_id = osim_id
        self.phys_id = None

        self.mass = None
        self.position = None
        self.velocity = None
        self.thrust = None
        self.forward = Vector3([1,0,0])
        self.up = Vector3([0,0,1])
        self.right = Vector3([0,1,0])
        self.radius = None

class SmartObject(SpaceObject, threading.Thread):
#class SmartObject(SpaceObject, multiprocessing.Process):
    def __init__(self, osim, osim_id = None):
        SpaceObject.__init__(self, osim, None)
        threading.Thread.__init__(self)
        #multiprocessing.Process.__init__(self)

        self.up = Vector3([0.0, 0.0, 1.0])
        self.object_type = None
        self.sock = socket.socket()
        self.done = False
        self.tout_val = 0
        self.phys_id = None

        #self.phys_id = osim.get_phys_id(self.sock, self.osim_id)

    # ++++++++++++++++++++++++++++++++
    # All of the handlers
    # ++++++++++++++++++++++++++++++++
    def messageHandler(self):
        try:
            ret = message.Message.get_message(self.sock)
        except socket.timeout as e:
            ret = None

        return ret

    def handle_client_message(self, client, msg):
        pass

    def handle_collision(self, collision):
        if collision.collision_type == "PHYS":
            #hit by a physical object, take damage
            print "%d suffered a Physical collision! %fJ!" % (self.osim_id, collision.energy)
            self.handle_phys(collision)
        elif collision.collision_type == "WEAP":
            #hit by a weapon, take damage
            print "%d suffered a weapon collision! %fJ!" % (self.osim_id, collision.energy)
            self.handle_weap(collision)
        elif collision.collision_type == "COMM":
            #hit by a comm beam, perform apropriate action
            self.handle_comm(collision)
        elif collision.collision_type == "SCAN":
            #hit by a scan beam
            self.handle_scan(collision)
        elif collision.collision_type == "SCRE":
            #self.handle_scanresult(collision)
            pass
        else:
            print "Bad collision: " + str(collision)

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
        self.handle_scanresult(mess)

    def handle_visdata(self, mess):
        print str(mess)

    def make_response(self, energy):
        return [ self.object_type, self.name ]

    def handle_query(self, mess):
        # This is where we respond to SCANQUERY messages.
        # return message.ScanResponseMsg.send(self.sock, self.phys_id, self.osim_id, [ mess.scan_id, self.make_response(), mess.energy ])
        srm = message.ScanResponseMsg()
        srm.scan_id = mess.scan_id
        srm.data = " ".join(self.make_response(mess.scan_energy))
        return message.ScanResponseMsg.send(self.sock, self.phys_id, self.osim_id, srm.build())

    # ++++++++++++++++++++++++++++++++

    def make_explosion(self, position, energy):
        message.BeamMsg.send(self.sock, self.phys_id, self.osim_id, [
                position[0], position[1], position[2],
                299792458.0, 0.0, 0.0,
                0.0, 0.0, 0.0,
                2*pi,
                2*pi,
                energy,
                "WEAP" ])

    def init_beam(self, BeamClass, energy, speed, direction, up, h_focus, v_focus):
        direction = direction.unit()

        velocity = direction.clone()
        velocity.scale(speed)

        origin = direction.clone()
        origin.scale(self.radius + 0.0001)

        beam = BeamClass(self.osim, self.phys_id, self.osim_id, energy, velocity, origin, up, h_focus, v_focus)
        return beam

    #create and launch a beam object. Assumes beam object is already populated with proper values
    def fire_beam(self, beam):
        beam.send_it(self.sock)

    def take_damage(self, amount):
        pass

    def enable_visdata(self):
        return message.VisualDataEnableMsg.send(self.sock, self.phys_id, self.osim_id, {'\x03': True})

    def disable_visdata(self):
        return message.VisualDataEnableMsg.send(self.sock, self.phys_id, self.osim_id, {'\x03': False})

    def set_thrust(self, x, y=None, z=None):
        if (y==None):
            return self.set_thrust(x[0], x[1], x[2])
        self.thrust = Vector3(x,y,z)
        pp = message.PhysicalPropertiesMsg()
        pm.thrust = [ x, y, z ]
        return message.PhysicalPropertiesMsg.send(self.sock, self.phys_id, self.osim_id, pm.build())

    def set_orientation(self, fX, fY = None, uX = None, uY = None):
        if y == None:
            return self.set_orientation(osim_id, fX[0], fX[1], fX[2], fX[3])

        [ self.forward, self.up, self.right ] = [fX, fY, uX, uY]
        pp = message.PhysicalPropertiesMsg()
        pm.orientation = [ fX, fY, uX, uY ]
        return message.PhysicalPropertiesMsg.send(self.sock, self.phys_id, self.osim_id, pm.build())

    def die(self):
        message.GoodbyeMsg.send(self.sock, self.phys_id, self.osim_id, {})
        self.done = True
        self.sock.shutdown(socket.SHUT_RDWR)
        self.sock.close()

    def run(self):
        #TODO: properly parse and branch wrt message recieved
        while not self.done:
            mess = self.messageHandler()

            if isinstance(mess, message.CollisionMsg):
                self.handle_collision(mess)
            elif isinstance(mess, message.VisualDataMsg):
                self.handle_visdata(mess)
            elif isinstance(mess, message.ScanResultMsg):
                self.handle_scanresult(mess)
            elif isinstance(mess, message.ScanQueryMsg):
                self.handle_query(mess)

            #else:
                #print str(mess)

# ==============================================================================
# Beams
# ==============================================================================

class Beam(SpaceObject):
    speed_of_light = 299792458.0

    def __init__(self, osim, phys_id, osim_id, beam_type, energy, velocity, origin, up, h_focus, v_focus):
        SpaceObject.__init__(self, osim, osim_id)
        self.phys_id = phys_id
        self.beam_type = beam_type
        self.energy = energy
        self.velocity = velocity
        self.origin = origin
        self.up = up
        self.h_focus = h_focus
        self.v_focus = v_focus

    def build_common(self):
        bm = message.BeamMsg()
        bm.origin = [ self.origin[i] for i in range(3) ]
        bm.velocity = [ self.velocity[i] for i in range(3) ]
        bm.up = [ self.up[i] for i in range(3) ]
        bm.spread_h = self.h_focus
        bm.spread_v = self.v_focus
        bm.energy = self.energy
        bm.beam_type = self.beam_type
        return bm

    def send_it(self, sock):
        bm = self.build_common()
        message.BeamMsg.send(sock, self.phys_id, self.osim_id, bm.build())

class CommBeam(Beam):
    def __init__(self, osim, phys_id, osim_id, energy, velocity, origin, up, h_focus, v_focus, message):
        Beam.__init__(self, osim, phys_id, osim_id, "COMM", energy, velocity, origin, up, h_focus, v_focus)
        self.message = message

    def send_it(self, sock):
        ar = self.build_common()
        ar.comm_msg = self.message
        message.BeamMsg.send(sock, self.phys_id, self.osim_id, ar.build())


class WeaponBeam(Beam):
    def __init__(self, osim, phys_id, osim_id, energy, velocity, origin, up, h_focus, v_focus, subtype="laser"):
        Beam.__init__(self, osim, phys_id, osim_id, "WEAP", energy, velocity, origin, up, h_focus, v_focus)
        self.subtype = subtype

    def send_it(self, sock):
        ar = self.build_common()
        #ar.append(self.subtype)
        message.BeamMsg.send(sock, self.phys_id, self.osim_id, ar.build)

class ScanBeam(Beam):
    def __init__(self, osim, phys_id, osim_id, energy, velocity, origin, up, h_focus, v_focus):
        Beam.__init__(self, osim, phys_id, osim_id, "SCAN", energy, velocity, origin, up, h_focus, v_focus)

# ==============================================================================

# ==============================================================================
# Missiles
# ==============================================================================


#a dumbfire missile, for example
class Missile(SmartObject):
    def __init__(self, osim, object_type="Missile", payload = 1.3e21):
        SmartObject.__init__(self, osim)
        self.object_type = object_type
        self.payload = payload
        self.radius = 1.0
        self.mass = 100.0
        #self.sock.settimeout(self.tout_val)

    def handle_phys(self, mess):
        self.detonate()

    def detonate(self):
        self.make_explosion(self.position, self.payload)
        self.die()

    def run(self):
        while not self.done:

            val = self.messageHandler()

            if val == None:
                return

            if isinstance(val, message.CollisionMsg):
                self.handle_collision(val)


class HomingMissile1(Missile):
    def __init__(self, osim, direction, payload = 1.3e21):
        Missile.__init__(self, osim, "Homing missile", payload)
        self.direction = direction
        self.position = direction.unit()
        self.radius = 2
        self.mass = 100
        self.tout_val = 1
        #self.sock.settimeout(self.tout_val)
        self.fuse = 100.0    #distance in meters to explode from target

    #so this is not great. Only works if there is a single target 'in front of' the missile
    #def handle_scanresult(self, mess):
        #enemy_pos = Vector3(mess.position)
        #enemy_vel = Vector3(mess.velocity)
        ##distance = self.position.distance(enemy_pos)
        #distance = enemy_pos.length()
        #if distance < self.fuse+mess.radius:
            #self.detonate()
        #else:
            #new_dir = enemy_pos.unit()
            #new_dir.scale(self.thrust.length())
            #print ("Homing missile %d setting new thrust vector " % self.osim_id) + str(new_dir) + (". Distance to target: %2f" % (distance-mess.radius))
            #self.set_thrust(new_dir)


    def handle_scanresult(self, mess):
        if ("ship" in mess.object_type or "Ship" in mess.object_type or "SHIP" in mess.object_type):
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
                print ("Homing missile %d setting new thrust vector " % self.osim_id) + str(new_dir) + (". Distance to target: %2f" % (distance-mess.radius))
                self.set_thrust(new_dir)
                epos = enemy_pos.unit()
                [ self.forward, self.up, self.right ] = Vector3.easy_look_at(epos)

    def do_scan(self):
        scan = self.init_beam(ScanBeam, 10000.0, Beam.speed_of_light, self.forward, self.up, h_focus=pi/4, v_focus=pi/4)
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

# ==============================================================================

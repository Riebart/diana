#!/usr/bin/env python

import bson
import struct # Needed to unpack the first four bytes of the string.
import sys
import socket
from vector import Vector3, zero3d
from cStringIO import StringIO

class Message:
    def __init__(self, client):
        pass

    @staticmethod
    def get_message_size(client):
        try:
            bytes = Message.big_read(client, 4)
            msg_length = struct.unpack('<L', bytes)[0]
            return (bytes, msg_length)
        except ValueError:
            print "Bad message length \"%s\" (not parsable) from %d" % (raw, client.fileno())
            return (None, None)
        except socket.timeout as e:
            raise e
        except socket.error, (errno, errmsg):
            if client.fileno() == -1 or errno == 10054 or errno == 10053:
                return (None, None)

            print "There was an error getting message size header from client %d" % client.fileno()
            print "Error:", sys.exc_info()
            return (None, None)

    @staticmethod
    def big_read(client, num_bytes):
        num_got = 0
        file_str = StringIO()

        while num_got < num_bytes:
            if client.fileno() == -1:
                break

            cur_read = min(4096, num_bytes - num_got)

            try:
                cur_msg = client.recv(cur_read)
            except socket.timeout as e:
                raise e
            except:
                if client.fileno() == -1:
                    return None

                print "There was an error getting message from client %d" % client.fileno()
                print "Error:", sys.exc_info()
                return None
            num_got += len(cur_msg)
            file_str.write(cur_msg)

        return file_str.getvalue()

    @staticmethod
    # Chunked indicates whether or not we want to be allowed to have other threds
    # grab and write to the socket in between our send() calls. Normall, this
    # is not something we tolerate.
    def big_send(client, msg, chunked = 0):
        if chunked:
            num_sent = 0
            num_bytes = len(msg)

            while num_sent < num_bytes:
                if client.fileno() == -1:
                    return num_sent

                try:
                    cur_sent = client.send(msg[num_sent:])
                except socket.error, (errno, errstr):
                    if client.fileno() == -1 or errno == 10504:
                        return num_sent
                    else:
                        print "There was an error sendall-ing message to client %d" % client.fileno()
                        print "Error:", sys.exc_info()
                        return num_sent

                num_sent += cur_sent

            return num_sent
        else:
            try:
                client.sendall(msg)
            except socket.error, (errno, errmsg):
                if errno != 10054 and errno != 10053:
                    print "There was an error sendall-ing message to client %d" % client.fileno()
                    print "Error:", sys.exc_info()

                return 0


    @staticmethod
    # Reads a single message from the socket and returns that object.
    def get_message(client):
        # First grab the message size
        bytes, msg_size = Message.get_message_size(client)
        if msg_size == None:
            return None

        msg_bytes = bytes = Message.big_read(client, msg_size - 4)
        msg = bson.loads(msg_bytes)

        # Snag out the IDs
        try:
            srv_id = msg['\x01']
            cli_id = msg['\x02']
        except:
            print "Couldn't parse the IDs from the message header from %d" % client.fileno()
            sys.stdout.flush()
            return None

        if not msg == None:
            msgtype = msg['MsgType']

            if msgtype in MessageTypes:
                m = MessageTypes[msgtype](msg, srv_id, cli_id)
                return m
            else:
                print "Unknown message type: \"%s\"" % msgtype
                sys.stdout.flush()
                return None
        else:
            return None

    @staticmethod
    def sendall(client, srv_id, cli_id, msg):
        # Detect a hangup
        if client.fileno() == -1:
            return 0

        msg['\x01'] = srv_id if srv_id != None else -1
        msg['\x02'] = cli_id if cli_id != None else -1
        num_sent = Message.big_send(client, bson.dumps(msg))
        if num_sent == 0:
            return 0

        return 1

    @staticmethod
    def prep_double(d):
        if d != None and d != "":
            s = "%.55f" % d
        else:
            s = ""
        return s

    @staticmethod
    def prep_double3(d1, d2, d3):
        s = ""
        s.append(Message.prep_double(d1))
        s.append(Message.prep_double(d2))
        s.append(Message.prep_double(d3))
        return s

    @staticmethod
    def read_double(sa):
        s = sa[0]
        del sa[0]

        if s != "":
            try:
                f = float(s)
                return f
            except:
                print "Error parsing double"
                print "Error:", sys.exc_info()
                return None
        else:
            return None

    @staticmethod
    def read_double3(sa):
        tmp = [ Message.read_double(sa),
                Message.read_double(sa),
                Message.read_double(sa) ]
        if tmp[0] != None and tmp[1] != None and tmp[2] != None:
            return tmp
        else:
            return None

    @staticmethod
    def read_double4(sa):
        tmp = [ Message.read_double(sa),
                Message.read_double(sa),
                Message.read_double(sa),
                Message.read_double(sa) ]
        if tmp[0] != None and tmp[1] != None and tmp[2] != None and tmp[3] != None:
            return tmp
        else:
            return None

    @staticmethod
    def read_int(sa):
        s = sa[0]
        del sa[0]

        if s != "":
            try:
                f = int(s)
                return f
            except:
                print "Error parsing int"
                print "Error:", sys.exc_info()
                return None
        else:
            return None

    @staticmethod
    def prep_mesh(m):
        return m

    @staticmethod
    def read_mesh(sa):
        m = sa[0]
        del sa[0]

        if m != "":
            return m
        else:
            return None

    @staticmethod
    def prep_texture(t):
        return t

    @staticmethod
    def read_texture(sa):
        t = sa[0]
        del sa[0]

        if t != "":
            return t
        else:
            return None

class UnknownMsg(Message):
    def __init__(self, msgtype):
        self.msgtype = msgtype

class HelloMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id

    #def sendto(self, client):
        #HelloMsg.send(client, self.srv_id, self.cli_id)

    @staticmethod
    def send(client, srv_id, cli_id):
        ret = Message.sendall(client, srv_id, cli_id, {})
        return ret

class PhysicalPropertiesMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.object_type = s[0]
        del s[0]
        self.mass = Message.read_double(s)
        self.position = Message.read_double3(s)
        self.velocity = Message.read_double3(s)
        self.orientation = Message.read_double4(s)
        self.thrust = Message.read_double3(s)
        self.radius = Message.read_double(s)

    def sendto(self, client):
        if object_type != None:
            msg['\x03'] = object_type
        if mass != None:
            msg['\x04'] = mass
        if position != None:
            msg['\x05'] = position.x
            msg['\x06'] = position.y
            msg['\x07'] = position.z
        if velocity != None:
            msg['\x08'] = velocity.x
            msg['\x09'] = velocity.y
            msg['\x0A'] = velocity.z
        if orientation != None:
            msg['\x0B'] = orientation.w
            msg['\x0C'] = orientation.x
            msg['\x0D'] = orientation.y
            msg['\x0E'] = orientation.z
        if thrust != None:
            msg['\x0F'] = thrust.x
            msg['\x10'] = thrust.y
            msg['\x11'] = thrust.z
        if radius != None:
            msg['\x12'] = radius

        PhysicalPropertiesMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "PHYSPROPS\n%s\n" % args[0]

        for i in range(1,16):
            msg += Message.prep_double(args[i]) + "\n"

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

    @staticmethod
    def make_from_object(obj, p = zero3d, v = zero3d, o = zero3d):
        # ### TODO ### Make this relative to orientation?
        orientation = [ obj.forward.x, obj.forward.y, obj.up.x, obj.up.z ]

        return [ obj.object_type, obj.mass,
                    obj.position.x - p.x, obj.position.y - p.y, obj.position.z - p.z,
                    obj.velocity.x - v.x, obj.velocity.y - v.y, obj.velocity.z - v.z,
                    orientation[0], orientation[1], orientation[2], orientation[3],
                    obj.thrust.x, obj.thrust.y, obj.thrust.z,
                    obj.radius ]


#class RotationMsg(Message):
    #def __init__(self, s, srv_id, cli_id):
        #self.srv_id = srv_id
        #self.cli_id = cli_id
        #self.is_velocity = Message.read_int(s)
        #self.angles = Message.read_double3(s)

    #def sendto(self, client):
        #RotationMsg.send(client, self.srv_id, self.cli_id, [ self.is_velocity ] + self.angles)

    #@staticmethod
    #def send(client, srv_id, cli_id, args):
        #msg = "ROTATE\n%d\n" % args[0]
        #msg += Message.prep_double3(args[1]) + "\n"

        #Message.sendall(client, srv_id, cli_id, msg)

class VisualPropertiesMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.mesh = Message.read_mesh(s)
        self.texture = Message.read_texture(s)

    #def sendto(self, client):
        #VisualPropertiesMsg.send(client, self.srv_id, self.cli_id,
            #[ self.mesh, self.texture ])

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "VISPROPS\n"
        msg += Message.prep_mesh(args[0])
        msg += Message.prep_texture(args[1])

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class VisualDataEnableMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        sys.stdout.flush()
        self.enabled = Message.read_int(s)

    #def sendto(self, client):
        #VisualDataEnableMsg.send(client, self.srv_id, self.cli_id, self.enabled)

    @staticmethod
    def send(client, srv_id, cli_id, arg):
        msg = "VISDATAENABLE\n%d\n" % arg

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class VisualMetaDataEnableMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.enabled = Message.read_int(s)

    #def sendto(self, client):
        #VisualMetaDataEnableMsg.send(client, self.srv_id, self.cli_id, self.enabled)

    @staticmethod
    def send(client, srv_id, cli_id, arg):
        msg = "VISMETADATAENABLE\n%d\n" % arg

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class VisualMetaDataMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.art_id = Message.read_int(s)
        self.mesh = Message.read_mesh(s)
        self.texture = Message.read_texture(s)

    #def sendto(self, client):
        #VisualMetaDataMsg.send(client, self.srv_id, self.cli_id, [ self.art_id, self.mesh, self.texture ])

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "VISMETADATA\n"

        art_id = args[0]
        mesh = args[1]
        texture = args[2]
        msg += "%d\n" % art_id
        msg += Message.prep_mesh(mesh) + "\n"
        msg += Message.prep_texture(texture) + "\n"

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class VisualDataMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.phys_id = Message.read_int(s)
        self.radius = Message.read_double(s)
        self.position = Message.read_double3(s)
        self.orientation = Message.read_double3(s)

    def sendto(self, client):
        if self.phys_id == -1:
            return VisualDataMsg.send(client, self.srv_id, self.cli_id, "VISDATA\n-1\n\n\n\n\n\n\n\n")
        else:
            return VisualDataMsg.send(client, self.srv_id, self.cli_id,
                "VISDATA\n" +
                str(self.phys_id) + "\n" +
                str(self.radius) + "\n" +
                str(self.position[0]) + "\n" + str(self.position[1]) + "\n" + str(self.position[2]) + "\n" +
                str(self.orientation[0]) + "\n" + str(self.orientation[1]) + "\n" + str(self.orientation[2]) + "\n")

    @staticmethod
    # This is special in that the args are exactly the string we want to send.
    def send(client, srv_id, cli_id, args):
        ret = Message.sendall(client, srv_id, cli_id, args)
        return ret

class BeamMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.origin = Message.read_double3(s)
        self.velocity = Message.read_double3(s)
        self.up = Message.read_double3(s)
        self.spread_h = Message.read_double(s)
        self.spread_v = Message.read_double(s)
        self.energy = Message.read_double(s)

        if s[0] == "SCAN":
            self.beam_type = s[0]
        elif s[0] == "WEAP":
            self.beam_type = s[0]
        elif s[0] == "COMM":
            self.beam_type = s[0]
            self.msg = ""
            del s[0]
            for line in s:
                msg += line + "\n"
        else:
            self.beam_type = None

    #def sendto(self, client):
        #args = self.origin + self.velocity + self.up + [ self.spread_h, self.spread_v, self.energy, self.beam_type ]

        #if s[0] == "SCAN":
            #pass
        #elif s[0] == "WEAP":
            #pass
        #elif s[0] == "COMM":
            #args += self.msg

        #BeamMsg.send(client, self.srv_id, self.cli_id, args)

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "BEAM\n"
        for i in range(0,12):
            msg += Message.prep_double(args[i]) + "\n"

        msg += args[12] + "\n"

        if args[12] == "SCAN":
            pass
        elif args[12] == "WEAP":
            pass
        elif args[12] == "COMM":
            for i in range(13, len(args)):
                msg += args[i] + "\n"
        else:
            print "Unknown beam subtype \"%s\"." % args[12]
            return 0

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class CollisionMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.position = Message.read_double3(s)
        self.direction = Message.read_double3(s)
        self.energy = Message.read_double(s)

        if s[0] == "PHYS":
            self.collision_type = s[0]
        elif s[0] == "SCAN":
            self.collision_type = s[0]
        elif s[0] == "WEAP":
            self.collision_type = s[0]
        elif s[0] == "COMM":
            self.collision_type = s[0]
            self.msg = ""
            del s[0]
            for line in s:
                msg += line + "\n"
        else:
            self.collision_type = None

    #def sendto(self, client):
        #args = self.position + self.direction + [ self.energy ]

        #if s[0] == "SCAN":
            #pass
        #elif s[0] == "WEAP":
            #pass
        #elif s[0] == "COMM":
            #args += self.msg

        #CollisionMsg.send(client, self.srv_id, self.cli_id, args)

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "COLLISION\n"

        for i in range(0,7):
            msg += Message.prep_double(args[i]) + "\n"

        msg += args[7] + "\n"

        if args[7] == "PHYS":
            pass
        elif args[7] == "SCAN":
            pass
        elif args[7] == "WEAP":
            pass
        elif args[7] == "COMM":
            for i in range(8, len(args)):
                msg += args[i] + "\n"
        else:
            print "Unknown beam subtype \"%s\"." % args[7]
            return 0

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class SpawnMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.object_type = s[0]
        del s[0]
        self.mass = Message.read_double(s)
        self.position = Message.read_double3(s)
        self.velocity = Message.read_double3(s)
        self.orientation = Message.read_double4(s)
        self.thrust = Message.read_double3(s)
        self.radius = Message.read_double(s)

    #def sendto(self, client):
        #SpawnMsg.send(client, self.srv_id, self.cli_id,
            #[ self.object_type, self.mass ] + self.position + self.velocity +
                #self.orientation + self.thrust + [ self.radius ])

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "SPAWN\n%s\n" % args[0]

        for i in range(1,16):
            msg += Message.prep_double(args[i]) + "\n"

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class ScanResultMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.object_type = s[0]
        del s[0]
        self.mass = Message.read_double(s)
        self.position = Message.read_double3(s)
        self.velocity = Message.read_double3(s)
        self.orientation = Message.read_double3(s)
        self.thrust = Message.read_double3(s)
        self.radius = Message.read_double(s)
        self.extra_parms = s

    #def sendto(self, client):
        #ScanResultMsg.send(client, self.srv_id, self.cli_id,
            #[ self.object_type, self.mass ] + self.position + self.velocity +
                #self.orientation + self.thrust + [ self.radius ] + self.extra_parms)

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "SCANRESULT\n%s\n" % args[0]

        for i in range(1,15):
            msg += Message.prep_double(args[i]) + "\n"

        if (len(args) > 15):
            msg += str(args[15])

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class ScanQueryMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.scan_id = Message.read_int(s)
        self.scan_power = Message.read_double(s)
        self.scan_dir = Message.read_double3(s)

    #def sendto(self, client):
        #ScanQueryMsg.send(client, self.srv_id, self.cli_id, [self.scan_id, self.scan_power] + self.scan_dir)

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "SCANQUERY\n%d\n" % args[0]
        for i in range (1,5):
            msg += Message.prep_double(args[i]) + "\n"

        ret = Message.sendall(client, srv_id, cli_id, msg)

class ScanResponseMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.scan_id = Message.read_int(s)
        self.parms = s

    #def sendto(self, client):
        #ScanResponseMsg.send(client, self.srv_id, self.cli_id, [ self.scan_id ] + self.parms)

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "SCANRESP\n%d\n" % args[0]

        for i in args[1:]:
            msg += str(i)

        ret = Message.sendall(client, srv_id, cli_id, msg)

class GoodbyeMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id

    #def sendto(self, client):
        #GoodbyeMsg.send(client, self.srv_id, self.cli_id)

    @staticmethod
    def send(client, srv_id, cli_id):
        msg = "GOODBYE\n"

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class DirectoryMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.item_type = s[0]
        del s[0]

        self.items = []
        for i in range(0, len(s) - (len(s) % 2), 2):
            self.items.append([int(s[i]), s[i+1]])

    #def sendto(self, client):
        #DirectoryMsg.send(client, self.srv_id, self.cli_id, [self.item_type] + self.items)

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "DIRECTORY\n%s\n" % args[0]

        del args[0]
        for a in args:
            msg += "%d\n%s\n" % (a[0], a[1])

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class NameMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.name = s[0]

    #def sendto(self, client):
        #NameMsg.send(client, self.srv_id, self.cli_id, self.name)

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "NAME\n%s\n" % args

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class ReadyMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.ready = int(s[0])

    #def sendto(self, client):
        #NameMsg.send(client, self.srv_id, self.cli_id, self.name)

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "READY\n%s\n" % args

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret


##Here begins the client <-> ship messages
class ThrustMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.thrust = Message.read_double3(s)

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "CMDTHRUST\n%s\n" % prep_double3(args)

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class VelocityMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.velocity = Message.read_double3(s)

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "CMDVELOCITY\n%s\n" % prep_double3(args)

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class JumpMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.new_position = Message.read_double3(s)

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "CMDJUMP\n%s\n" % prep_double3(args)

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class InfoUpdateMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.type = s[0]
        self.data = s[1]

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "INFO\n%s\n" % args[0]

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class RequestUpdateMsg(Message):
    def __init__(self, s, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.continuous = Message.read_int(s)
        self.type = s

    @staticmethod
    def send(client, srv_id, cli_id, args):
        msg = "REQUEST\n%d\n%s" % (args[0], args[1])

        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

MessageTypes = { 1: HelloMsg,
                 2: PhysicalPropertiesMsg,
                 3: VisualPropertiesMsg,
                 4: VisualDataEnableMsg,
                 5: VisualMetaDataEnableMsg,
                 6: VisualMetaDataMsg,
                 7: VisualDataMsg,
                 8: BeamMsg,
                 9: CollisionMsg,
                10: SpawnMsg,
                11: ScanResultMsg,
                12: ScanQueryMsg,
                13: ScanResponseMsg,
                14: GoodbyeMsg,
                15: DirectoryMsg,
                16: NameMsg,
                17: ReadyMsg,
                18: ThrustMsg,
                19: VelocityMsg,
                20: JumpMsg,
                21: InfoUpdateMsg,
                22: RequestUpdateMsg
               }

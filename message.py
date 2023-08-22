#!/usr/bin/env python

from __future__ import print_function
import bson
import struct # Needed to unpack the first four bytes of the BSON message for read-length.
import sys
import socket
from vector import Vector3, zero3d
from cStringIO import StringIO
from collections import OrderedDict

class Spectrum:
    def __init__(self, wavelengths, powers):
        if wavelengths != None and powers != None:
            print(wavelengths)
            print(powers)
            self.spectrum = dict(zip(wavelengths, powers))
        else:
            self.spectrum = None

    def get_parts(self):
        if self.spectrum == None:
            return []

        keys = sorted(self.spectrum.keys())
        vals = [ self.spectrum[k] for k in keys ]

        return [ len(keys), keys, vals ]

class Message:
    def __init__(self, client):
        pass

    @staticmethod
    def get_message_size(client):
        try:
            bytes = Message.big_read(client, 4)
            msg_length = struct.unpack('<l', bytes)[0]
            return (bytes, msg_length)
        except ValueError:
            print("Bad message length \"%s\" (not parsable) from %d" % (raw, client.fileno()))
            return (None, None)
        except socket.timeout as e:
            raise e
        except socket.error as xxx_todo_changeme2:
            (errno, errmsg) = xxx_todo_changeme2.args
            if client.fileno() == -1 or errno == 10054 or errno == 10053:
                return (None, None)

            print("There was an error getting message size header from client %d" % client.fileno())
            print("Error:", sys.exc_info())
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

                print("There was an error getting message from client %d" % client.fileno())
                print("Error:", sys.exc_info())
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
                except socket.error as xxx_todo_changeme:
                    (errno, errstr) = xxx_todo_changeme.args
                    if client.fileno() == -1 or errno == 10504:
                        return num_sent
                    else:
                        print("There was an error sendall-ing message to client %d" % client.fileno())
                        print("Error:", sys.exc_info())
                        return num_sent

                num_sent += cur_sent

            return num_sent
        else:
            try:
                client.sendall(msg)
            except socket.error as xxx_todo_changeme1:
                (errno, errmsg) = xxx_todo_changeme1.args
                if errno != 10054 and errno != 10053:
                    print("There was an error sendall-ing message to client %d" % client.fileno())
                    print("Error:", sys.exc_info())

                return 0


    @staticmethod
    # Reads a single message from the socket and returns that object.
    def get_message(client):
        # First grab the message size
        bytes, msg_size = Message.get_message_size(client)
        if msg_size == None:
            return None

        msg_bytes = bytes + Message.big_read(client, msg_size - 4)
        msg = bson.loads(msg_bytes)
        #print "RECV", msg

        # Snag out the IDs
        srv_id = msg['\x01'] if '\x01' in msg else None
        cli_id = msg['\x02'] if '\x02' in msg else None

        if not msg == None:
            msgtype = msg['']

            if msgtype in MessageTypeClasses:
                m = MessageTypeClasses[msgtype](msg, msgtype, srv_id, cli_id)
                m.socket = client
                return m
            else:
                print("Unknown message type: \"%s\"" % msgtype)
                sys.stdout.flush()
                return None
        else:
            return None

    @staticmethod
    def sendall(client, srv_id, cli_id, msg):
        # Detect a hangup
        if client.fileno() == -1:
            return 0

        if srv_id != None:
            msg['\x01'] = srv_id
        if cli_id != None:
            msg['\x02'] = cli_id

        omsg = OrderedDict(sorted(msg.items(), key=lambda t: t[0]))
        #print "SEND", omsg
        num_sent = Message.big_send(client, bson.dumps(omsg))
        if num_sent == 0:
            return 0

        return 1

    @staticmethod
    def ReadMsgEl(key, msg):
        if isinstance(key, list) or isinstance(key, tuple):
            return [ (msg[k] if k in msg else None) for k in key]
        else:
            return msg[key] if key in msg else None

    @staticmethod
    def SendMsgEl(key, val, msg):
        if isinstance(key, list) or isinstance(key, tuple):
            for k, v in zip(key, val):
                if v != None:
                    msg[k] = v
        elif val != None:
            msg[key] = val

class UnknownMsg(Message):
    def __init__(self, msgtype):
        self.msgtype = msgtype

class HelloMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id

    def build(self):
        return {}

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[HelloMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class PhysicalPropertiesMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.object_type = Message.ReadMsgEl('\x03', msg)
        self.mass = Message.ReadMsgEl('\x04', msg)
        self.position = Message.ReadMsgEl(('\x05','\x06','\x07'), msg)
        self.velocity = Message.ReadMsgEl(('\x08','\x09','\x0A'), msg)
        self.orientation = Message.ReadMsgEl(('\x0B','\x0C','\x0D','\x0E'), msg)
        self.thrust = Message.ReadMsgEl(('\x0F','\x10','\x11'), msg)
        self.radius = Message.ReadMsgEl('\x12', msg)

        # We can ignore the spectrum component count, and just consider the arrays that follow, because we're Python
        self.spectrum = Spectrum(Message.ReadMsgEl('\x14', msg), Message.ReadMsgEl('\x15', msg))

    def build(self):
        msg = {}
        vals = [ self.object_type, self.mass ] + \
            list(self.position) + list(self.velocity) + \
            list(self.orientation) + list(self.thrust) + [ self.radius ] + self.spectrum.get_parts()
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[PhysicalPropertiesMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

    @staticmethod
    def make_from_object(obj, p = zero3d, v = zero3d):
        msg = {}
        vals = [ obj.object_type, obj.mass ] + \
            [a-b for a,b in zip(obj.position,p)] + \
            [a-b for a,b in zip(obj.velocity,v)] + \
            obj.orientation + obj.thrust + [ obj.radius ] + self.spectrum.get_parts()
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)

        return msg

class VisualPropertiesMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        #self.mesh = Message.read_mesh(s)
        #self.texture = Message.read_texture(s)

    def build(self):
        msg = {}
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[VisualPropertiesMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)

class VisualDataEnableMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.enabled = Message.ReadMsgEl('\x03', msg)

    def build(self):
        msg = {}
        Message.SendMsgEl('\x03', self.enabled, msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[VisualDataEnableMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class VisualMetaDataEnableMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        #self.art_id = Message.read_int(s)
        #self.mesh = Message.read_mesh(s)
        #self.texture = Message.read_texture(s)

    def build(self):
        msg = {}
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[VisualMetaDataEnableMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)

class VisualMetaDataMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        #self.art_id = Message.read_int(s)
        #self.mesh = Message.read_mesh(s)
        #self.texture = Message.read_texture(s)

    def build(self):
        msg = {}
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[VisualMetaDataMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)

class VisualDataMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.phys_id = Message.ReadMsgEl('\x03', msg)
        self.radius = Message.ReadMsgEl('\x04', msg)
        self.position = Message.ReadMsgEl(('\x05','\x06','\x07'), msg)
        self.orientation = Message.ReadMsgEl(('\x0B','\x0C','\x0D','\x0E'), msg)

    def build(self):
        msg = {}
        vals = [ self.phys_id, self.radius ] + list(self.position) + list(self.orientation)
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[VisualDataMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class BeamMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.origin = Message.ReadMsgEl(('\x03','\x04','\x05'), msg)
        self.velocity = Message.ReadMsgEl(('\x06','\x07','\x08'), msg)
        self.up = Message.ReadMsgEl(('\x09','\x0A','\x0B'), msg)
        self.spread_h = Message.ReadMsgEl('\x0C', msg)
        self.spread_v = Message.ReadMsgEl('\x0D', msg)
        self.energy = Message.ReadMsgEl('\x0E', msg)
        self.beam_type = Message.ReadMsgEl('\x0F', msg)
        self.comm_msg = Message.ReadMsgEl('\x10', msg)
        # Skip 11
        self.spectrum = Spectrum(Message.ReadMsgEl('\x12', msg), Message.ReadMsgEl('\x13', msg))


    def build(self):
        msg = {}
        vals = self.origin + self.velocity + self.up + \
            [ self.spread_h, self.spread_v, self.energy, self.beam_type, self.comm_msg ] + self.spectrum.get_parts()
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[BeamMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class CollisionMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.position = Message.ReadMsgEl(('\x03','\x04','\x05'), msg)
        self.direction = Message.ReadMsgEl(('\x06','\x07','\x08'), msg)
        self.energy = Message.ReadMsgEl('\x09', msg)
        self.collision_type = Message.ReadMsgEl('\x0A', msg)
        self.comm_msg = Message.ReadMsgEl('\x0B', msg)
        # Skip 0C
        self.spectrum = Spectrum(Message.ReadMsgEl('\x0D', msg), Message.ReadMsgEl('\x0E', msg))

    def build(self):
        msg = {}
        vals = self.position + self.direction + [ self.energy + self.beam_type, self.comm_msg ] + self.spectrum.get_parts()
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[CollisionMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class SpawnMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.is_smart = Message.ReadMsgEl('\x03', msg)
        self.object_type = Message.ReadMsgEl('\x04', msg)
        self.mass = Message.ReadMsgEl('\x05', msg)
        self.position = Message.ReadMsgEl(('\x06','\x07','\x08'), msg)
        self.velocity = Message.ReadMsgEl(('\x09','\x0A','\x0B'), msg)
        self.orientation = Message.ReadMsgEl(('\x0C','\x0D','\x0E','\x0F'), msg)
        self.thrust = Message.ReadMsgEl(('\x10','\x11','\x12'), msg)
        self.radius = Message.ReadMsgEl('\x13', msg)
        # Skip 14
        self.spectrum = Spectrum(Message.ReadMsgEl('\x15', msg), Message.ReadMsgEl('\x16', msg))

    def build(self):
        msg = {}
        vals = [ self.is_smart, str(self.object_type), self.mass ] + \
            list(self.position) + list(self.velocity) + list(self.orientation) + list(self.thrust) + \
            [ self.radius ] + self.spectrum.get_parts()
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[SpawnMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class ScanResultMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.object_type = Message.ReadMsgEl('\x03', msg)
        self.mass = Message.ReadMsgEl('\x04', msg)
        self.position = Message.ReadMsgEl(('\x05','\x06','\x07'), msg)
        self.velocity = Message.ReadMsgEl(('\x08','\x09','\x0A'), msg)
        self.orientation = Message.ReadMsgEl(('\x0B','\x0C','\x0D','\x0E'), msg)
        self.thrust = Message.ReadMsgEl(('\x0F','\x10','\x11'), msg)
        self.radius = Message.ReadMsgEl('\x12', msg)
        self.data = Message.ReadMsgEl('\x13', msg)
        # Skip 14
        self.spectrum = Spectrum(Message.ReadMsgEl('\x15', msg), Message.ReadMsgEl('\x16', msg))
        # Skip 17
        self.spectrum = Spectrum(Message.ReadMsgEl('\x18', msg), Message.ReadMsgEl('\x19', msg))

    def build(self):
        msg = {}
        vals = [ self.mass ] + self.position + self.velocity + self.orientation + self.thrust + [ self.radius, self.data ] + self.spectrum.get_parts()
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[ScanResultMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class ScanQueryMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.scan_id = Message.ReadMsgEl('\x03', msg)
        self.scan_energy = Message.ReadMsgEl('\x04', msg)
        self.scan_dir = Message.ReadMsgEl(('\x05','\x06','\x07'), msg)
        # Skip 08
        self.spectrum = Spectrum(Message.ReadMsgEl('\x09', msg), Message.ReadMsgEl('\x0A', msg))

    def build(self):
        msg = {}
        print(self.scan_dir)
        vals = [ self.scan_id, self.scan_energy ] + self.scan_dir + self.spectrum.get_parts()
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[ScanQueryMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)

class ScanResponseMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.scan_id = Message.ReadMsgEl('\x03', msg)
        self.data = Message.ReadMsgEl('\x04', msg)

    def build(self):
        msg = {}
        vals = [ self.scan_id, self.data ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[ScanResponseMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)

class GoodbyeMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id

    def build(self):
        return {}

    @staticmethod
    def send(client, srv_id, cli_id, msg = {}):
        msg[''] = MessageTypeIDs[GoodbyeMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class DirectoryMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.item_type = Message.ReadMsgEl('\x03', msg)
        item_count = Message.ReadMsgEl('\x04', msg)

        # Item 5 is an array of IDs, and item 6 is an array of names
        ids = Message.ReadMsgEl('\x05', msg)
        names = Message.ReadMsgEl('\x06', msg)
        if ids != None and names != None and len(ids) == len(names):
            self.items = zip(ids, names)
        else:
            self.items = None

    def build(self):
        msg = {}
        Message.SendMsgEl('\x03', self.item_type, msg)
        Message.SendMsgEl('\x04', 0 if self.items == None else len(self.items), msg)
        Message.SendMsgEl('\x05', [] if self.items == None else [ i[0] for i in self.items ], msg)
        Message.SendMsgEl('\x06', [] if self.items == None else [ i[1] for i in self.items ], msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[DirectoryMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class NameMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.name = Message.ReadMsgEl('\x03', msg)

    def build(self):
        msg = {}
        val = [ self.name ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[NameMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class ReadyMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.ready = Message.ReadMsgEl('\x03', msg)

    def build(self):
        msg = {}
        val = [ self.ready ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[ReadyMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret



##Here begins the client <-> ship messages
class ThrustMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.thrust = Message.ReadMsgEl(('\x03','\x04','\x05'), msg)

    def build(self):
        msg = {}
        val = [ self.thrust ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[ThrustMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class VelocityMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.velocity = Message.ReadMsgEl(('\x03','\x04','\x05'), msg)

    def build(self):
        msg = {}
        val = [ self.velocity ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[VelocityMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class JumpMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.new_position = Message.ReadMsgEl(('\x03','\x04','\x05'), msg)

    def build(self):
        msg = {}
        val = [ self.new_position ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[JumpMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class InfoUpdateMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.type = Message.ReadMsgEl('\x03', msg)
        self.data = Message.ReadMsgEl('\x04', msg)

    def build(self):
        msg = {}
        val = [ self.type, self.data ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[InfoUpdateMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class RequestUpdateMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.type = Message.ReadMsgEl('\x03', msg)
        self.continuous = Message.ReadMsgEl('\x04', msg)

    def build(self):
        msg = {}
        val = [ self.type, self.continuous ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        return msg

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[RequestUpdateMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class SystemUpdateMsg(Message):
    def __init__(self, msg={}, msgtype=-1, srv_id=-1, cli_id=-1):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id

    #message is Osim -> client only at this time
    def build(self):
        pass
    
    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg[''] = MessageTypeIDs[SystemUpdateMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

MessageTypeClasses = {1: HelloMsg,
                      2: PhysicalPropertiesMsg,
#                      3: VisualPropertiesMsg,
                      4: VisualDataEnableMsg,
#                      5: VisualMetaDataEnableMsg,
#                      6: VisualMetaDataMsg,
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
                      22: RequestUpdateMsg,
                      23: SystemUpdateMsg
                      }

MessageTypeIDs = { HelloMsg: 1,
                  PhysicalPropertiesMsg: 2,
#                  VisualPropertiesMsg: 3,
                  VisualDataEnableMsg: 4,
#                  VisualMetaDataEnableMsg: 5,
#                  VisualMetaDataMsg: 6,
                  VisualDataMsg: 7,
                  BeamMsg: 8,
                  CollisionMsg: 9,
                  SpawnMsg: 10,
                  ScanResultMsg: 11,
                  ScanQueryMsg: 12,
                  ScanResponseMsg: 13,
                  GoodbyeMsg: 14,
                  DirectoryMsg: 15,
                  NameMsg: 16,
                  ReadyMsg: 17,
                  ThrustMsg: 18,
                  VelocityMsg: 19,
                  JumpMsg: 20,
                  InfoUpdateMsg: 21,
                  RequestUpdateMsg: 22,
                  SystemUpdateMsg: 23
                  }

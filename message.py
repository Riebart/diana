#!/usr/bin/env python

import bson
import struct # Needed to unpack the first four bytes of the BSON message for read-length.
import sys
import socket
from vector import Vector3, zero3d
from cStringIO import StringIO

MessageTypeClasses = {1: HelloMsg,
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

MessageTypeIDs = { HelloMsg: 1,
                  PhysicalPropertiesMsg: 2,
                  VisualPropertiesMsg: 3,
                  VisualDataEnableMsg: 4,
                  VisualMetaDataEnableMsg: 5,
                  VisualMetaDataMsg: 6,
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
                  RequestUpdateMsg: 22
                  }

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
                m = MessageTypeClasses[msgtype](msg, msgtype, srv_id, cli_id)
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

        msg['\x01'] = srv_id if srv_id != None else None
        msg['\x02'] = cli_id if cli_id != None else None
        num_sent = Message.big_send(client, bson.dumps(msg))
        if num_sent == 0:
            return 0

        return 1

    @staticmethod
    def ReadMsgEl(key, msg):
        if isinstance(key, list) or isinstance(key, tuple):
            return tuple([ (msg[k] if k in msg else None) for k in key])
        else:
            return msg[key] if key in msg else None

    @staticmethod
    def SendMsgEl(key, val, msg):
        if isinstance(key, list) or isinstance(key, tuple):
            for k, v in zip(key, val):
                if v != None:
                    msg[k] = v
        elif v != None:
            msg[k] = v

class UnknownMsg(Message):
    def __init__(self, msgtype):
        self.msgtype = msgtype

class HelloMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id

    def sendto(self, client):
        HelloMsg.send(client, self.srv_id, self.cli_id, {})

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[HelloMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class PhysicalPropertiesMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.object_type = Message.ReadMsgEl('\x03', msg)
        self.mass = Message.ReadMsgEl('\x04', msg)
        self.position = Message.ReadMsgEl(('\x05','\x06','\x07'), msg)
        self.velocity = Message.ReadMsgEl(('\x08','\x09','\x0A'), msg)
        self.orientation = Message.ReadMsgEl(('\x0B','\x0C','\x0D','\x0E'), msg)
        self.thrust = Message.ReadMsgEl(('\x0F','\x10','\x11'), msg)
        self.radius = ReadMsgEl('\x12', msg)

    def sendto(self, client):
        msg = {}
        vals = [ self.object_type, self.mass ] + self.position + self.velocity + self.orientation + self.thrust + [ self.radius ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        PhysicalPropertiesMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[PhysicalPropertiesMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

    @staticmethod
    def make_from_object(obj, p = zero3d, v = zero3d):
        msg = {}
        vals = [ obj.object_type, obj.mass ] +
            (obj.position + p) +
            (obj.velocity + v) +
            obj.orientation + obj.thrust + [ obj.radius ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)

        return msg

#class VisualPropertiesMsg(Message):
    #def __init__(self, s, srv_id, cli_id):
        #self.srv_id = srv_id
        #self.cli_id = cli_id
        #self.mesh = Message.read_mesh(s)
        #self.texture = Message.read_texture(s)

    ##def sendto(self, client):
        ##VisualPropertiesMsg.send(client, self.srv_id, self.cli_id,
            ##[ self.mesh, self.texture ])

    #@staticmethod
    #def send(client, srv_id, cli_id, args):
        #msg = "VISPROPS\n"
        #msg += Message.prep_mesh(args[0])
        #msg += Message.prep_texture(args[1])

        #ret = Message.sendall(client, srv_id, cli_id, msg)
        #return ret

class VisualDataEnableMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.enabled = Message.ReadMsgEl('\x03', msg)

    def sendto(self, client):
        msg = {}
        Message.SendMsgEl('\x03', self.enabled, msg)
        VisualDataEnableMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[VisualDataEnableMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

#class VisualMetaDataEnableMsg(Message):
    #def __init__(self, s, srv_id, cli_id):
        #self.srv_id = srv_id
        #self.cli_id = cli_id
        #self.enabled = Message.read_int(s)

    ##def sendto(self, client):
        ##VisualMetaDataEnableMsg.send(client, self.srv_id, self.cli_id, self.enabled)

    #@staticmethod
    #def send(client, srv_id, cli_id, arg):
        #msg = "VISMETADATAENABLE\n%d\n" % arg

        #ret = Message.sendall(client, srv_id, cli_id, msg)
        #return ret

#class VisualMetaDataMsg(Message):
    #def __init__(self, s, srv_id, cli_id):
        #self.srv_id = srv_id
        #self.cli_id = cli_id
        #self.art_id = Message.read_int(s)
        #self.mesh = Message.read_mesh(s)
        #self.texture = Message.read_texture(s)

    ##def sendto(self, client):
        ##VisualMetaDataMsg.send(client, self.srv_id, self.cli_id, [ self.art_id, self.mesh, self.texture ])

    #@staticmethod
    #def send(client, srv_id, cli_id, args):
        #msg = "VISMETADATA\n"

        #art_id = args[0]
        #mesh = args[1]
        #texture = args[2]
        #msg += "%d\n" % art_id
        #msg += Message.prep_mesh(mesh) + "\n"
        #msg += Message.prep_texture(texture) + "\n"

        #ret = Message.sendall(client, srv_id, cli_id, msg)
        #return ret

class VisualDataMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.phys_id = Message.ReadMsgEl('\x03', msg)
        self.radius = Message.ReadMsgEl('\x04', msg)
        self.position = Message.ReadMsgEl(('\x05','\x06','\x07'), msg)
        self.orientation = Message.ReadMsgEl(('\x0B','\x0C','\x0D','\x0E'), msg)

    def sendto(self, client):
        msg = {}
        vals = [ self.phys_id, self.radius ] + self.position + self.orientation
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        VisualDataMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[VisualDataMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class BeamMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
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

    def sendto(self, client):
        msg = {}
        vals = self.origin + self.velocity + self.up +
            [ self.spread_h, self.spread_v, self.energy, self.beam_type, self.comm_msg ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        BeamMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[BeamMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class CollisionMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.position = Message.ReadMsgEl(('\x03','\x04','\x05'), msg)
        self.direction = Message.ReadMsgEl(('\x06','\x07','\x08'), msg)
        self.energy = Message.ReadMsgEl('\x09', msg)
        self.collision_type = Message.ReadMsgEl('\x0A', msg)
        self.comm_msg = Message.ReadMsgEl('\x0B', msg)

    def sendto(self, client):
        msg = {}
        vals = self.position + self.direction + [ self.energy + self.beam_type, self.comm_msg ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        CollisionMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[CollisionMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class SpawnMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.mass = Message.ReadMsgEl('\x03', msg)
        self.position = Message.ReadMsgEl(('\x04','\x05','\x06'), msg)
        self.velocity = Message.ReadMsgEl(('\x07','\x08','\x09'), msg)
        self.orientation = Message.ReadMsgEl(('\x0A','\x0B','\x0C','\x0D'), msg)
        self.thrust = Message.ReadMsgEl(('\x0E','\x0F','\x10'), msg)
        self.radius = Message.ReadMsgEl('\x11', msg)

    def sendto(self, client):
        msg = {}
        vals = [ self.mass ] + self.position + self.velocity + self.orientation + self.thrust + [ self.radius ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        SpawnMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[SpawnMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class ScanResultMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.mass = Message.ReadMsgEl('\x03', msg)
        self.position = Message.ReadMsgEl(('\x04','\x05','\x06'), msg)
        self.velocity = Message.ReadMsgEl(('\x07','\x08','\x09'), msg)
        self.orientation = Message.ReadMsgEl(('\x0A','\x0B','\x0C','\x0D'), msg)
        self.thrust = Message.ReadMsgEl(('\x0E','\x0F','\x10'), msg)
        self.radius = Message.ReadMsgEl('\x11', msg)
        self.data = Message.ReadMsgEl('\x12', msg)

    def sendto(self, client):
        msg = {}
        vals = [ self.mass ] + self.position + self.velocity + self.orientation + self.thrust + [ self.radius, self.data ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        ScanResultMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[ScanResultMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class ScanQueryMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.scan_id = Message.ReadMsgEl('\x03', msg)
        self.scan_energy = Message.ReadMsgEl('\x04', msg)
        self.scan_dir = Message.ReadMsgEl(('\x05','\x06','\x07'), msg)

    def sendto(self, client):
        msg = {}
        vals = [ self.scan_id, self.scan_energy ] + self.scan_dir
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        ScanQueryMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[ScanQueryMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)

class ScanResponseMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.scan_id = Message.ReadMsgEl('\x03', msg)
        self.data = Message.ReadMsgEl('\x04', msg)

    def sendto(self, client):
        msg = {}
        vals = [ self.scan_id, self.data ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        ScanResponseMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[ScanResponseMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)

class GoodbyeMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id

    def sendto(self, client):
        GoodbyeMsg.send(client, self.srv_id, self.cli_id, {})

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[GoodbyeMsg]
        ret = Message.sendall(client, srv_id, cli_id, {})
        return ret

#class DirectoryMsg(Message):
    #def __init__(self, msg, msgtype, srv_id, cli_id):
        #self.srv_id = srv_id
        #self.cli_id = cli_id
        #self.item_type = s[0]
        #del s[0]

        #self.items = []
        #for i in range(0, len(s) - (len(s) % 2), 2):
            #self.items.append([int(s[i]), s[i+1]])

    ##def sendto(self, client):
        ##DirectoryMsg.send(client, self.srv_id, self.cli_id, [self.item_type] + self.items)

    #@staticmethod
    #def send(client, srv_id, cli_id, args):
        #msg = "DIRECTORY\n%s\n" % args[0]

        #del args[0]
        #for a in args:
            #msg += "%d\n%s\n" % (a[0], a[1])

        #ret = Message.sendall(client, srv_id, cli_id, msg)
        #return ret

class NameMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.name = Message.ReadMsgEl('\x03', msg)

    def sendto(self, client):
        msg = {}
        val = [ self.name ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        NameMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[NameMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class ReadyMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.ready = Message.ReadMsgEl('\x03', msg)

    def sendto(self, client):
        msg = {}
        val = [ self.ready ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        ReadyMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[ReadyMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret


##Here begins the client <-> ship messages
class ThrustMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.thrust = Message.ReadMsgEl(('\x03','\x04','\x05'), msg)

    def sendto(self, client):
        msg = {}
        val = [ self.thrust ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        ThrustMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[ThrustMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class VelocityMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.velocity = Message.ReadMsgEl(('\x03','\x04','\x05'), msg)

    def sendto(self, client):
        msg = {}
        val = [ self.velocity ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        VelocityMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[VelocityMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class JumpMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.new_position = Message.ReadMsgEl(('\x03','\x04','\x05'), msg)

    def sendto(self, client):
        msg = {}
        val = [ self.new_position ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        JumpMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[JumpMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class InfoUpdateMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.type = Message.ReadMsgEl('\x03', msg)
        self.data = Message.ReadMsgEl('\x04', msg)

    def sendto(self, client):
        msg = {}
        val = [ self.type, self.data ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        InfoUpdateMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[InfoUpdateMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

class RequestUpdateMsg(Message):
    def __init__(self, msg, msgtype, srv_id, cli_id):
        self.msgtype = msgtype
        self.srv_id = srv_id
        self.cli_id = cli_id
        self.type = Message.ReadMsgEl('\x03', msg)
        self.continuous = Message.ReadMsgEl('\x04', msg)

    def sendto(self, client):
        msg = {}
        val = [ self.type, self.continuous ]
        Message.SendMsgEl([chr(i) for i in range(3,3+len(vals))], vals, msg)
        RequestUpdateMsg.send(client, self.srv_id, self.cli_id, msg)

    @staticmethod
    def send(client, srv_id, cli_id, msg):
        msg['MsgType'] = MessageTypeIDs[RequestUpdateMsg]
        ret = Message.sendall(client, srv_id, cli_id, msg)
        return ret

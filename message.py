#!/usr/bin/env pytho\n

import sys
from socket import timeout

class Message:
    def __init__(self, client):
        pass

    @staticmethod
    def get_message_size(client):
        try:
            msg_length = int(client.recv(10).rstrip())
            # We really don't want zero-length messages
            if msg_length > 0:
                return msg_length
            else:
                return None
        except timeout as e:
            raise e
        except:
            print "There was an error getting message size header from client %d" % client.fileno()
            print "Error:", sys.exc_info()
            return None

    @staticmethod
    def big_read(client, num_bytes):
        num_got = 0

        from cStringIO import StringIO
        file_str = StringIO()

        while num_got < num_bytes:
            if client.fileno() == -1:
                break

            cur_read = min(4096, num_bytes - num_got)
            try:
                cur_msg = client.recv(cur_read)
            except:
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
                except:
                    print "There was an error sending message to client %d" % client.fileno()
                    print "Error:", sys.exc_info()
                    return num_sent
                num_sent += cur_sent

            return num_sent
        else:
            try:
                client.sendall(msg)
            except:
                if client.fileno() != -1:
                    print "There was an error sending message to client %d" % client.fileno()
                    print "Error:", sys.exc_info()
                return 0


    @staticmethod
    # Reads a single message from the socket and returns that object.
    def get_message(client):
        # First grab the message size
        msg_size = Message.get_message_size(client)
        if msg_size == None:
            print "Error getting message length"
            return None

        msg = Message.big_read(client, msg_size).split("\n")

        if not msg == None:
            msgtype = msg[0]
            del msg[0]

            if msgtype in MessageTypes:
                m = MessageTypes[msgtype](msg)
                return m
            else:
                print "Unknown message type: \"%s\"" % msgtype
                sys.stdout.flush()
                return None
        else:
            return None

    @staticmethod
    def sendall(client, msg):
        # Detect a hangup
        if client.fileno() == -1:
            return 0

        msg_len = "%09d\n" % len(msg)
        num_sent = Message.big_send(client, msg_len)
        if num_sent == 0:
            return 0
        Message.big_send(client, msg)
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
        s = []
        s.append(Message.prep_double(d1))
        s.append(Message.prep_double(d2))
        s.append(Message.prep_double(d3))

    @staticmethod
    def read_double(sa):
        s = sa[0]
        del sa[0]

        if s != "":
            try:
                f = float(s)
                return f
            except:
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
    def read_int(sa):
        s = sa[0]
        del sa[0]
        
        if s != "":
            try:
                f = int(s)
                return f
            except:
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
    def __init__(self, s):
            self.endpoint_id = Message.read_int(s)

    @staticmethod
    def send(client, args):
        msg = "HELLO\n%d\n" % args

        ret = Message.sendall(client, msg)
        return ret

class PhysicalPropertiesMsg(Message):
    def __init__(self, s):
        self.mass = Message.read_double(s)
        self.position = Message.read_double3(s)
        self.velocity = Message.read_double3(s)
        self.orientation = Message.read_double3(s)
        self.thrust = Message.read_double3(s)
        self.radius = Message.read_double(s)

    @staticmethod
    def send(client, args):
        msg = "PHYSPROPS\n"
        for i in range(0,14):
            msg += Message.prep_double(args[i]) + "\n"

        ret = Message.sendall(client, msg)
        return ret

class VisualPropertiesMsg(Message):
    def __init__(self, s):
        self.mesh = Message.read_mesh(s)
        self.texture = Message.read_texture(s)

    @staticmethod
    def send(client, args):
        msg = "VISPROPS\n"
        msg += Message.prep_mesh(args[0])
        msg += Message.prep_texture(args[1])

        ret = Message.sendall(client, msg)
        return ret

class VisualDataEnableMsg(Message):
    def __init__(self, s):
        sys.stdout.flush()
        self.enabled = Message.read_int(s)

    @staticmethod
    def send(client, arg):
        msg = "VISDATAENABLE\n%d\n" % arg

        ret = Message.sendall(client, msg)
        return ret

class VisualMetaDataEnableMsg(Message):
    def __init__(self, s):
        self.enabled = Message.read_int(s)

    @staticmethod
    def send(client, arg):
        msg = "VISMETADATAENABLE\n%d\n" % arg

        ret = Message.sendall(client, msg)
        return ret


class VisualMetaDataMsg(Message):
    def __init__(self, s):
        self.art_id = Message.read_int(s)
        self.mesh = Message.read_mesh(s)
        self.texture = Message.read_texture(s)

    @staticmethod
    def send(client, args):
        msg = "VISMETADATA\n"

        art_id = args[0]
        mesh = args[1]
        texture = args[2]
        msg += "%d\n" % art_id
        msg += Message.prep_mesh(mesh) + "\n"
        msg += Message.prep_texture(texture) + "\n"

        ret = Message.sendall(client, msg)
        return ret

class VisualDataMsg(Message):
    def __init__(self, s):
        self.phys_id = Message.read_int(s)
        self.radius = Message.read_double(s)
        self.position = Message.read_double3(s)
        self.orientation = Message.read_double3(s)

    @staticmethod
    # This one is special, since these updates will be going to multiple clients
    # at the same time, quite frequently, we shouldn't need to rebuild this for
    # every client.
    #
    # That means that the args in this case are exactly the string we want to send.
    def send(client, args):
        ret = Message.sendall(client, args)
        return ret

class BeamMsg(Message):
    MsgTypes = { "SCAN": 0, "WEAP": 1, "COMM": 2 }
    
    def __init__(self, s):
        self.origin = Message.read_double3(s)
        self.velocity = Message.read_double3(s)
        self.up = Message.read_double3(s)
        self.spread_h = Message.read_double(s)
        self.spread_v = Message.read_double(s)
        self.energy = Message.read_double(s)

        if s[0] == "SCAN":
            self.beam_type = BeamMsg.MsgTypes[s[0]]
        elif s[0] == "WEAP":
            self.beam_type = BeamMsg.MsgTypes[s[0]]
        elif s[0] == "COMM":
            self.beam_type = BeamMsg.MsgTypes[s[0]]
            self.msg = ""
            del s[0]
            for line in s:
                msg += line + "\n"
        else:
            self.beam_type = None
            

    @staticmethod
    def send(client, args):
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

        ret = Message.sendall(client, msg)
        return ret

MessageTypes = { "HELLO": HelloMsg,
                "PHYSPROPS": PhysicalPropertiesMsg,
                "VISPROPS": VisualPropertiesMsg,
                "VISDATAENABLE": VisualDataEnableMsg,
                "VISMETADATAENABLE": VisualMetaDataEnableMsg,
                "VISMETADATA": VisualMetaDataMsg,
                "VISDATA": VisualDataMsg,
                "BEAM": BeamMsg }

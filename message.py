#!/usr/bin/env pytho\n

import sys

class Message:
    @staticmethod
    # Reads a single message from the socket and returns that object.
    def get_message(client):
        f = client.makefile()

        # The first line should be
        msgtype = f.readline().rstrip()
        if msgtype in MessageTypes:
            return MessageTypes[msgtype](f)
        else:
            print "Unknown message type: \"%s\"" % msgtype
            sys.stdout.flush()
            return None

    @staticmethod
    def sendall(client, msg):
        try:
            client.sendall(msg)
            return 1
        except:
            print "There was an error sending to client %d" % client.fileno()
            print "Error:", sys.exc_info()[0]
            return 0

    @staticmethod
    def prep_double(d):
        if d != None and d != "":
            s = "%.55f" % d
        else:
            s = ""
        return s

    @staticmethod
    def read_double(s):
        if s != "":
            try:
                f = float(s)
                return f
            except:
                print "Error:", sys.exc_info()[0]
                return None
        else:
            return None

    @staticmethod
    def prep_mesh(m):
        return m

    @staticmethod
    def read_mesh(m):
        if m != "":
            return m
        else:
            return None

    @staticmethod
    def prep_texture(t):
        return t

    @staticmethod
    def read_texture(t):
        if t != "":
            return t
        else:
            return None

class UnknownMsg(Message):
    def __init__(self, msgtype):
        self.msgtype = msgtype

class HelloMsg(Message):
    def __init__(self, f):
        try:
            self.endpoint_id = int(f.readline().rstrip())
        except:
            print "Error:", sys.exc_info()[0]
            self.endpoint_id = None

    @staticmethod
    def send(client, args):
        msg = "HELLO\n%d\n" % args

        ret = Message.sendall(client, msg)
        return ret

class PhysicalPropertiesMsg(Message):
    def __init__(self, f):
        tmp = Message.read_double(f.readline().rstrip())
        if tmp:
            self.mass = tmp
        else:
            self.mass = None

        tmp = [ Message.read_double(f.readline().rstrip()),
                Message.read_double(f.readline().rstrip()),
                Message.read_double(f.readline().rstrip()) ]
        if tmp[0] and tmp[1] and tmp[2]:
            self.position = tmp
        else:
            self.position = None

        tmp = [ Message.read_double(f.readline().rstrip()),
                Message.read_double(f.readline().rstrip()),
                Message.read_double(f.readline().rstrip()) ]
        if tmp[0] and tmp[1] and tmp[2]:
            self.velocity = tmp
        else:
            self.velocity = None

        tmp = [ Message.read_double(f.readline().rstrip()),
                Message.read_double(f.readline().rstrip()),
                Message.read_double(f.readline().rstrip()) ]
        if tmp[0] and tmp[1] and tmp[2]:
            self.orientation = tmp
        else:
            self.orientation = None

        tmp = [ Message.read_double(f.readline().rstrip()),
                Message.read_double(f.readline().rstrip()),
                Message.read_double(f.readline().rstrip()) ]
        if tmp[0] and tmp[1] and tmp[2]:
            self.thrust = tmp
        else:
            self.thrust = None

        tmp = Message.read_double(f.readline().rstrip())
        if tmp:
            self.radius = tmp
        else:
            self.radius = None

    @staticmethod
    # This won't ever actually get called, but it is instructive to see how it is
    # implemented.
    def send(client, args):
        msg = "PHYSPROPS\n"
        msg += Message.prep_double(args[0]) + "\n"
        msg += Message.prep_double(args[1]) + "\n"
        msg += Message.prep_double(args[2]) + "\n"
        msg += Message.prep_double(args[3]) + "\n"
        msg += Message.prep_double(args[4]) + "\n"
        msg += Message.prep_double(args[5]) + "\n"
        msg += Message.prep_double(args[6]) + "\n"
        msg += Message.prep_double(args[7]) + "\n"
        msg += Message.prep_double(args[8]) + "\n"
        msg += Message.prep_double(args[9]) + "\n"
        msg += Message.prep_double(args[10]) + "\n"
        msg += Message.prep_double(args[11]) + "\n"
        msg += Message.prep_double(args[12]) + "\n"
        msg += Message.prep_double(args[13]) + "\n"
        
        ret = Message.sendall(client, msg)
        return ret

class VisualPropertiesMsg(Message):
    def __init__(self, f):
        self.mesh = Message.read_mesh(f.readline().rstrip())
        self.texture = Message.read_texture(f.readline().rstrip())

class VisualDataEnableMsg(Message):
    def __init__(self, f):
        self.enabled = int(f.readline().rstrip())

class VisualMetaDataEnableMsg(Message):
    def __init__(self, f):
        self.enabled = int(f.readline().rstrip())

class VisualMetaDataMsg(Message):
    @staticmethod
    def send(client, args):
        msg = "VISMETADATA\n%d\n" % len(args)

        for o in args:
            phys_id = o[0]
            mesh = o[1]
            texture = o[2]
            msg += "%d\n" % phys_id
            msg += Message.prep_mesh(mesh) + "\n"
            msg += Message.prep_texture(texture) + "\n"

        ret = Message.sendall(client, msg)
        return ret

class VisualDataMsg(Message):
    @staticmethod
    # This one is special, since these updates will be going to multiple clients
    # at the same time, quite frequently, we shouldn't need to rebuild this for
    # every client.
    #
    # That means that the args in this case are exactly the string we want to send.
    def send(client, args):
        ret = Message.sendall(client, args)
        return ret

MessageTypes = { "HELLO": HelloMsg,
                "PHYSPROPS": PhysicalPropertiesMsg,
                "VISPROPS": VisualPropertiesMsg,
                "VISDATAENABLE": VisualDataEnableMsg,
                "VISMETADATAENABLE": VisualMetaDataEnableMsg,
                "VISMETADATA": VisualMetaDataMsg,
                "VISDATA": VisualDataMsg }

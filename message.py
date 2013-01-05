#!/usr/bin/env pytho\n

import sys

class Message:
    def __init__(self, client):
        pass

    @staticmethod
    def get_message_size(client):
        try:
            msg_length = int(client.recv(10).rstrip())
            return msg_length
        except:
            print "There was an error getting message header from client %d" % client.fileno()
            print "Error:", sys.exc_info()[0]
            return None

    @staticmethod
    def big_read(client, num_bytes):
        num_got = 0

        from cStringIO import StringIO
        file_str = StringIO()
        
        while num_got < num_bytes:
            cur_read = min(4096, num_bytes - num_got)
            cur_msg = client.recv(cur_read)
            num_got += len(cur_msg)
            file_str.write(cur_msg)

        return file_str.getvalue()
            
        
    @staticmethod
    # Reads a single message from the socket and returns that object.
    def get_message(client):
        # First grab the message size
        msg_size = Message.get_message_size(client)
        if msg_size == None:
            print "Error getting message length"
            return None
            
        msg = Message.big_read(client, msg_size).split("\n")
        msgtype = msg[0]
        del msg[0]

        if msgtype in MessageTypes:
            m = MessageTypes[msgtype](msg)
            return m
        else:
            print "Unknown message type: \"%s\"" % msgtype
            sys.stdout.flush()
            return None
        pass

    @staticmethod
    def sendall(client, msg):
        try:
            msg_len = "%09d\n" % len(msg)
            client.sendall(msg_len)
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
    def read_int(s):
        if s != "":
            try:
                f = int(s)
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
    def __init__(self, s):
        try:
            self.endpoint_id = Message.read_int(s[0].rstrip())
        except:
            print "Error:", sys.exc_info()[0]
            self.endpoint_id = None

    @staticmethod
    def send(client, args):
        msg = "HELLO\n%d\n" % args

        ret = Message.sendall(client, msg)
        return ret

class PhysicalPropertiesMsg(Message):
    def __init__(self, s):
        tmp = Message.read_double(s[0].rstrip())
        if tmp:
            self.mass = tmp
        else:
            self.mass = None

        tmp = [ Message.read_double(s[1].rstrip()),
                Message.read_double(s[2].rstrip()),
                Message.read_double(s[3].rstrip()) ]
        if tmp[0] and tmp[1] and tmp[2]:
            self.position = tmp
        else:
            self.position = None

        tmp = [ Message.read_double(s[4].rstrip()),
                Message.read_double(s[5].rstrip()),
                Message.read_double(s[6].rstrip()) ]
        if tmp[0] and tmp[1] and tmp[2]:
            self.velocity = tmp
        else:
            self.velocity = None

        tmp = [ Message.read_double(s[7].rstrip()),
                Message.read_double(s[8].rstrip()),
                Message.read_double(s[9].rstrip()) ]
        if tmp[0] and tmp[1] and tmp[2]:
            self.orientation = tmp
        else:
            self.orientation = None

        tmp = [ Message.read_double(s[10].rstrip()),
                Message.read_double(s[11].rstrip()),
                Message.read_double(s[12].rstrip()) ]
        if tmp[0] and tmp[1] and tmp[2]:
            self.thrust = tmp
        else:
            self.thrust = None

        tmp = Message.read_double(s[13].rstrip())
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
    def __init__(self, s):
        self.mesh = Message.read_mesh(s[0].rstrip())
        self.texture = Message.read_texture(s[1].rstrip())

class VisualDataEnableMsg(Message):
    def __init__(self, s):
        self.enabled = Message.read_int(s[0].rstrip())
        
    @staticmethod
    def send(client, arg):
        msg = "VISDATAENABLE\n%d\n" % arg
        
        ret = Message.sendall(client, msg)
        return ret

class VisualMetaDataEnableMsg(Message):
    def __init__(self, s):
        self.enabled = Message.read_int(s[0].rstrip())

    @staticmethod
    def send(client, arg):
        msg = "VISMETADATAENABLE\n%d\n" % arg
        
        ret = Message.sendall(client, msg)
        return ret


class VisualMetaDataMsg(Message):
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

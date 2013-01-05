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
    def send_double(client, d):
        if d != None and d != "":
            s = "%.55f" % d
            client.send(s)

    @staticmethod
    def read_double(f):
        l = f.readline().rstrip()
        if l != "":
            return float(l)
        else:
            return None

class UnknownMsg(Message):
    def __init__(self, msgtype):
        self.msgtype = msgtype

class HelloMsg(Message):
    def __init__(self, f):
        self.endpoint_id = int(f.readline().rstrip())

    @staticmethod
    def send(client, args):
        msg = "HELLO\n%d\n" % args
        client.send(msg)

class PhysicalPropertiesMsg(Message):
    # Order of properties:
    # - Mass
    # - Orientation
    def __init__(self, f):
        tmp = Message.read_double(f)
        if tmp:
            self.mass = tmp
        else:
            self.mass = None

        tmp = [ Message.read_double(f), Message.read_double(f), Message.read_double(f) ]
        if tmp[0]:
            self.position = tmp
        else:
            self.position = None

        tmp = [ Message.read_double(f), Message.read_double(f), Message.read_double(f) ]
        if tmp[0]:
            self.velocity = tmp
        else:
            self.velocity = None

        tmp = [ Message.read_double(f), Message.read_double(f), Message.read_double(f) ]
        if tmp[0]:
            self.orientation = tmp
        else:
            self.orientation = None

        tmp = [ Message.read_double(f), Message.read_double(f), Message.read_double(f) ]
        if tmp[0]:
            self.thrust = tmp
        else:
            self.thrust = None

        tmp = Message.read_double(f)
        if tmp:
            self.radius = tmp
        else:
            self.radius = None

    @staticmethod
    def send(client, args):
        client.send("PHYSPROPS\n")
        Message.send_double(client, args[0])
        client.send("\n")
        Message.send_double(client, args[1])
        client.send("\n")
        Message.send_double(client, args[2])
        client.send("\n")
        Message.send_double(client, args[3])
        client.send("\n")
        Message.send_double(client, args[4])
        client.send("\n")
        Message.send_double(client, args[5])
        client.send("\n")
        Message.send_double(client, args[6])
        client.send("\n")
        Message.send_double(client, args[7])
        client.send("\n")
        Message.send_double(client, args[8])
        client.send("\n")
        Message.send_double(client, args[9])
        client.send("\n")
        Message.send_double(client, args[10])
        client.send("\n")
        Message.send_double(client, args[11])
        client.send("\n")
        Message.send_double(client, args[12])
        client.send("\n")
        Message.send_double(client, args[13])
        client.send("\n")

MessageTypes = { "HELLO": HelloMsg, "PHYSPROPS": PhysicalPropertiesMsg }

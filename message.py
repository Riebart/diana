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

class UnknownMsg(Message):
    def __init__(self, msgtype):
        self.msgtype = msgtype

class HelloMsg(Message):
    def __init__(self, f):
        self.endpoint_id = f.readline().rstrip()

    @staticmethod
    def build(f):
        msg = HelloMsg(f)
        return msg

    @staticmethod
    def send(client, endpoint_id):
        msg = "HELLO\n%d" % endpoint_id
        client.send(msg)

MessageTypes = { "HELLO": HelloMsg }
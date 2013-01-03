#!/usr/bin/env python

# Source: http://code.activestate.com/recipes/531824/

"""
A basic, multiclient 'chat server' using Python's select module
with interrupt handling.

Entering any line of input at the terminal will exit the server.
"""

import select
import socket
import sys
import signal
import thread

import cPickle
import struct

marshall = cPickle.dumps
unmarshall = cPickle.loads

def send(channel, *args):
    buf = marshall(args)
    value = socket.htonl(len(buf))
    size = struct.pack("L",value)
    channel.send(size)
    channel.send(buf)

def receive(channel):
    size = struct.calcsize("L")
    size = channel.recv(size)
    try:
        size = socket.ntohl(struct.unpack("L", size)[0])
    except struct.error, e:
        return ''

    buf = ""

    while len(buf) < size:
        buf = channel.recv(size - len(buf))

    return unmarshall(buf)[0]


class MIMOServer:
    """ Simple chat server using select """

    def __init__(self, callback, port=5505, backlog=5):
        self.callback = callback
        self.clients = 0
        self.clientmap = []
        # Output socket list
        self.outputs = []
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server.bind(('',port))
        print 'Listening to port',port,'...'
        self.server.listen(backlog)
        
        # Trap keyboard interrupts
        signal.signal(signal.SIGINT, self.sighandler)

    def sighandler(self, signum, frame):
        # Close the server
        print 'Shutting down server...'
        # Close existing client sockets
        for o in self.outputs:
            o.close()
        self.server.close()

    def serve(self):

        inputs = [self.server]
        self.outputs = []

        running = 1

        while running:

            sys.stdout.flush()
            try:
                inputready,outputready,exceptready = select.select(inputs, self.outputs, [])
            except select.error, e:
                print e
                break
            except socket.error, e:
                print e
                break

            for s in inputready:
                if s == self.server:
                    # handle the server socket
                    client, address = self.server.accept()
                    print 'got connection %d from %s' % (client.fileno(), address)
                    context = callback(client)
                    self.clientmap[client] = context
                    self.clients += 1
                    inputs.append(client)
                    thread.start_new_thread(callback, (client,))
                    self.outputs.append(client)
                else:
                    # handle all other sockets
                    try:
                        ret = clientmap[s].handle()
                        
                        if not ret:
                            print '%d hung up' % s.fileno()
                            self.clients -= 1
                            s.close()
                            inputs.remove(s)
                            self.outputs.remove(s)

                    except socket.error, e:
                        # Remove
                        inputs.remove(s)
                        self.outputs.remove(s)

        self.server.close()

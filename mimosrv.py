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
    def __init__(self, callback, port=5505, backlog=5):
        self.running = 0
        self.callback = callback
        
        self.clients = 0
        self.contextmap = dict()
        self.outputs = []
        
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.port = port
        self.backlog = backlog
        
        # Trap keyboard interrupts
        signal.signal(signal.SIGINT, self.sighandler)

    def stop(self):
        self.running = 0
        thread.join(self.serve_thread)
        # Close the server
        print 'Shutting down server...'
        # Close existing client sockets
        for o in self.outputs:
            o.close()
        self.server.close()

    def sighandler(self, signum, frame):
        self.shutdown()

    def start(self):
        if self.running == 0:
            self.server.bind(('', self.port))
            print 'Listening to port', self.port, '...'
            self.server.listen(self.backlog)
            self.serve_thread = thread.start_new_thread(self.serve, ())

    # This gets called when a client hangs up on us.
    def hangup(self, client):
        print '%d hung up' % client.fileno()
        sys.stdout.flush()
        self.clients -= 1
        client.close()
        self.inputs.remove(client)
        self.outputs.remove(client)
        del self.contextmap[client]
        
    def serve(self):
        self.inputs = [self.server]
        self.outputs = []
        self.running = 1

        while self.running:

            try:
                inputready,outputready,exceptready = select.select(self.inputs, self.outputs, [])
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
                    sys.stdout.flush()
                    context = self.callback(client)
                    self.contextmap[client] = context
                    #thread.start_new_thread(context.handle, ())
                    context.handle()

                    self.clients += 1
                    self.inputs.append(client)
                    self.outputs.append(client)
                else:
                    # handle all other sockets
                    try:
                        #thread.start_new_thread(self.contextmap[s].handle, ())
                        self.handlingmap[s] = 1
                        self.contextmap[s].handle()

                    except socket.error, e:
                        # Remove
                        inputs.remove(s)
                        self.outputs.remove(s)
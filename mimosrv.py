#!/usr/bin/env python

# Source: http://code.activestate.com/recipes/531824/

from __future__ import print_function
import select
import socket
import sys
import signal
import thread
import threading
import traceback
import struct
from message import Message

# ==============================================================================
# This implements sending serializable objects back and forth. We won't use it
# but this is kept around for fun

#import cPickle
#import struct

#marshall = cPickle.dumps
#unmarshall = cPickle.loads

#def send(channel, *args):
    #buf = marshall(args)
    #value = socket.htonl(len(buf))
    #size = struct.pack("L",value)
    #channel.send(size)
    #channel.send(buf)

#def receive(channel):
    #size = struct.calcsize("L")
    #size = channel.recv(size)
    #try:
        #size = socket.ntohl(struct.unpack("L", size)[0])
    #except struct.error, e:
        #return ''

    #buf = ""

    #while len(buf) < size:
        #buf = channel.recv(size - len(buf))

    #return unmarshall(buf)[0]

# ==============================================================================

# ==============================================================================

class MIMOServer:
    # ======================================================================

    class ThreadServer(threading.Thread):
        def __init__(self, server):
            threading.Thread.__init__(self)
            self.server = server

        def run(self):
            self.server.serve()

    # ======================================================================
    # ======================================================================

    class ThreadSocket(threading.Thread):
        def __init__(self, client, handler, on_hangup):
            threading.Thread.__init__(self)
            self.client = client
            self.handler = handler
            self.on_hangup = on_hangup
            self.running = 1

        def run(self):
            while self.running:
                if not self.running:
                    break

                response = Message.get_message(self.client)
                self.handler(response)

            sys.stdout.flush()
            self.on_hangup(self.client)

        def stop(self):
            self.running = 0
            try:
                self.client.shutdown(socket.SHUT_RDWR)
                self.client.close()
            except socket.error as e:
                if e.errno != 10053 and e.errno != 10054:
                    print("Error hanging up %d" % self.client.fileno())
                    print("Error:", sys.exc_info())

    # ======================================================================

    def __init__(self, data_callback, hangup_callback = None, port=5505, backlog=5):
        self.running = 0
        self.port = port
        self.backlog = backlog
        self.data_callback = data_callback
        self.hangup_callback = hangup_callback

        self.threadmap = dict()
        self.inputs = []
        self.hangup_lock = threading.Lock()
        self.hangups = []

    def sighandler(self, signum, frame):
        self.stop()

    def start(self):
        if self.running == 0:
            self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server.bind(('', self.port))
            print('Listening to port', self.port, '...')
            self.server.listen(self.backlog)
            self.inputs = []
            self.running = 1

            self.serve_thread = MIMOServer.ThreadServer(self)
            self.serve_thread.start()

    def stop(self):
        if self.running == 1:
            print("Shutting down server...")
            sys.stdout.flush()
            self.server.close()
            self.running = 0
            self.serve_thread.join()

            stubborn = 0
            while len(self.inputs) > 0:
                print("Hanging up %d %sclient%s" % (len(self.inputs) - 1,
                                                    "stubborn " if stubborn == 1 else "",
                                                    "s" if len(self.inputs) > 2 else ""))
                for c in self.inputs:
                    self.hangup(c)
                stubborn = 1

            print("All hungup")

            self.hangups = []

    # This gets called by a client's thread just before it terminates to tell us
    # that the client hung up
    def on_hangup(self, client):
        self.hangup_lock.acquire()
        self.hangups.append(client)
        self.hangup_lock.release()

    # This gets called when a client hangs up on us.
    def hangup(self, client):
        print("Hanging up %d" % client.fileno())
        if client.fileno() == -1:
            # Detect an already hung-up client
            if client in self.inputs:
                self.inputs.remove(client)

            if client in self.threadmap:
                del self.threadmap[client]
            return

        if self.hangup_callback != None:
            self.hangup_callback(client)
        self.threadmap[client].stop()
        self.inputs.remove(client)
        del self.threadmap[client]

    def serve(self):
        while self.running:
            try:
                # ### PARAMETER ### How often the server select times out to service hangups
                inputready, outputready, exceptready = select.select([self.server], [], [], 0.1)
            except select.error as e:
                print(e)
                break
            except socket.error as e:
                print(e)
                break

            if not self.running:
                break

            if len(self.hangups) > 0:
                self.hangup_lock.acquire()
                pre_hangup = len(self.inputs)

                for h in self.hangups:
                    self.hangup(h)

                num_hung_up = pre_hangup - len(self.inputs)
                print("Successfully hung up %d client%s" % (num_hung_up, "s" if num_hung_up > 1 else ""))
                self.hangups = []
                self.hangup_lock.release()
                continue

            if len(inputready) > 0:
                # handle the server socket
                try:
                    client, address = self.server.accept()
                except socket.error as e:
                    pass

                print("got connection %d from %s" % (client.fileno(), address))
                sys.stdout.flush()

                self.threadmap[client] = MIMOServer.ThreadSocket(client, self.data_callback, self.on_hangup)
                self.threadmap[client].start()
                self.inputs.append(client)

# ==============================================================================

class Tester:
    def __init__(self, client):
        self.client = client

    def handle(self, client):
        print("handling %d" % client.fileno())
        sys.stdout.flush()
        print(client.recv(1024))

def cb(client):
    return Tester(client)

if __name__ == "__main__":
    srv = MIMOServer(cb)
    srv.start()
    raw_input("Press Enter to continue...\n")
    srv.stop()
    #raw_input("Press Enter to continue...\n")
    #srv.start()
    #raw_input("Press Enter to continue...\n")
    #srv.stop()

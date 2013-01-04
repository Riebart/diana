#!/usr/bin/env python

# Source: http://code.activestate.com/recipes/531824/

import select
import socket
import sys
import signal
import thread
import threading

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
    
    class ThreadHandler(threading.Thread):
        def __init__(self, client, context, server):
            threading.Thread.__init__(self)
            self.client = client
            self.context = context
            self.server = server
            #self.ran = 0

        def run(self):
            #if self.ran == 0:
                #self.ran = 1
                #return

            self.context.handle(self.client)
            del self.server.handlingmap[self.client]

    # ======================================================================
            
    def __init__(self, callback, port=5505, backlog=5):
        self.running = 0
        self.callback = callback
        
        self.num_clients = 0
        self.contextmap = dict()
        self.handlingmap= dict()
        self.threadmap = dict()
        self.inputs = []
        self.outputs = []

        self.port = port
        self.backlog = backlog
        
        # Trap keyboard interrupts
        signal.signal(signal.SIGINT, self.sighandler)

    def sighandler(self, signum, frame):
        self.stop()

    def start(self):
        if self.running == 0:
            self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server.bind(('', self.port))
            print 'Listening to port', self.port, '...'
            self.server.listen(self.backlog)

            self.serve_thread = MIMOServer.ThreadServer(self)
            self.serve_thread.start()

    def stop(self):
        self.running = 0
        self.server.close()
        self.serve_thread.join()
        # Close the server
        print 'Shutting down server...'
        # Close existing client sockets
        for o in self.outputs:
            o.close()

    # This gets called when a client hangs up on us.
    def hangup(self, client):
        print '%d hung up' % client.fileno()
        sys.stdout.flush()
        self.num_clients -= 1
        client.close()
        
        self.inputs.remove(client)
        self.outputs.remove(client)
        
        #del self.contextmap[client]
        #if self.handlingmap[client]:
            #del self.handlingmap[client]
        
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

            if not self.running:
                break

            for s in inputready:
                if s in self.handlingmap:
                    continue

                if s == self.server:
                    # handle the server socket
                    client, address = self.server.accept()
                    print 'got connection %d from %s' % (client.fileno(), address)
                    sys.stdout.flush()

                    context = self.callback(client)
                    self.contextmap[client] = context
                    self.threadmap[client] = MIMOServer.ThreadHandler(client, context, self)
                    
                    self.num_clients += 1
                    self.inputs.append(client)
                    self.outputs.append(client)
                else:
                    # handle all other sockets
                    try:
                        print "reading from %d" % s.fileno()
                        sys.stdout.flush()

                        # This peeks at the first byte of data. If we don't get
                        # anything, then we know the other end hung up
                        data = s.recv(1, socket.MSG_PEEK)
                        if not data:
                            self.hangup(s)
                            continue

                        # If they didn't hang up on us, let's mark this as
                        # handled so we don't spin through its input ready state
                        # and spool off the handler
                        self.handlingmap[s] = 1
                        self.threadmap[s].run()

                    except socket.error, e:
                        # Remove
                        self.hangup(s)

# ==============================================================================

class Tester:
    def __init__(self, client):
        self.client = client

    def handle(self, client):
        print "handling %d" % client.fileno()
        sys.stdout.flush()
        print client.recv(1024)
        
def cb(client):
    return Tester(client)

if __name__ == "__main__":
    srv = MIMOServer(cb)
    srv.start()
    raw_input("Press Enter to continue...")
    srv.stop()
    #raw_input("Press Enter to continue...")
    #srv.start()
    #raw_input("Press Enter to continue...")
    #srv.stop()
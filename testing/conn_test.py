#!/usr/bin/env python

import socket
import sys
import time

socks = []

msg1 = "000000008\nHELLO\n0\n"
msg2 = "000000069\nPHYSPROPS\n150.2\n1.1\n2.3\n3.5\n4.5\n5.6\n5.7\n6.8\n8.9\n8.7\n9.8\n8.7\n7.2\n15.2\n"

print "msg1 len is %d" % len(msg1)
print "msg2 len is %d" % len(msg2)

dest = ( '10.0.0.220', 5505 )

try:
    num_connections = int(sys.argv[1])
except:
    num_connections = 128

print "Attempting to spawn 128 connections to", dest

for i in range(num_connections):
        print "socket %d start" % i
        s = socket.socket()
        socks.append(s)
        s.connect(dest)
        print "socket %d up" % i
        n = s.send(msg1)
        print "socket %d send1 of %d" % (i, n)
        d = s.recv(1024)
        print "socket %d recv of %d" % (i, len(d))
        n = s.send(msg2)
        print "socket %d send2 of %d" % (i, n)
        sys.stdout.flush()

print "Done spinning up %d sockets" % len(socks)
print "Press enter to continue"
sys.stdout.flush()
raw_input()


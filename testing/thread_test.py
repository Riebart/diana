#!/usr/bin/env python

import thread
import threading
import sys
import time

class MThread(threading.Thread):
        def __init__(self):
                threading.Thread.__init__(self)

        def run(self):
                do_work([1074,4873782,762])

def do_work(args):
        a = args[0]
        b = args[1]
        c = args[2]

        while 1:
                a = b * c
                b = c - b
                c = a + b
                time.sleep(1)

t = []
for i in range(10240):
        t.append(MThread())
        t[i].start()
        print "Running with", threading.active_count()
        sys.stdout.flush()


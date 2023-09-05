#!/usr/bin/env python

import bson
import sys

print(bson.loads(sys.stdin.read()))

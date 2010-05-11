#!/usr/bin/python

import io
import sys

f = open(sys.argv[1],'r')
a = f.read()
for x in a:
	sys.stdout.write('\\x%x' % (ord(x)))

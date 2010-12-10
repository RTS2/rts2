#!/usr/bin/python

# Join filters from filter configuration file

import sys

fil = sys.stdin.readlines()

for x in range(0,len(fil)):
	fil[x] = fil[x].rstrip()

print ':'.join(fil)

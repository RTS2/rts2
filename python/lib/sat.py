#!/usr/bin/python

import ephem
import time
import json

class TLESet:
	"""Set of two line elements satellite data.
	Set can be loaded from a file. Satellite visibility can be computed for
	set of cities."""
	def __init__(self):
		self.sats = []

	def parse_file(self, filename):
		f = open(filename)
		lines = [None,None,None]
		l = f.readline()
		while not(l == ''):
			if l[0] == '1' or l[0] == '2':
				lines[int(l[0])] = l
			elif not(l[0] == '#'):
				lines[0] = l

			if lines[1] is not None and lines[2] is not None:
				if lines[0] is None:
					lines[0] = '#{0}'.format(len(self.sats) + 1)
				self.sats.append(ephem.readtle(*lines))
				lines = []

			l = f.readline()
		f.close()

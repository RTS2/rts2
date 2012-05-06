#!/usr/bin/python
#
# Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
#
# Python interface for call to imgprocess. Parses output, look for RA DEC and offsets
#

import sys
import subprocess
import re

class ImgProcess: 
	def __init__(self):
		self.img_process = '/etc/rts2/img_process'

		self.radecline1 = re.compile('^\d+\s+(\S+)\s+(\S+)\s+\((\S+),(\S+)\)')
		self.radecline2 = re.compile('^correct\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)')

	def run(self,file_path):
		past_call = [self.img_process, file_path]
		proc = subprocess.Popen(past_call,stdout=subprocess.PIPE,stderr=subprocess.PIPE)

		while True:
			a = proc.stdout.readline()
			if a == '':
				break
			match = self.radecline1.match(a)
			if match:
				return (float(match.group(1)),float(match.group(2)),float(match.group(3))/60.0,float(match.group(4))/60.0)
			match = self.radecline2.match(a)
			if match:
				return (float(match.group(1)),float(match.group(2)),float(match.group(3)),float(match.group(4)))
		raise Exception('Astrometry did not succeed')

if __name__ == '__main__':
	p = ImgProcess()
	for f in sys.argv[1:]:
		print p.run(f)

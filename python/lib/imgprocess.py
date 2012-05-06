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

		self.radecline1 = re.compile('^\i+ (\d+) (\d+) \((\d+) (\d+)\)')
		self.radecline2 = re.compile('^correct (\d+) (\d+) (\d+) (\d+)')

	def run(self,file_path):
		past_call = [self.img_process, file_path]
		proc = subprocess.Popen(past_call,stdout=subprocess.PIPE,stderr=subprocess.PIPE)

		while True:
			a = proc.stdout.readline()
			if a == '':
				break
			match = self.radecline1.match(a)
			if match:
				return (match.group(1),match.group(2),match.group(3),match.group(4))
			match = self.radecline2.match(a)
			if match:
				return (match.group(1),match.group(2),match.group(3),match.group(4))
		raise Exception('Astrometry did not succeed')

if __name__ == '__main__':
	p = ImgProcess()
	for f in sys.argv[1:]:
		print p.run(f)

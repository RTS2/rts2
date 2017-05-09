# Python interface for call to imgprocess. Parses output, look for RA DEC and offsets
# Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

import sys
import subprocess
import re

class ImgProcess: 
	def __init__(self):
		self.img_process = '/etc/rts2/img_process'

		self.radecline1 = re.compile('^\d+\s+(\S+)\s+(\S+)\s+\((\S+),(\S+)\)')
		self.radecline2 = re.compile('^correct\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)')

		# print out logs..
		self.logmatch = re.compile('^log')

	def run(self,file_path):
		past_call = [self.img_process, file_path]
		proc = subprocess.Popen(past_call,stdout=subprocess.PIPE,stderr=subprocess.PIPE)

		ret = None

		while True:
			a = proc.stdout.readline()
			if a == '':
				break
			match = self.radecline1.match(a)
			if match:
				ret = (float(match.group(1)),float(match.group(2)),float(match.group(3))/60.0,float(match.group(4))/60.0)
			match = self.radecline2.match(a)
			if match:
				ret = (float(match.group(1)),float(match.group(2)),float(match.group(3)),float(match.group(4)))
			match = self.logmatch.match(a)
			# pass log commands
			if match:
				print(a)
		if ret:
			return ret
		raise Exception('Astrometry did not succeed')

if __name__ == '__main__':
	p = ImgProcess()
	for f in sys.argv[1:]:
		print(p.run(f))

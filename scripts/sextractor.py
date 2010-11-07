#!/usr/bin/python

import sys
import subprocess
import os
from ds9 import *

class Sextractor:
    """Class for a catalogue (SExtractor result)"""
    def __init__(self, filename, fields=['NUMBER', 'FLUXERR_ISO', 'FLUX_AUTO', 'X_IMAGE', 'Y_IMAGE'], sexconfig='/usr/share/sextractor/default.sex'):
        self.filename = filename
	self.sexconfig = sexconfig

	self.fields = fields
        self.objects = []

    def runSExtractor(self):
    	pfn = '/tmp/pysex_{0}'.format(os.getpid())
	output = '/tmp/pysex_out_{0}'.format(os.getpid())
    	pf = open(pfn,'w')
	for f in self.fields:
		pf.write(f + '\n')
	pf.close()

        cmd = ['sextractor', self.filename, '-c ', self.sexconfig, '-PARAMETERS_NAME', pfn, '-DETECT_THRESH', '3', '-FILTER', 'N', '-CATALOG_NAME', output]
       	proc = subprocess.Popen(cmd)
	proc.wait()

	# parse output
	of = open(output,'r')
	while (True):
	 	x=of.readline()
		if x == '':
			break
		if x[0] == '#':
			continue
		self.objects.append(x.split())
	
	# unlink tmp files
	os.unlink(pfn)
	os.unlink(output)

    def sortObjects(self,col):
        """Sort objects by given collumn."""
    	self.objects.sort(cmp=lambda x,y: x[col] < y[col])

    def reverseObjects(self,col):
	"""Reverse sort objects by given collumn."""
	self.objects.reverse(cmp=lambda x,y: x[col] < y[col])

if __name__ == "__main__":
  	d = ds9()
  	for fn in sys.argv[1:]:
		c = Sextractor(fn,['X_IMAGE','Y_IMAGE'])
		c.runSExtractor()

		print fn, c.objects

		# display in ds9
		d.set('file {0}'.format(fn))
		for x in c.objects:
			d.set('regions', 'image; circle {0} {1} 10'.format(x[0],x[1]))


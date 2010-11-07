#!/usr/bin/python

import sys
import subprocess
import os
from ds9 import *

class Sextractor:
    """Class for a catalogue (SExtractor result)"""
    def __init__(self, filename, fields=['NUMBER', 'FLUXERR_ISO', 'FLUX_AUTO', 'X_IMAGE', 'Y_IMAGE'], sexconfig='/usr/share/sextractor/default.sex', threshold=2.7):
        self.filename = filename
	self.sexconfig = sexconfig

	self.fields = fields
        self.objects = []
	self.threshold = threshold

    def runSExtractor(self):
    	pfn = '/tmp/pysex_{0}'.format(os.getpid())
	output = '/tmp/pysex_out_{0}'.format(os.getpid())
    	pf = open(pfn,'w')
	for f in self.fields:
		pf.write(f + '\n')
	pf.close()

        cmd = ['sextractor', self.filename, '-c ', self.sexconfig, '-PARAMETERS_NAME', pfn, '-DETECT_THRESH', str(self.threshold), '-FILTER', 'N', '-STARNNW_NAME', '/usr/share/sextractor/default.nnw', '-CATALOG_NAME', output, '-VERBOSE_TYPE', 'QUIET']
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
		self.objects.append(map(float,x.split()))
	
	# unlink tmp files
	os.unlink(pfn)
	os.unlink(output)

    def sortObjects(self,col):
        """Sort objects by given collumn."""
    	self.objects.sort(cmp=lambda x,y: cmp(x[col],y[col]))

    def reverseObjects(self,col):
	"""Reverse sort objects by given collumn."""
	self.objects.sort(cmp=lambda x,y: cmp(x[col],y[col]))
	self.objects.reverse()

if __name__ == "__main__":
  	d = ds9()
  	for fn in sys.argv[1:]:
		c = Sextractor(fn,['X_IMAGE','Y_IMAGE','MAG_BEST','FLAGS','CLASS_STAR','FWHM_IMAGE','A_IMAGE','B_IMAGE'])
		c.runSExtractor()

		# sort by magnitude
		c.sortObjects(2)

		# display in ds9
		d.set('file {0}'.format(fn))
		i = 0
		fwhm = 0
		a = 0
		b = 0
		for x in c.objects:
		  	if (x[3] == 0 and x[4] != 0):
				d.set('regions', 'image; circle {0} {1} 10'.format(x[0],x[1]))
				fwhm += x[5]
				a += x[6]
				b += x[7]
				i += 1
				if i > 9:
					break 
		print 'with {0} stars, average fwhm={1} a={2} b={3}'.format(i,fwhm/i,a/i,b/i)


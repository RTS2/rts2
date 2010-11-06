#!/usr/bin/python

import subprocess
import os

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

        cmd = ['sextractor', self.filename, '-c ', self.sexconfig, '-PARAMETERS_NAME', pfn, '-FILTER', 'N', '-CATALOG_NAME', output]
       	proc = subprocess.Popen(cmd)
	proc.wait()

	# parse output
	of = open(output,'r')
	while (True):
	 	x=of.readline()
		if x == '':
			break
		self.objects.append(x.split())
	
	# unlink tmp files
	os.unlink(pfn)
	os.unlink(output)

if __name__ == "__main__":
	c = Sextractor('/tmp/20101106075535-050-RA.fits')
	c.runSExtractor()

	print c.objects

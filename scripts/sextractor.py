#!/usr/bin/python
#
# You will need: scipy matplotlib sextractor
# This should work on Debian/ubuntu:
# sudo apt-get install python-matplotlib python-scipy python-pyfits sextractor
#
# If you would like to see sextractor results, get DS9 and pyds9:
#
# http://hea-www.harvard.edu/saord/ds9/
#
# Sextractor-Python wrapper.
#
# (C) 2010  Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

import sys
import subprocess
import os

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

def getFWHM(fn,starsn):
	"""Returns average FWHM of first starsn stars from the image"""
	c = Sextractor(fn,['X_IMAGE','Y_IMAGE','MAG_BEST','FLAGS','CLASS_STAR','FWHM_IMAGE','A_IMAGE','B_IMAGE'])
	c.runSExtractor()

	# sort by magnitude
	c.sortObjects(2)

	# display in ds9
	i = 0
	fwhm = 0
	a = 0
	b = 0
	for x in c.objects:
	  	if (x[3] == 0 and x[4] != 0):
			fwhm += x[5]
			a += x[6]
			b += x[7]
			i += 1
			if i > starsn:
				break 
	if i > 0:
		return float(fwhm) / i

	raise Exception('cannot find any stars on the image')

if __name__ == "__main__":
  	# test method
	from ds9 import *
  	d = ds9()
  	for fn in sys.argv[1:]:
		c = Sextractor(fn,['X_IMAGE','Y_IMAGE','MAG_BEST','FLAGS','CLASS_STAR','FWHM_IMAGE','A_IMAGE','B_IMAGE'])
		c.runSExtractor()

		# sort by magnitude
		c.sortObjects(2)

		# display in ds9
		d.set('file {0}'.format(fn))
		for x in c.objects:
			d.set('regions', 'image; circle {0} {1} 10'.format(x[0],x[1]))

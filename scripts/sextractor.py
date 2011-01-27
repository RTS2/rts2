#!/usr/bin/python
#
# Sextractor-Python wrapper.
#
# You will need: scipy matplotlib sextractor
# This should work on Debian/ubuntu:
# sudo apt-get install python-matplotlib python-scipy python-pyfits sextractor
#
# If you would like to see sextractor results, get DS9 and pyds9:
#
# http://hea-www.harvard.edu/saord/ds9/
#
# Please be aware that current sextractor Ubuntu packages does not work
# properly. The best workaround is to install package, and the overwrite
# sextractor binary with one compiled from sources (so you will have access
# to sextractor configuration files, which program assumes).
#
# (C) 2010  Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place - Suite 330, Boston, MA  02111-1307, USA.

import sys
import subprocess
import os
import tempfile

class Sextractor:
    """Class for a catalogue (SExtractor result)"""
    def __init__(self, filename, fields=['NUMBER', 'FLUXERR_ISO', 'FLUX_AUTO', 'X_IMAGE', 'Y_IMAGE'], sexpath='sextractor', sexconfig='/usr/share/sextractor/default.sex', starnnw='/usr/share/sextractor/default.nnw', threshold=2.7, deblendmin = 0.03, saturlevel=65535):
        self.filename = filename
	self.sexpath = sexpath
	self.sexconfig = sexconfig
	self.starnnw = starnnw

	self.fields = fields
	self.objects = []
	self.threshold = threshold
	self.deblendmin = deblendmin
	self.saturlevel = saturlevel

    def runSExtractor(self):
    	pf,pfn = tempfile.mkstemp()
	ofd,output = tempfile.mkstemp()
	pfi = os.fdopen(pf,'w')
	for f in self.fields:
		pfi.write(f + '\n')
	pfi.flush()

	cmd = [self.sexpath, self.filename, '-c', self.sexconfig, '-PARAMETERS_NAME', pfn, '-DETECT_THRESH', str(self.threshold), '-DEBLEND_MINCONT', str(self.deblendmin), '-SATUR_LEVEL', str(self.saturlevel), '-FILTER', 'N', '-STARNNW_NAME', self.starnnw, '-CATALOG_NAME', output, '-VERBOSE_TYPE', 'QUIET']
	proc = subprocess.Popen(cmd)
	proc.wait()

	# parse output
	of = os.fdopen(ofd,'r')
	while (True):
	 	x=of.readline()
		if x == '':
			break
		if x[0] == '#':
			continue
		self.objects.append(map(float,x.split()))
	
	# unlink tmp files
	pfi.close()
	of.close()

	os.unlink(pfn)
	os.unlink(output)

    def sortObjects(self,col):
        """Sort objects by given collumn."""
    	self.objects.sort(cmp=lambda x,y: cmp(x[col],y[col]))

    def reverseObjects(self,col):
	"""Reverse sort objects by given collumn."""
	self.objects.sort(cmp=lambda x,y: cmp(x[col],y[col]))
	self.objects.reverse()

def getFWHM(fn,starsn,ds9display=False,filterGalaxies=True,threshold=2.7,deblendmin=0.03):
	"""Returns average FWHM of first starsn stars from the image"""
	c = Sextractor(fn,['X_IMAGE','Y_IMAGE','MAG_BEST','FLAGS','CLASS_STAR','FWHM_IMAGE','A_IMAGE','B_IMAGE'],threshold=threshold,deblendmin=deblendmin)
	c.runSExtractor()

	# sort by magnitude
	c.sortObjects(2)

	# display in ds9
	if ds9display:
		import ds9
		d = ds9.ds9()
		d.set('file {0}'.format(fn))

	fwhmlist = []

	a = 0
	b = 0
	for x in c.objects:
		if x[3] == 0 and (filterGalaxies == False or x[4] != 0):
			fwhmlist.append(x[5])
			a += x[6]
			b += x[7]
			if ds9display:
				d.set('regions', 'image; circle {0} {1} 10'.format(x[0],x[1]))
			if len(fwhmlist) >= starsn:
				break
		elif ds9display:
			d.set('regions', 'image; point {0} {1} # point=cross'.format(x[0],x[1]))
			d.set('regions', 'image; text {0} {1} # text={{{2}}}'.format(x[0],x[1] - 15,'{0} {1}'.format(x[3],x[4])))

	if len(fwhmlist) >= starsn:
		import numpy
		return numpy.median(fwhmlist), len(fwhmlist)
#		return numpy.average(fwhmlist), len(fwhmlist)
	if len(fwhmlist) > 0:
		raise Exception('too few stars - {0}, expected {1}'.format(len(fwhmlist),starsn))
	raise Exception('cannot find any stars on the image')

if __name__ == "__main__":
  	# test method
	from ds9 import *
  	d = ds9()
  	for fn in sys.argv[1:]:
		c = Sextractor(fn,['X_IMAGE','Y_IMAGE','MAG_BEST','FLAGS','CLASS_STAR','FWHM_IMAGE','A_IMAGE','B_IMAGE'],sexpath='/home/mink/bin/sex',sexconfig='/home/observer/findfwhm/Sextractor/focus.sex',starnnw='/home/observer/findfwhm/Sextractor/default.nnw')
		c.runSExtractor()

		# sort by magnitude
		c.sortObjects(2)

		print 'from {0} extracted {1} sources'.format(fn,len(c.objects))

		# display in ds9
		d.set('file {0}'.format(fn))
		for x in c.objects:
			d.set('regions', 'image; circle {0} {1} 10'.format(x[0],x[1]))

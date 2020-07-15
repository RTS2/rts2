#!/usr/bin/env python

# test xy2wcs - compares what we get using ds9 and xy2wcs algorithm

from rts2.astrometry import xy2wcs
import sys
import ds9
import pyfits
import re

d=ds9.ds9('XY2WCS')

for x in sys.argv[1:]:
	ff = pyfits.fitsopen(x,'readonly')
	fh=ff[0].header
	ff.close()
	d.set('file {0}'.format(x))
	d.set('regions system wcs')
	xmax = fh['NAXIS1']
	ymax = fh['NAXIS2']
	# pixels to check
	xy = [[0,0],[0,ymax],[xmax,0],[xmax,ymax],[xmax/2.0,ymax/2.0]]
	pmatch = re.compile('point\(([^,]*),([^)]*)\)')
	for p in xy:
		d.set('regions delete all')
		d.set('regions','image; point {0} {1} # point=cross'.format(p[0],p[1]))
		r=d.get('regions')
		radec = xy2wcs(p[0],p[1],fh)
		for l in r.split('\n'):
			match = pmatch.match(l)
			if match:
				ds9radec=[float(match.group(1)),float(match.group(2))]
				print radec,ds9radec
				print (radec[0]-ds9radec[0])*3600.0,(radec[1]-ds9radec[1])*3600.0


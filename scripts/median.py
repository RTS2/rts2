#!/usr/bin/env python
#
# Calculates median from files suplied as arguments. Median
# is saved to file specified as first argument.
#
# You will need numpy and pyfits packages. Those are available from python-numpy
# and python-pyfits Debian/Ubuntu packages.
#
# Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>

import sys
import numpy
import pyfits
import os

def median(of,files):
	"""Computes normalized median of files."""
	f = pyfits.open(files[0])
	d = numpy.empty([len(files),len(f[0].data),len(f[0].data[0])])
	avrg = numpy.mean(f[0].data)
	d[0] = f[0].data / avrg
	for x in range(1,len(files)):
	  	f = pyfits.open(files[x])
		avrg = numpy.mean(f[0].data)
		d[x] = f[0].data / avrg
	if (os.path.exists(of)):
	  	print "removing " + of
		os.unlink(of)
	f = pyfits.open(of,mode='append')
	m = numpy.median(d,axis=0)
	i = pyfits.PrimaryHDU(data=m)
	f.append(i)
	print 'writing %s of mean: %f std: %f median: %f' % (of,numpy.mean(m), numpy.std(m), numpy.median(numpy.median(m)))
	f.close()

def filtersort(of,files):
	# sort by filters..
	flats = {}
	for x in files:
	  	f = pyfits.open(x)
		filt = f[0].header['FILTER']
		try:
			flats[filt].append(x)
		except KeyError:
			flats[filt] = [x]
		print '\rflats ',
		for y in flats.keys():
		  	print '%s : %03d' % (y,len(flats[y])),

	for x in flats.keys():
		print ''
		print 'processing filter %s with %d images' % (x,len(flats[x]))
		median(of + x + '.fits', flats[x])


filtersort(sys.argv[1],sys.argv[2:])

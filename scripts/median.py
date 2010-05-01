#!/usr/bin/python
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
	f = pyfits.fitsopen(files[0])
	d = numpy.empty([len(files),len(f[0].data),len(f[0].data[0])])
	d[0] = f[0].data
	for x in range(1,len(files)):
	  	f = pyfits.fitsopen(files[x])
		d[x] = f[0].data
	if (os.path.exists(of)):
	  	print "removing " + of
		os.unlink(of)
	f = pyfits.open(of,mode='append')
	m = numpy.median(d,axis=0)
	max = numpy.max(d)
	print 'maximal value: %f' % (max)
	# normalize
	m = m / max
	i = pyfits.PrimaryHDU(data=m)
	f.append(i)
	print 'writing %s of mean: %f std: %f median: %f' % (of,numpy.mean(m), numpy.std(m), numpy.median(numpy.median(m)))
	f.close()

def filtersort(of,files):
	# sort by filters..
	flats = {}
	for x in files:
	  	f = pyfits.fitsopen(x)
		filt = f[0].header['FILTER']
		try:
			flats[filt].append(x)
		except KeyError:
			flats[filt] = [x]
		print '\rflats ',
		for y in flats.keys():
		  	print y, ': ', len(flats[y]),

	for x in flats.keys():
		print ''
		print 'processing filter %s with %d images' % (x,len(flats[x]))
		median(of + x + '.fits', flats[x])

filtersort(sys.argv[1],sys.argv[2:])

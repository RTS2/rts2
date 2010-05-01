#!/usr/bin/python
#
# Calculates median from files suplied as arguments. Median
# is saved to file specified as first argument.

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
	i = pyfits.PrimaryHDU(data=m)
	f.append(i)
	f.close()

median(sys.argv[1],sys.argv[2:])

#!/usr/bin/python

# Process images, produce master flat.
#
# Work in following steps:
#
# (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

from datetime import datetime
import numpy
import os
import pyfits
import string

from optparse import OptionParser

def createMasterFits(of,files,debug=False,dpoint=None):
	"""Process acquired flat images."""

	f = pyfits.fitsopen(files[0])
	d = numpy.empty([len(files),len(f[0].data),len(f[0].data[0])])
	if dpoint:
		print "input ",f[0].data[dpoint[0]][dpoint[1]],numpy.median(f[0].data),
	# normalize
	d[0] = f[0].data / numpy.median(f[0].data)
	for x in range(1,len(files)):
	  	f = pyfits.fitsopen(files[x])
		if dpoint:
			print f[0].data[dpoint[0]][dpoint[1]],numpy.median(f[0].data),
		d[x] = f[0].data / numpy.median(f[0].data)

	if dpoint:
		print

	if (os.path.exists(of)):
  		print "removing {0}".format(of)
		os.unlink(of)
	if debug:
		for x in range(0,len(files)):
			df = pyfits.open('{0}.median'.format(files[x]),mode='append')
			df.append(pyfits.PrimaryHDU(data=d[x]))
			df.close()
	if dpoint:
		print 'medians ',
		for x in range(0,len(files)):
			print d[x][dpoint[0]][dpoint[1]],
		print
	f = pyfits.open(of,mode='append')
	m = numpy.median(d,axis=0)
	if dpoint:
		print 'master',m[dpoint[0]][dpoint[1]]
	# normalize
	m = m / numpy.max(m)
	if dpoint:
		print 'normalized master',m[dpoint[0]][dpoint[1]]
	i = pyfits.PrimaryHDU(data=m)
	f.append(i)
	f.close()
	print 'I','writing %s of min: %f max: %f mean: %f std: %f median: %f' % (of,numpy.min(m),numpy.max(m),numpy.mean(m),numpy.std(m),numpy.median(numpy.median(m)))

if __name__ == "__main__":
	parser = OptionParser()

	parser.add_option('-o',help='name of output file',action='store',dest='outf',default='master.fits')
	parser.add_option('--debug',help='write normalized files to disk',action='store_true',dest='debug',default=False)
	parser.add_option('--dpoint',help='print values of medianed files and master at given point (x:y)',action='store',dest='dpoint')

	(options,args) = parser.parse_args()

	dpoint = None

	if options.dpoint:
		try:
			dpoint = string.split(options.dpoint,':')
			dpoint = map(lambda x:int(x),dpoint)
		except Exception,ex:
			print ex
			import sys
			sys.exit(-1)

	createMasterFits(options.outf,args,options.debug,dpoint)

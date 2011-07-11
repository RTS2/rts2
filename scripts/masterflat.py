#!/usr/bin/python

# Process images, produce master flat.
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

class Channel:
	def __init__(self,name,data,headers):
		self.name = name
		self.data = data
		self.headers = headers

class Channels:
	def __init__(self,headers=[],verbose=0):
		"""Headers - list of headers name which will be copied to any produced file."""
		self.channels = []
		self.headers = headers
		self.verbose = verbose

	def findChannel(self,name):
		"""Find channel with given name."""
		for c in self.channels:
			if c.name == name:
				return c
		raise KeyError('Cannot find key {0}'.format(name))

	def names(self):
		return map(lambda x:x.name,self.channels)

	def addFile(self,fn,check_channles=True):
		f = pyfits.open(fn)
		if len(self.channels):
			if self.verbose:
				print 'reading channels ',
			ok = self.names()
			for x in self.names():
				try:
					self.findChannel(x).data.append(f[x].data)
					ok.remove(x)
					if self.verbose:
						print x,
				except KeyError,ke:
					print >> sys.stderr, 'cannot find in file {0} extension with name {1}'.format(fn,x)
					if check_channles:
						raise ke
			if len(ok):
				raise Exception('file {0} miss channels {1}'.format(fn,' '.join(ok)))
		else:
			if self.verbose:
				print 'reading channels',
			for i in range(0,len(f)):
				d = f[i]
				if d.data is not None:
					if self.verbose:
						print d.header['EXTNAME'],
					cp = {}
					for h in self.headers:
						cp[h] = d.header[h]
					print d.data,cp
					self.channels.append(Channel(d.header['EXTNAME'],[d.data],cp))
		if self.verbose:
			print

	def median(self,axis=0):
		if self.verbose:
			print 'producing channel median'
		for x in self.channels:
			if self.verbose:
				print '\t',x.name,
			x.data = numpy.median(x.data,axis=axis)
			if self.verbose:
				print x.data[:10]

	def mean(self,axis=0):
		if self.verbose:
			print 'producing channel mean'
		for x in self.channels:
			if self.verbose:
				print '\t',x.name
			x.data = numpy.mean(x.data,axis=axis)
			if self.verbose:
				print x.data[:10]

	def writeto(self,fn):
		"""Writes data as a single FITS file"""
		f = None
		try:
			os.unlink(fn)
		except OSError,ose:
			pass

		f = pyfits.open(fn,mode='append')

		f.append(pyfits.PrimaryHDU())
	
		for i in c.channels:
			h = pyfits.ImageHDU(data=i.data)
			h.header.update('EXTNAME',i.name)
			ak = i.headers.keys()
			ak.sort()
			for k in ak:
				h.header.update(k,i.headers[k])
			h.header.update('OVRSCAN1',50)
			h.header.update('OVRSCAN2',50)
			h.header.update('PRESCAN1',50)
			h.header.update('PRESCAN2',50)
			f.append(h)

		f.close()

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
		dpoint = string.split(options.dpoint,':')
		dpoint = map(lambda x:int(x),dpoint)
	

	c = Channels(headers=['DETSIZE','CCDSEC','AMPSEC','DATASEC','DETSEC','NAMPS','LTM1_1','LTM2_2','LTV1','LTV2','ATM1_1','ATM2_2','ATV1','ATV2','DTM1_1','DTM2_2','DTV1','DTV2'],verbose=1)
	for a in args:
		c.addFile(a)

	c.median()

	c.writeto(options.outf)

	#createMasterFits(options.outf,args,options.debug,dpoint)

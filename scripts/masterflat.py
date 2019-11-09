#!/usr/bin/env python

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
import math
import os
import pyfits
import string
import sys
import multiprocessing

from optparse import OptionParser

class Channel:
	def __init__(self,name,data,headers):
		self.name = name
		self.data = data
		self.headers = headers

class Channels:
	def __init__(self,headers=[],verbose=0,dpoint=None):
		"""Headers - list of headers name which will be copied to any produced file."""
		self.channels = []
		self.headers = headers
		self.verbose = verbose
		self.dpoint = dpoint

	def findChannel(self,name):
		"""Find channel with given name."""
		for c in self.channels:
			if c.name == name:
				return c
		raise KeyError('Cannot find key {0}'.format(name))

	def names(self):
		return map(lambda x:x.name,self.channels)

	def addFile(self,fn,check_channels=True):
		"""Adds file to set of files for processing.
		fn                 filename of file to process
		check_channels     if True, will check to see that all channels were present in file. That's most probably what you want."""
		f = pyfits.open(fn)
		if len(self.channels):
			if self.verbose:
				print fn,'reading channels',
			ok = self.names()
			for x in self.names():
				try:
					self.findChannel(x).data.append(f[x].data)
					ok.remove(x)
					if self.verbose:
						print x,
					if self.dpoint:
						print f[x].data[self.dpoint[0]][self.dpoint[1]],
				except KeyError,ke:
					print >> sys.stderr, 'cannot find in file {0} extension with name {1}'.format(fn,x)
					if check_channels:
						raise ke
			if len(ok):
				raise Exception('file {0} miss channels {1}'.format(fn,' '.join(ok)))
		else:
			if self.verbose:
				print fn,'reading channels',
			for i in range(0,len(f)):
				d = f[i]
				if d.data is not None and len(d.data.shape) == 2:
					if self.verbose:
						print d.name,
					if self.dpoint:
						print d.data[self.dpoint[0]][self.dpoint[1]],
					cp = {}
					for h in self.headers:
						cp[h] = d.header[h]
					self.channels.append(Channel(d.name,[d.data],cp))
		if self.verbose:
			print

	def plot_histogram(self,channel):
		"""plot histograms for given channel"""
		import pylab
		ch = self.findChannel(channel)
		sp = int(math.ceil(math.sqrt(len(ch.data))))
		pylab.ion()
		for x in range(0,len(ch.data)):
			pylab.subplot(sp,sp,x + 1)
			pylab.hist(ch.data[x].flatten(),100)
			pylab.draw()

		pylab.show()

	def plot_result(self):
		"""plot result histogram"""
		import pylab
		sp = int(math.ceil(math.sqrt(len(self.channels))))
		pylab.ion()
		for x in range(0,len(self.channels)):
			pylab.subplot(sp,sp,x+1)
			pylab.hist(self.channels[x].data.flatten(),100)
			pylab.draw()

		pylab.show()

	def median(self,axis=0):
		if self.verbose:
			print 'producing channel median'
		for x in self.channels:
			if self.verbose or self.dpoint:
				print '\t',x.name,
				if self.dpoint:
					print ' '.join(map(lambda d:str(d[self.dpoint[0]][self.dpoint[1]]),x.data)),
			x.data = numpy.median(x.data,axis=axis)
			if self.verbose:
				print x.data[:10]
			if self.dpoint:
				print 'result',x.data[self.dpoint[0]][self.dpoint[1]]

	def mean(self,axis=0):
		if self.verbose:
			print 'producing channel mean'
		for x in self.channels:
			if self.verbose or self.dpoint:
				print '\t',x.name
				if self.dpoint:
					print ' '.join(map(lambda d:str(d[self.dpoint[0]][self.dpoint[1]]),x.data)),
			x.data = numpy.mean(x.data,axis=axis)
			if self.verbose:
				print x.data[:10]
			if self.dpoint:
				print 'result',x.data[self.dpoint[0]][self.dpoint[1]]

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
			f.append(h)

		f.close()

if __name__ == "__main__":
	parser = OptionParser()

	parser.add_option('-o',help='name of output file',action='store',dest='outf',default='master.fits')
	parser.add_option('--debug',help='write normalized files to disk',action='store_true',dest='debug',default=False)
	parser.add_option('--dpoint',help='print values of medianed files and master at given point (x:y)',action='store',dest='dpoint')
	parser.add_option('--histogram',help='plot histograms',action='append',dest='histogram',default=[])

	(options,args) = parser.parse_args()

	dpoint = None

	if options.dpoint:
		dpoint = string.split(options.dpoint,':')
		dpoint = map(lambda x:int(x),dpoint)
	#c = Channels(verbose=1)

	c = Channels(headers=['DETSIZE','CCDSEC','AMPSEC','DATASEC','DETSEC','NAMPS','LTM1_1','LTM2_2','LTV1','LTV2','ATM1_1','ATM2_2','ATV1','ATV2','DTM1_1','DTM2_2','DTV1','DTV2'],verbose=1,dpoint=dpoint)
	for a in args:
		c.addFile(a)

	for h in options.histogram:
		print 'plotting histogram for channel',h
		p = multiprocessing.Process(target=c.plot_histogram,args=(h,))
		p.start()

	c.median()

	if options.histogram:
		p = multiprocessing.Process(target=c.plot_result)
		p.start()

	c.writeto(options.outf)

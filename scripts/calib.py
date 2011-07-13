#!/usr/bin/python

# Process images routines. Works with multiple channel images.
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
import re

class Channel:
	def __init__(self,name,data,headers):
		self.name = name
		self.data = data
		self.headers = headers

class Channels:
	def __init__(self,headers=[],verbose=0,dpoint=None,keep_ch_headers=False,keep_phu=False):
		"""Headers - list of headers name which will be copied to any produced file."""
		self.channels = []
		self.filenames = []
		self.headers = headers
		self.verbose = verbose
		self.dpoint = dpoint
		self.keep_ch_headers = keep_ch_headers
		self.keep_phu = keep_phu
		self.phu = {}

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
					if self.verbose or self.dpoint:
						print x,
						if self.dpoint:
							print f[x].data[self.dpoint[0]][self.dpoint[1]],
						sys.stdout.flush()
				except KeyError,ke:
					print >> sys.stderr, 'cannot find in file {0} extension with name {1}'.format(fn,x)
					if check_channels:
						raise ke
			if len(ok):
				raise Exception('file {0} miss channels {1}'.format(fn,' '.join(ok)))
		else:
			if self.verbose:
				print fn,'reading channels',
			if self.keep_phu:
				self.phu = f[0].header
			for i in range(0,len(f)):
				d = f[i]
				if d.data is not None and len(d.data.shape) == 2:
					if self.verbose or self.dpoint:
						print d.name,
						if self.dpoint:
							print d.data[self.dpoint[0]][self.dpoint[1]],
						sys.stdout.flush()
					cp = {}
					if self.keep_ch_headers:
						cp = d.header
					else:
						for h in self.headers:
							cp[h] = d.header[h]
					self.channels.append(Channel(d.name,[d.data],cp))
		if self.verbose:
			print
		self.filenames.append(fn)
		f.close()

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

	def plot_square(self,size):
		import pylab
		# number of squares
		w = int(math.ceil(self.channels[0].data.shape[0] / size))
		h = int(math.ceil(self.channels[0].data.shape[1] / size))
		num = 1
		pylab.ion()
		for x in range(0,w+1):
			for y in range(0,h+1):
				d = numpy.empty(len(self.channels) * size ** 2)
				for c in self.channels:
					d = numpy.append(d,c.data[x*size:(x+1)*size,y*size:(y+1)*size])
				pylab.subplot(w + 1,h + 1,num)
				pylab.hist(d.flatten(),100)
				pylab.draw()
				num += 1
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

	def op(self,fn,op):
		"""Performs operaion. +, -, * and / are supported for operations."""
		f = pyfits.open(fn)
		if self.verbose:
			if op == '+':
				print 'adding {0}'.format(fn),
			if op == '-':
				print 'sutracting {0}'.format(fn),
			elif op == '*':
				print 'multipling by {0}'.format(fn),
			elif op == '/':
				print 'dividing by {0}'.format(fn),
		# work by channels..
		for c in self.channels:
			if self.verbose or self.dpoint:
				print c.name,
				sys.stdout.flush()
			ar = f[c.name]
			for di in range(0,len(c.data)):
				d = c.data[di]
				if not(ar.data.shape == d.shape):
					raise Exception('invalid data shape for operation {0}: {1} {0} {2}'.format(op,d.shape,ar.data.shape))
				if self.dpoint:
					print d[self.dpoint[0]][self.dpoint[1]],
				if op == '+':
					c.data[di] = d + ar.data
				elif op == '-':
					c.data[di] = d - ar.data
				elif op == '*':
					c.data[di] = d * ar.data
				elif op == '/':
					c.data[di] = d / ar.data
				else:
					raise Exception('unknow operation {0}'.format(op))

			if self.verbose or self.dpoint:
				if self.dpoint:
					print 'results',' '.join(map(lambda d:str(d[self.dpoint[0]][self.dpoint[1]]),c.data))
		if self.verbose:
			print

	def add(self,fn):
		return self.op(fn,'+')

	def subtract(self,fn):
		return self.op(fn,'-')

	def multiply(self,fn):
		return self.op(fn,'*')

	def divide(self,fn):
		return self.op(fn,'/')

	def writeto(self,fn):
		"""Writes data as a single FITS file"""
		f = None
		try:
			os.unlink(fn)
		except OSError,ose:
			pass

		f = pyfits.open(fn,mode='append')

		ph = pyfits.PrimaryHDU()
		for k in self.phu:
			ph.header.update(k,self.phu[k])

		f.append(ph)
	
		for i in self.channels:
			h = pyfits.ImageHDU(data=i.data)
			h.header.update('EXTNAME',i.name)
			ak = i.headers.keys()
			ak.sort()
			for k in ak:
				h.header.update(k,i.headers[k])
			f.append(h)

		f.close()

	def write_files(self,pat=None):
		"""Write channels to files. Use pattern to rename file."""
		rex = None
		if pat:
			rex = re.compile(r'(%[f])')
			rex_a = re.compile(r'(@(?:{[^- @}.]+}|[^- @.{}]+))')

		for i in range(0,len(self.filenames)):
			fn = self.filenames[i]
			if self.verbose:
				print fn,
			if rex:
				class match_o:
					def __init__(self,fn):
						self.fn = fn
						self.fp = None
					def p_match(self,m):
						if m.group(0)[1] == 'f':
							return os.path.splitext(os.path.basename(self.fn))[0]
					def a_match(self,m):
						if not(self.fp):
							self.fp = pyfits.open(self.fn)
						return self.fp.header[m.group(0)[1:]]

				ma = match_o(fn)

				fn = rex.sub(ma.p_match,pat)
				fn = rex_a.sub(ma.a_match,fn)
 
			if self.verbose:
				print '->',fn,
				sys.stdout.flush()

			try:
				os.unlink(fn)
			except OSError,ose:
				pass
			f = pyfits.open(fn,'append')

			ph = pyfits.PrimaryHDU()
			for k in self.phu:
				ph.header.update(k,self.phu[k])

			f.append(ph)

			for c in self.channels:
				h = pyfits.ImageHDU(data=c.data[i])
				h.header.update('EXTNAME',c.name)
				ak = c.headers.keys()
				ak.sort()
				for k in ak:
					h.header.update(k,c.headers[k])
				f.append(h)
			f.close()
			if self.verbose:
				print

if __name__ == "__main__":
	from optparse import OptionParser
	import multiprocessing

	parser = OptionParser()

	parser.add_option('-o',help='name of output file',action='store',dest='outf',default='master.fits')
	parser.add_option('--debug',help='write normalized files to disk',action='store_true',dest='debug',default=False)
	parser.add_option('--dpoint',help='print values of medianed files and master at given point (x:y)',action='store',dest='dpoint')
	parser.add_option('--histogram',help='plot histograms',action='append',dest='histogram',default=[])
	parser.add_option('--result-histogram',help='plot histogram of result (by channels)',action='store_true',dest='result_histogram',default=False)
	parser.add_option('--square',help='plot histogram of channel squares',action='store',dest='square',default=None)
	parser.add_option('--median',help='produce median of images - usefull for master dark procession',action='store_true',dest='median',default=False)
	parser.add_option('--mean',help='produce mean (average) of images',action='store_true',dest='mean',default=False)
	# file operations, producing set of files
	parser.add_option('--add',help='add to all images (arguments) image provided as option',action='store',dest='add')
	parser.add_option('--subtract',help='subtract from all images (arguments) image provided as option',action='store',dest='subtract')
	parser.add_option('--multiply',help='multiply all images (arguments) by image provided as option',action='store',dest='multiple')
	parser.add_option('--divide',help='divide all images (arguments) by image provided as option',action='store',dest='divide')

	(options,args) = parser.parse_args()

	if len(args) == 0:
		print >> sys.stderr, 'you must provide at least single file as an argument'
		sys.exit(1)
	
	# check how many operations user provide
	op = 0
	def plus_1(x):
		if x:
			global op
			op += 1

	map(plus_1,[options.median,options.mean,options.add,options.subtract,options.multiple,options.divide])

	if op > 1:
		print >> sys.stderr, 'you cannot do median and/or mean and/or subtract in one call'
		sys.exit(1)

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

	if options.add or options.subtract or options.multiple or options.divide:
		if options.add:
			c.add(options.add)
		elif options.subtract:
			c.subtract(options.subtract)
		elif options.multiple:
			c.multiple(options.multiple)
		else:
			c.divide(options.divide)
		
		c.write_files('%f-res.fits')
	else:
		if options.median:
			c.median()
		elif options.mean:
			c.mean()

		if options.result_histogram:
			p = multiprocessing.Process(target=c.plot_result)
			p.start()

		if options.square:
			p = multiprocessing.Process(target=c.plot_square,args=(int(options.square),))
			p.start()

		c.writeto(options.outf)

#!/usr/bin/env python
#
# Copyright 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
#
# Transfor amplifier coordinates into detector coordinates.
#

import pyfits
import sys
import numpy

class Channel:
	def __init__(self,he):
		self.atv=[self.__header(he,'DTV1',0),self.__header(he,'DTV2',0)]
		self.atm=[[self.__header(he,'DTM1_1',1),self.__header(he,'DTM1_2',0)],
			[self.__header(he,'DTM2_1',0),self.__header(he,'DTM2_2',1)]]

	def __header(self,h,key,default):
		"""Return default header"""
		try:
			return h[key]
		except KeyError,ke:
			return default

class ChannelsDTV:
	def __init__(self,pyfile):
		self.chan={}
		for h in range(0,len(pyfile)):
			# get axis
			try:
				self.chan[pyfile[h].header['EXTNAME']] = Channel(pyfile[h].header)
			except KeyError,ke:
				if h > 0 or len(pyfile) == 1:
					self.chan[h] = Channel(pyfile[h].header)

	def xy2Detector(self,extname,x,y):
		ch = self.chan[extname]
		(x,y) = numpy.dot([x,y],ch.atm)
		return (x+ch.atv[0],y+ch.atv[1])

if __name__ == '__main__':
	for f in sys.argv[1:]:
		ch=ChannelsDTV(pyfits.open(f))
		print ch.xy2Detector('IM1',0,200)
		print ch.xy2Detector('IM2',10,200)
		print ch.xy2Detector('IM3',20,200)
		print ch.xy2Detector('IM4',30,200)

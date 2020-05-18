#!/usr/bin/env python

# Script for autoconfiguring Andor exposure times
# (C) 2010,2011 Martin Jelinnek
# (C) 2011      Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
#
# rts2.scriptcomm.py is included in RTS2 distribution. You must eithert copy it to the
# same location as this script, or include it in PYTHONPATH.
#
# If you would like to process images, you need to have numpy and pyfits
# installaed. Please see median.py in RTS2/scripts for more details.
#
# As with all scripts intended to be called by RTS2 exe script command, you can
# test this script by calling it and verifiing that it prints something what
# does not look like error message on standard output.

import sys
import string
import time
import rts2.scriptcomm

class FlatScript (rts2.scriptcomm.Rts2Comm):
	"""Class for taking and processing skyflats."""
	def __init__(self):
		rts2.scriptcomm.Rts2Comm.__init__(self)
		self.OptimalLevel = 3500
		self.BiasLevel = 400

		self.optimalRange = 0.2

		self.expTimes = range(1,30) # allowed exposure times
		self.expTimes = map(lambda x: x*2.0, self.expTimes)

		self.startExpTime = self.expTimes[0] # starting exposure time. Must be within expTimes[0] .. expTimes[-1]

	def optimalExpTime(self,ratio):
		exptime = self.exptime / ratio # We adjust the exposure time for the next exposure, so that it is close to the optimal value
		if (exptime < self.expTimes[0]):
			return self.startExpTime
		if (exptime > self.expTimes[-1]):
	  		return self.expTimes[-1]

		lastE = self.expTimes[0]
		for e in self.expTimes:
			if (exptime < e):
			  	if (exptime - lastE) > ((e - lastE) / 2.0):
					return e
				return lastE
			lastE = e

		return self.expTimes[-1]
	
	def beforeReadout(self):
		pass

	def execute(self):
		"""Acquires images for flats. Return 0 if image was added to flats, 1 if it was too brigth, -1 if it was too dark."""
		self.setValue('exposure',self.exptime)
		img = self.exposure(self.beforeReadout)
		avrg = self.getValueFloat('max')
		ratio = (avrg - self.BiasLevel) / self.OptimalLevel
		ret = None

		if (ratio <= 0): # special case, ratio is bellow bias
			self.log('W','average is bellow bias: average %f bias %f. Please adjust BiasLevel in flats script.' % (avrg, self.BiasLevel))
			self.exptime = self.expTimes[0]
			ret = -1
		elif (abs(1.0 - ratio) <= self.optimalRange): # Images within optimalRange of the optimal flux value
			ret = 0
		else:
			if (ratio > 1.0):
				ret = 1
			else:
				ret = -1

			self.exptime = self.optimalExpTime(ratio)

		# from ret to brightness
		brightness = 'OK'
		if (ret < 0):
			brightness = 'dim'
		elif (ret > 0):
			brightness = 'bright'

		self.log('I',"run ratio %f avrg %f next exptime %f ret %s" % (ratio,avrg,self.exptime,brightness))
		self.process(img)

	def configure(self):
		self.exptime = self.startExpTime
		self.setValue('filter','r')
		self.setValue('binning','0')

	def run(self):
		# make sure we are taking light images..
		self.setValue('SHUTTER','LIGHT')
		self.setValue('EMON','true')
		self.setValue('EMON','false')

		self.target = self.getValueInteger('current','EXEC')
		next_t = self.target

		self.configure()

		while (self.target == next_t):
			self.execute()
			next_t = self.getValueInteger('next','EXEC')

		self.log('I','script finished, waiting for change to next target')

a = FlatScript()
a.run()

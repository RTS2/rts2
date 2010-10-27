#!/usr/bin/python

# RTS2 script communication
# (C) 2009,2010 Petr Kubanek

import sys
import time

# constants for device types
DEVICE_TELESCOPE   = "TELESCOPE"
DEVICE_CCD         = "CCD"
DEVICE_DOME        = "DOME"
DEVICE_WEATHER     = "WEATHER"
DEVICE_PHOT        = "PHOT"
DEVICE_PLAN        = "PLAN"
DEVICE_FOCUS       = "FOCUS"
DEVICE_CUPOLA      = "CUPOLA"
DEVICE_FW          = "FW"
DEVICE_SENSOR      = "SENSOR"

class Rts2Comm:
	"""Class for communicating with RTS2 in exe command."""
	def __init__(self):
		return

	def getValue(self,value,device = None):
		"""Returns given value."""
		if (not (device is None)):
			print 'G',device,value
		else:
			print '?',value
		sys.stdout.flush()
		return sys.stdin.readline()

	def getValueFloat(self,value,device = None):
		"""Return value as float number."""
		return float(self.getValue(value,device))

	def getValueInteger(self,value,device = None):
		return int(self.getValue(value,device))

	def incrementValue(self,name,new_value,device = None):
		if (device is None):
			print "value",name,'+=',new_value
		else:
			print "V",device,name,'+=',new_value
		sys.stdout.flush()
	
	def incrementValueType(self,device,new_value):
		print "VT",device,name,'+=',new_value
		sys.stdout.flush()

	def setValue(self,name,new_value,device = None):
		if (device is None):
			print "value",name,'=',new_value
		else:
			print "V",device,name,'=',new_value
		sys.stdout.flush()

	def setValueByType(self,device,name,new_value):
		"""Set value for all devices of given type. Please use DEVICE_xx constants to specify device type."""
		print "VT",name,'-',new_value
		sys.stdout.flush()
	
	def exposure(self, before_readout_callback = None):
		print "exposure"
		sys.stdout.flush()
		a = sys.stdin.readline()
		if (a != "exposure_end\n"):
			self.log('E', "invalid return from exposure - expected exposure_end, received " + a)
		if (not (before_readout_callback is None)):
			before_readout_callback()
		a = sys.stdin.readline()
		image,fn = a.split()
		return fn

	def __imageAction(self,action,imagename):
		print action,imagename
		sys.stdout.flush()
		return sys.stdin.readline()


	def rename(self,imagename,pattern):
		print "rename",imagename,pattern
		sys.stdout.flush()
		return sys.stdin.readline()

	def move(self,imagename,pattern):
		"""Move image to new path, delete it from the database."""
		print "move",imagename, pattern
		sys.stdout.flush()
		return sys.stdin.readline()
	
	def toFlat(self,imagename):
		return self.__imageAction("flat",imagename)

	def toDark(self,imagename):
		return self.__imageAction("dark",imagename)

	def toArchive(self,imagename):
		"""Moves image at path to archive. Return new image path."""
		return self.__imageAction("archive",imagename)

	def toTrash(self,imagename):
		"""Move image at path to trash. Return new image path."""
		return self.__imageAction("trash",imagename)
	
	def delete(self,imagename):
		print "delete",imagename
		sys.stdout.flush()
	
	def log(self,level,text):
		print "log",level,text
		sys.stdout.flush()
	
	def isEvening(self):
		"""Returns true if is evening - sun is on west"""
		sun_az = self.getValueFloat('sun_az','centrald')
		return sun_az < 180.0

#!/usr/bin/python
#
# RTS2 script communication
# (C) 2009,2010 Petr Kubanek <petr@kubanek.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

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
	
	def sendCommand(self,command,device = None):
		"""Send command to device."""
		if device is None:
		  	print 'command',command
		else:
		  	print 'C',device,command
		sys.stdout.flush()

	def getValue(self,value,device = None):
		"""Returns given value."""
		if device is None:
			print '?',value
		else:
			print 'G',device,value
		sys.stdout.flush()
		return sys.stdin.readline().rstrip('\n')

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
	
	def incrementValueType(self,device,name,new_value):
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

	def radec(self,ra,dec):
		print "radec",ra,dec
		sys.stdout.flush()
	
	def newObs(self,ra,dec):
		print "newobs",ra,dec
		sys.stdout.flush()

	def altaz(self,alt,az):
		print "altaz",alt,az
		sys.stdout.flush()

	def newObsAltAz(self,alt,az):
		print "newaltaz",alt,az
		sys.stdout.flush()

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
		"""Move image at path to archive. Return new image path."""
		return self.__imageAction("archive",imagename)

	def toTrash(self,imagename):
		"""Move image at path to trash. Return new image path."""
		return self.__imageAction("trash",imagename)
	
	def delete(self,imagename):
		"""Delete image from disk."""
		print "delete",imagename
		sys.stdout.flush()
	
	def process(self,imagename):
		"""Put image to image processor queue."""
		print "process",imagename
		sys.stdout.flush()

	def doubleValue(self,name,desc,value):
		"""Add to device double value."""
		print "double",name,'"{0}"'.format(desc),value
		sys.stdout.flush()

	def integerValue(self,name,desc,value):
		"""Add to device integer value."""
		print "integer",name,'"{0}"'.format(desc),value
		sys.stdout.flush()
	
	def stringValue(self,name,desc,value):
		"""Add to device string value."""
		print "string",name,'"{0}"'.format(desc),value
		sys.stdout.flush()
	
	def log(self,level,text):
		print "log",level,text
		sys.stdout.flush()
	
	def isEvening(self):
		"""Returns true if is evening - sun is on West"""
		sun_az = self.getValueFloat('sun_az','centrald')
		return sun_az < 180.0

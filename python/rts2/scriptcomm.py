#!/usr/bin/python
#
# RTS2 script communication
# (C) 2009,2010 Petr Kubanek <petr@kubanek.net>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

from __future__ import print_function

import sys
import time
import re

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
DEVICE_EXECUTOR    = "EXEC"
DEVICE_SELECTOR    = "SELECTOR"
DEVICE_IMGP        = "IMGP"

# constants for device states
CAM_EXPOSING       = 0x001
TEL_TRACKING       = 0x020
FOC_FOCUSING       = 0x001
DOME_OPENING       = 0x002
DOME_CLOSING       = 0x008

# constants for dataypes
DT_RA              = "DT_RA"
DT_DEC             = "DT_DEC"
DT_DEGREES         = "DT_DEGREES"
DT_DEG_DIST        = "DT_DEG_DIST"
DT_PERCENTS        = "DT_PERCENTS"
DT_ROTANG          = "DT_ROTANG"
DT_HEX             = "DT_HEX"
DT_BYTESIZE        = "DT_BYTESIZE"
DT_KMG             = "DT_KMG"
DT_INTERVAL        = "DT_INTERVAL"
DT_ONOFF           = "DT_ONOFF"

class Rts2Exception(Exception):
	"""Thrown on exceptions on communicated over stdin/stdout connection."""
	def __init__(self,message):
		Exception.__init__(self,message)

class Rts2NotActive(Exception):
	def __init__(self):
		Exception.__init__(self,'script is not active')

class Rts2Comm:
	"""Class for communicating with RTS2 in exe command."""
	def __init__(self, log_device = True):
		self.exception_re = re.compile('([!&]) (\S.*)')
		self.__run_device = None
		self.__log_device = log_device

	def sendCommand(self,command,device = None):
		"""Send command to device."""
		if device is None:
		  	print('command {0}'.format(command))
		else:
		  	print('C {0} {1}'.format(device,command))
		sys.stdout.flush()

	def readline(self):
		"""Reads single line from standard input. Checks for exceptions."""
		ex = None
		while True:
			a = sys.stdin.readline().rstrip('\n')
			# handle exceptions
			m = self.exception_re.match(a)
			if m:
				if m.group(1) == '!':
					self.log('W','exception from device: {0}'.format(m.group(2)))
					ex = Rts2Exception(m.group(2))
				elif m.group(1) == '&':
					raise Rts2NotActive()
			elif ex:
				raise ex
			else:
				return a

	def getValue(self,value,device = None):
		"""Returns given value."""
		if device is None:
			print('? {0}'.format(value))
		else:
			print('G {0} {1}'.format(device,value))
		sys.stdout.flush()
		return self.readline()

	def getOwnValue(self, value):
		print('get_own {0}'.format(value))
		sys.stdout.flush()
		return self.readline()

	def getLoopCount(self):
		print('loopcount')
		sys.stdout.flush()
		return int(self.readline())

	def getRunDevice(self):
		if self.__run_device is None:
			print('run_device')
			sys.stdout.flush()
			self.__run_device = self.readline()
		return self.__run_device

	def getValueFloat(self,value,device = None):
		"""Return value as float number."""
		return float(self.getValue(value,device).split(" ")[0])

	def getValueInteger(self,value,device = None):
		val = self.getValue(value,device)
		if val[0:2] == "0x":
			vals = val.split()[0:2]
			if len(vals[0]) > 2:
				return int(vals[0], base=16)
			else:
				return int(vals[1], base=16)
		else:
			return int(val.split(" ")[0])

	def incrementValue(self,name,new_value,device = None):
		if (device is None):
			print("value {0} += {1}".format(name,new_value))
		else:
			print("V {0} {1} += {2}".format(device,name,new_value))
		sys.stdout.flush()
	
	def incrementValueType(self,device,name,new_value):
		print("VT {0} {1} += {2}".format(device,name,'+=',new_value))
		sys.stdout.flush()

	def setValue(self, name, new_value, device = None):
		if (device is None):
			print("value {0} = {1}".format(name,new_value))
		else:
			print("V {0} {1} = {2}".format(device,name,new_value))
		sys.stdout.flush()

	def setOwnValue(self, name, new_value):
		print('set_own {0} {1}'.format(name, new_value))
		sys.stdout.flush()

	def setValueByType(self,device,name,new_value):
		"""Set value for all devices of given type. Please use DEVICE_xx constants to specify device type."""
		print("VT {0} {1} = {2}".format(device,name,new_value))
		sys.stdout.flush()

	def getDeviceByType(self,device):
		"""Returns name of the first device with given type."""
		print("device_by_type {0}".format(device))
		sys.stdout.flush()
		return self.readline()

	def targetDisable(self):
		"""Disable target for autonomous selection."""
		print("target_disable")
		sys.stdout.flush()

	def targetTempDisable(self,ti):
		"""Temporary disable target for given number of seconds."""
		print("target_tempdisable {0}".format(ti))
		sys.stdout.flush()

	def endScript(self):
		"""Ask controlling process to end the current script."""
		print("end_script")
		sys.stdout.flush()

	def getState(self,device):
		"""Retrieve device state"""
		print('S {0}'.format(device))
		sys.stdout.flush()
		return int(self.readline())
	
	def waitIdle(self,device,timeout):
		"""Wait for idle state (with timeout)"""
		print('waitidle {0} {1}'.format(device,timeout))
		sys.stdout.flush()
		return int(self.readline())

	def waitMask(self,device,mask,timeout):
		"""Wait for given state (when mask & state = mask)"""
		print('waitmask {0} {1} {2}'.format(device,mask,timeout))
		sys.stdout.flush()
		return int(self.readline())

	def exposure(self, before_readout_callback = None, fileexpand = None, overwrite = False):
		"""Start new exposure. Allow user to specify fallback function to call after end of exposure and expand pattern for new file.
		Unless fileexpand parameter is provided, user is responsible to specify image treatment."""
		if fileexpand:
			if overwrite:
				print("exposure_overwrite {0}".format(fileexpand))
			else:
				print("exposure_wfn {0}".format(fileexpand))
		else:
			print("exposure")
		sys.stdout.flush()
		a = self.readline()
		if a == 'exposure_failed':
			raise Rts2Exception("exposure failed")
		if a == 'exposure_end_noimage':
			return None
		if a != "exposure_end":
			self.log('E', "invalid return from exposure - expected exposure_end, received " + a)
		if not (before_readout_callback is None):
			before_readout_callback()
		a = self.readline()
		image,fn = a.split()
		return fn

	def endShift(self, fileexpand = None, overwrite = False):
		"""Ends shiftstore sequence.
		Unless fileexpand parameter is provided, user is responsible to specify image treatment."""
		if fileexpand:
			if overwrite:
				print("exposure_overwrite {0}".format(fileexpand))
			else:
				print("exposure_wfn {0}".format(fileexpand))
		else:
			print("endshift")
		sys.stdout.flush()
		a = self.readline()
		if a == 'exposure_failed':
			raise Rts2Exception("exposure failed")
		if a == 'exposure_end_noimage':
			return None
		if a != "exposure_end":
			self.log('E', "invalid return from exposure - expected exposure_end, received " + a)
		if not (before_readout_callback is None):
			before_readout_callback()
		a = self.readline()
		image,fn = a.split()
		return fn

	def progressUpdate(self,expected_end,start=time.time()):
		print("progress {0} {1}".format(start,expected_end))
		sys.stdout.flush()

	def radec(self,ra,dec):
		print("radec {0} {1}".format(ra,dec))
		sys.stdout.flush()
	
	def newObs(self,ra,dec):
		print("newobs {0} {1}".format(ra,dec))
		sys.stdout.flush()

	def altaz(self,alt,az):
		print("altaz {0} {1}".format(alt,az))
		sys.stdout.flush()

	def newObsAltAz(self,alt,az):
		print("newaltaz {0} {1}".format(alt,az))
		sys.stdout.flush()

	def __imageAction(self,action,imagename):
		print(action,imagename)
		sys.stdout.flush()
		return self.readline()

	def rename(self,imagename,pattern):
		print("rename",imagename,pattern)
		sys.stdout.flush()
		return self.readline()

	def move(self,imagename,pattern):
		"""Move image to new path, delete it from the database."""
		print("move",imagename, pattern)
		sys.stdout.flush()
		return self.readline()
	
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
		print("delete",imagename)
		sys.stdout.flush()
	
	def process(self,imagename):
		"""Put image to image processor queue."""
		print("process",imagename)
		sys.stdout.flush()

	def doubleValue(self,name,desc,value,rts2_type=None):
		"""Add to device double value."""
		print("double",name,'"{0}"'.format(desc),value,rts2_type if rts2_type else '')
		sys.stdout.flush()

	def doubleVariable(self,name,desc,value):
		"""Add to device double writable variable."""
		print("double_w",name,'"{0}"'.format(desc),value)
		sys.stdout.flush()

	def timeValue(self,name,desc,value,rts2_type=None):
		"""Add to device time value."""
		print("timevalue",name,'"{0}"'.format(desc),value,rts2_type if rts2_type else '')
		sys.stdout.flush()

	def timeVariable(self,name,desc,value):
		"""Add to device time writable variable."""
		print("timevalue_w",name,'"{0}"'.format(desc),value)
		sys.stdout.flush()

	def integerValue(self,name,desc,value):
		"""Add to device integer value."""
		print("integer",name,'"{0}"'.format(desc),value)
		sys.stdout.flush()

	def integerVariable(self,name,desc,value):
		"""Add to device integer writable variable."""
		print("integer_w",name,'"{0}"'.format(desc),value)
		sys.stdout.flush()
	
	def stringValue(self,name,desc,value):
		"""Add to device string value."""
		print("string",name,'"{0}"'.format(desc),value)
		sys.stdout.flush()

	def stringVariable(self,name,desc,value):
		"""Add to device string writable variable."""
		print("string_w",name,'"{0}"'.format(desc),value)
		sys.stdout.flush()

	def boolValue(self,name,desc,value):
		"""Add to device boolean value."""
		print("bool",name,'"{0}"'.format(desc),value)
		sys.stdout.flush()

	def boolVariable(self,name,desc,value):
		"""Add to device boolean writable variable."""
		print("bool_w",name,'"{0}"'.format(desc),value)
		sys.stdout.flush()

	def onoffValue(self,name,desc,value):
		"""Add to device boolean value with on/off display type."""
		print("onoff",name,'"{0}"'.format(desc),value)
		sys.stdout.flush()

	def onoffVariable(self,name,desc,value):
		"""Add to device boolean writable variable with on/off display type."""
		print("onoff_w",name,'"{0}"'.format(desc),value)
		sys.stdout.flush()

	def raDecValue(self,name,desc,ra,dec):
		"""Add RA/DEC value pair"""
		print("radec",name,'"{0}"'.format(desc),ra,dec)
		sys.stdout.flush()

	def doubleArrayValue(self,name,desc,values):
		print("double_array",name,'"{0}"'.format(desc),' '.join(map(str,values)))
		sys.stdout.flush()

	def doubleArrayVariable(self,name,desc,values):
		print("double_array_w",name,'"{0}"'.format(desc),' '.join(map(str,values)))
		sys.stdout.flush()

	def doubleArrayAdd(self,name,values):
		print("double_array_add",name,' '.join(map(str,values)))
		sys.stdout.flush()

	def statAdd(self, name, desc, num, value):
		"""Add to statistics boolean value."""
		print("stat_add",name,'"{0}"'.format(desc),num,value)
		sys.stdout.flush()

	def log(self,level, text):
		if self.__log_device:
			text = self.getRunDevice() + ' ' + text
		print("log",level,text)
		sys.stdout.flush()
	
	def isEvening(self):
		"""Returns true if is evening - sun is on West"""
		sun_az = self.getValueFloat('sun_az','centrald')
		return sun_az < 180.0

	def tempentry(self, entry):
		print('tempentry',entry)
		sys.stdout.flush()

	def endTarget(self):
		print('end_target')
		sys.stdout.flush()

	def stopTarget(self):
		print('stop_target')
		sys.stdout.flush()

	def waitTargetMove(self):
		"""Returns after move was commanded"""
		print('wait_target_move')
		sys.stdout.flush()
		return self.readline()

	def queueClear(self, queue, selector=None):
		"""Clear selector queue"""
		if selector is None:
			selector=self.getDeviceByType(DEVICE_SELECTOR)
		return self.sendCommand('clear {0}'.format(queue), selector)

	def queueAppend(self, queue, target_id, selector=None):
		if selector is None:
			selector=self.getDeviceByType(DEVICE_SELECTOR)
		return self.command('queue {0} {1}'.format(queue, target_id), selector)

	def queueInsert(self, queue, target_id, index=0, selector=None):
		if selector is None:
			selector=self.getDeviceByType(DEVICE_SELECTOR)
		return self.command('insert {0} {1} {2}'.format(queue, index, target_id), selector)

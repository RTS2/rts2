#!/usr/bin/python

# Flat taking script.
# (C) 2009,2010 Antonio de Ugarte & Petr Kubanek

import sys
import time

class Rts2Comm:
	"""Class for communicating with RTS2 in exe command."""
	def __init__(self):
		self.eveningFilters = ['Y','B','b','g','r','i','z'] # filters for evening, we will use reverse for morning

		self.morningFilters = self.eveningFilters[:] # filters for morning - reverse of evening filters - deep copy them first
		self.morningFilters.reverse ()

		self.doDarks = True

		self.filter = None

		self.SaturationLevel = 65536 # should be 16bit for the nonEM and 14bit for EM)
		self.OptimalFlat = self.SaturationLevel / 3
		self.optimalRange = 0.3 # range of allowed divertion from OptimalFlat. 0.3 means 30%
		self.allowedOptimalDeviation = 0.1 # deviation allowed from optimalRange to start real flats
		self.BiasLevel = 390 # although this could be set to 0 and should not make much difference
		self.NumberFlats = 10 # Number of flats that we want to obtain
		self.sleepTime = 1 # number of seconds to wait for optimal flat conditions
		self.startExpTime = 1 # starting exposure time

		self.shiftRa = 10.0 / 3600  # shift after every flat in RA [degrees]
		self.shiftDec = 10.0 / 3600 # shift after every flat in DEC [degrees]

		# self.waitingSubWindow = '100 100 200 200' # x y w h of subwindow while waiting for good flat level
		self.waitingSubWindow = None # do not use subwidnow to wait for flats
		self.isSubWindow = False

		self.expTimes = range(1,20) # allowed exposure times
		self.expTimes = map(lambda x: x/2.0, self.expTimes)

		self.Ngood = {}
		self.usedExpTimes = []

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

	def setValue(self,name,new_value,device = None):
		if (device is None):
			print "value",name,'=',new_value
		else:
			print "V",device,name,'=',new_value
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
	
	def toFlat(self,imagename):
		return self.__imageAction("flat",imagename)

	def toDark(self,imagename):
		return self.__imageAction("dark",imagename)

	def toArchive(self,imagename):
		return self.__imageAction("archive",imagename)

	def toTrash(self,imagename):
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

	def optimalExpTime(self,ratio):
		exptime = self.exptime / ratio # We adjust the exposure time for the next exposure, so that it is close to the optimal value
		if (exptime < self.expTimes[0]):
			if (ratio > (1.0 - self.optimalRange - self.allowedOptimalDeviation)):
				return self.startExpTime
			return self.expTimes[0]
		if (exptime > self.expTimes[-1]):
			if (ratio < (1.0 + self.optimalRange + self.allowedOptimalDeviation)):
				return self.startExpTime
		  	return self.expTimes[-1]

		lastE = self.expTimes[0]
		for e in self.expTimes:
			if (exptime < e):
				return lastE
			lastE = e

		return self.expTimes[-1]
	
	def beforeReadout(self):
		if (self.isSubWindow == True):
			return
		if (self.shiftRa != 0 or self.shiftDec != 0):
			self.incrementValue("OFFS",self.shiftRa.__str__() + ' ' + self.shiftDec.__str__(),"T0")

	def fullWindow(self):
		self.setValue('WINDOW','-1 -1 -1 -1')
		self.isSubWindow = False

	def acquireImage(self):
		"""Acquires images for flats. Return 0 if image was added to flats, 1 if it was too brigth, -1 if it was too dark."""
		self.setValue('exposure',self.exptime)
		img = self.exposure(self.beforeReadout)
		avrg = self.getValueFloat('average') # Calculate average of image (can be just the central 100x100pix if you want to speed up)
		ratio = (avrg - self.BiasLevel) / self.OptimalFlat
		ret = None
		if (abs(1.0 - ratio) <= self.optimalRange): # Images within optimalRange of the optimal flux value
			ret = 0
		  	if (self.isSubWindow):
				self.fullWindow()
				self.delete(img)
			else:
				self.toFlat(img)
				self.Ngood[self.filter] += 1
				# add used exposure time - if it does not exists
				try:
					self.usedExpTimes.index(self.exptime)
				except ValueError:
					self.usedExpTimes.append(self.exptime)
		elif (abs(1.0 - ratio) < (self.optimalRange + self.allowedOptimalDeviation)):
			if (self.isSubWindow):
				self.fullWindow()
			self.delete(img)
			# we believe that the next image will be good one..
			ret = 0
		else:
			self.delete(img) #otherwise it is not useful and we delete it
			if (ratio > 1.0):
				ret = 1
			else:
				ret = -1

		self.exptime = self.optimalExpTime(ratio)

		self.log('I',"run ratio %f avrg %f ngood %d filter %s next exptime %f ret %i" % (ratio,avrg,self.Ngood[self.filter],self.filter,self.exptime,ret))
		return ret
	
	def executeEvening(self):
		self.Ngood[self.filter] = 0 # Number of good images
		self.exptime = self.startExpTime

		if (not ((self.waitingSubWindow is None) or (self.isSubWindow))):
			self.isSubWindow = True
			self.setValue('WINDOW',self.waitingSubWindow)

		while (self.Ngood[self.filter] < self.NumberFlats): # We continue until we have enough flats
			imgstatus = self.acquireImage()
			if (imgstatus == -1):
				# too dim image..
				return
			elif (imgstatus == 1):
				time.sleep(self.sleepTime)
			# 0 mean good image, just continue..
	
	def runEvening(self):
		for self.filter in self.eveningFilters: # starting from the bluest and ending with the redest
			self.setValue('filter',self.filter)
			self.executeEvening()

	def executeMorning(self):
		self.Ngood[self.filter] = 0 # Number of good images
		self.exptime = self.startExpTime

		if (not ((self.waitingSubWindow is None) or (self.isSubWindow))):
			self.isSubWindow = True
			self.setValue('WINDOW',self.waitingSubWindow)

		while (self.Ngood[self.filter] < self.NumberFlats): # We continue until we have enough flats
			imgstatus = self.acquireImage()
			if (imgstatus == 1):
				# too bright image
				return
			elif (imgstatus == -1):
				time.sleep(self.sleepTime) # WAIT sleepTime seconds (we would wait to until the sky is a bit brighter
			# good image, just continue as usuall

	def runMorning(self):
		for self.filter in self.morningFilters: # starting from the redest and ending with the bluest
			self.setValue('filter',self.filter)
			self.executeMorning()

	def takeDarks(self):
		"""Take flats dark images in spare time."""
		self.setValue('SHUTTER','DARK')
		while (True):
			for exp in self.usedExpTimes:
				sun_alt = self.getValueFloat('sun_alt','centrald')
				next = self.getValueInteger('next','EXEC')
				if (sun_alt >= -0.5 or not (next == 2 or next == -1)):
					self.setValue('SHUTTER','LIGHT')
					return
				dark = self.exposure(exp)
				self.toDark(dark)
			return

	def run(self):
		# make sure we are taking light images..
		self.setValue('SHUTTER','LIGHT')
		# choose filter sequence..
		if (self.isEvening()):
			self.runEvening()
		else:
			self.runMorning()

		if (self.doDarks):
			self.takeDarks()

a = Rts2Comm()
a.run()

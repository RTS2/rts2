#!/usr/bin/python

# Flat taking script.
# (C) 2009 Antonio de Ugarte & Petr Kubanek

import sys
import time

class Rts2Comm:
	def __init__(self):
		self.eveningFilters = ['B','b','g','r','i','z','Y'] # filters for evening, we will use reverse for morning

		self.morningFilters = self.eveningFilters[:] # filters for morning - reverse of evening filters - deep copy them first
		self.morningFilters.reverse ()

		self.MaxExpT = 10   # Maximum exposure time that we allow
		# if self.MaxExpT is changed, adjust self.expTimes = range(1,20) below
		self.MinExpT = 0.5  # Minimum exposure time that we allow)
		self.SaturationLevel = 65536 # should be 16bit for the nonEM and 14bit for EM)
		self.OptimalFlat = self.SaturationLevel / 3
		self.optimalRange = 0.3 # range of allowed divertion from OptimalFlat. 0.3 means 30%
		self.BiasLevel = 390 # although this could be set to 0 and should not make much difference
		self.NumberFlats = 10 # Number of flats that we want to obtain
		self.sleepTime = 30 # number of seconds to wait for optimal flat conditions
		self.startExpTime = 1 # starting exposure time

		self.shiftRa = 10.0 / 3600  # shift after every flat in RA [degrees]
		self.shiftDec = 10.0 / 3600 # shift after every flat in DEC [degrees]

		# self.waitingSubWindow = '100 100 200 200' # x y w h of subwindow while waiting for good flat level
		self.waitingSubWindow = None # do not use subwidnow to wait for flats
		self.isSubWindow = False

		self.expTimes = range(1,20) # allowed exposure times
		self.expTimes = map(lambda x: x/2.0, self.expTimes)

	def getValue(self,value,device = None):
		"""Returns given value."""
		if (device != None):
			print 'G',device,value
		else:
			print '?',value
		sys.stdout.flush()
		return sys.stdin.readline()

	def getValueFloat(self,value,device = None):
		"""Return value as float number."""
		return float(self.getValue(value,device))

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
			self.log ('E', "invalid return from exposure - expected exposure_end, received " + a)
		if (not (before_readout_callback is None)):
			print "calling before readout"
			before_readout_callback()
		a = sys.stdin.readline()
		image,fn = a.split()
		return fn

	def rename(self,imagename,pattern):
		print "rename",imagename,pattern
		sys.stdin.flush()
	
	def toFlat(self,imagename):
		print "flat",imagename
		sys.stdin.flush()
	
	def toTrash(self,imagename):
		print "trash",imagename
		sys.stdin.flush()

	def delete(self,imagename):
		print "delete",imagename
		sys.stdin.flush()

	def log(self,level,text):
		print "log",level,text
		sys.stdin.flush()
	
	def isEvening(self):
		"""Returns true if is evening - sun is on west"""
		sun_az = self.getValueFloat('sun_az','centrald')
		return sun_az < 180.0

	def optimalExpTime(self,avrg):
		self.exptime = self.exptime * self.OptimalFlat / (avrg - self.BiasLevel) # We adjust the exposure time for the next exposure, so that it is close to the optimal value
		if (self.exptime < self.expTimes[0]):
			return self.MinExpT
		if (self.exptime > self.expTimes[-1]):
		  	return self.MaxExpT

		lastE = self.expTimes[0]
		for e in self.expTimes:
			if (self.exptime < e):
				return lastE
			lastE = e

		return self.expTimes[-1]
	
	def beforeReadout(self):
		if (self.isSubWindow == True):
			return
		if (self.shiftRa != 0 or self.shiftDec != 0):
			self.incrementValue("OFFS",self.shiftRa.__str__() + ' ' + self.shiftDec.__str__(),"T0")
	
	def acquireImage(self):
		self.setValue('exposure',self.exptime)
		img = self.exposure(self.beforeReadout)
		avrg = self.getValueFloat('average') # Calculate average of image (can be just the central 100x100pix if you want to speed up)
		if (abs(1.0 - (avrg - self.BiasLevel) / self.OptimalFlat) <= self.optimalRange): # Images within optimalRange of the optimal flux value
		  	if (self.isSubWindow):
				# acquire full image
				self.setValue('WINDOW','-1 -1 -1 -1')
				self.isSubWindow = False
				self.delete(img)
			else:
				self.toFlat(img)
				self.Ngood += 1
		else:
			self.delete(img) #otherwise it is not useful and we delete it
		self.exptime = self.optimalExpTime(avrg)

		self.log('I',"run avrg %f ngood %d filter %s next exptime %f" % (avrg,self.Ngood,filter,self.exptime))

	
	def executeEvening(self):
		self.Ngood = 0 # Number of good images
		if (self.exptime >= self.MaxExpT):
		  	self.exptime = self.MinExpT # Used in the case where we have changed filter due to a too dim sky

		if (not ((self.waitingSubWindow is None) or (self.isSubWindow))):
			self.isSubWindow = True
			self.setValue('WINDOW',self.waitingSubWindow)

		while (self.Ngood < self.NumberFlats and self.exptime < self.MaxExpT): # We continue when we have enough flats or when the sky is too dim
			if (self.exptime < self.MinExpT):
				self.exptime = self.MinExpT
				time.sleep(self.sleepTime) # WAIT sleepTime seconds (we would wait to until the sky is a bit dimer

			self.acquireImage()
	
	def runEvening(self):
		self.exptime = self.startExpTime

		for filter in self.eveningFilters: # starting from the bluest and ending with the redest
			self.setValue('filter',filter)
			self.executeEvening()

	def executeMorning(self):
		self.Ngood = 0 # Number of good images
		if (self.exptime <= self.MinExpT):
			self.exptime = self.startExpTime # Used in the case where we have changed filter due to a too bright sky

		if (not ((self.waitingSubWindow is None) or (self.isSubWindow))):
			self.isSubWindow = True
			self.setValue('WINDOW',self.waitingSubWindow)

		while (self.Ngood < self.NumberFlats and self.exptime > self.MinExpT): # We continue when we have enough flats or when the sky is too bright
			if (self.exptime > self.MaxExpT):
				self.exptime = self.MaxExpT
				time.sleep(self.sleepTime) # WAIT sleepTime seconds (we would wait to until the sky is a bit brighter

			self.acquireImage()

	def runMorning(self):
		self.exptime = self.startExpTime

		for filter in self.morningFilters: # starting from the redest and ending with the bluest
			self.setValue('filter',filter)
			self.executeMorning()

	def run(self):
		# choose filter sequence..
		if (self.isEvening()):
			return self.runEvening()
		return self.runMorning()

a = Rts2Comm()
a.run()

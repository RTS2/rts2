#!/usr/bin/python

# Flat taking script.
# (C) 2009 Antonio de Ugarte & Petr Kubanek

import sys
import time

class Rts2Comm:
	def __init__(self):
		self.filters = ['B','b','g','r','i','z','Y'] # filters for evening, we will use reverse for morning
		self.MaxExpT = 10   # Maximum exposure time that we allow
		self.MinExpT = 0.5  # Minimum exposure time that we allow)
		self.SaturationLevel = 65536 # should be 16bit for the nonEM and 14bit for EM)
		self.OptimalFlat = self.SaturationLevel / 2
		self.BiasLevel = 390 # although this could be set to 0 and should not make much difference
		self.NumberFlats = 10 # Number of flats that we want to obtain
		self.sleepTime = 30 # number of seconds to wait for optimal flat conditions
		self.startExpTime = 1 # starting exposure time

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

	def setValue(self,name,new_value):
		print "value",name,'=',new_value
		sys.stdout.flush()
	
	def exposure(self):
		print "exposure"
		sys.stdout.flush()
		a = sys.stdin.readline()
		image,fn = a.split()
		return fn
	
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
	
	def runEvening(self):
		self.exptime = self.startExpTime

		for filter in self.filters: # starting from the bluest and ending with the redest
			self.Ngood = 0 # Number of good images
			self.setValue('filter',filter)
			if (self.exptime > self.MaxExpT):
			  	self.exptime = self.MinExpT # Used in the case where we have changed filter due to a too dim sky

			while (self.Ngood < self.NumberFlats and self.exptime < self.MaxExpT): # We continue when we have enough flats or when the sky is too dim
				if (self.exptime < self.MinExpT):
					self.exptime = self.MinExpT
					time.sleep(self.sleepTime) # WAIT sleepTime seconds (we would wait to until the sky is a bit dimer

				self.setValue('exposure',self.exptime)
				img = self.exposure()
				avrg = self.getValueFloat('average') # Calculate average of image (can be just the central 100x100pix if you want to speed up)
				if (abs(1.0 - (avrg - self.BiasLevel) / self.OptimalFlat) <= 0.3): # Images within ~ 30% of the optimal flux value
					self.toFlat(img)
					self.Ngood += 1
				else:
					self.delete(img) #otherwise it is not useful and we delete it
				self.exptime = self.exptime * self.OptimalFlat / (avrg - self.BiasLevel) # We adjust the exposure time for the next exposure, so that it is close to the optimal value

				self.log('I',"runEvening avrg %f ngood %d filter %s next exptime %f" % (avrg,self.Ngood,filter,self.exptime))

	def runMorning(self):
		self.filters.reverse() # oposite order of filters then at evening
		self.exptime = self.startExpTime

		for filter in self.filters: # starting from the redest and ending with the bluest
			self.Ngood = 0 # Number of good images
			self.setValue('filter',filter)
			if (self.exptime < self.MinExpT):
				self.exptime = self.MinExpT # Used in the case where we have changed filter due to a too bright sky
			while (self.Ngood < self.NumberFlats and self.exptime > self.MinExpT): # We continue when we have enough flats or when the sky is too bright
				if (self.exptime > self.MaxExpT):
					self.exptime = MaxExpT
					time.sleep(self.sleepTime) # WAIT sleepTime seconds (we would wait to until the sky is a bit brighter

				self.setValue('exposure',self.exptime)
				img = self.exposure()
				avrg = self.getValueFloat('average') # Calculate average of image (can be just the central 100x100pix if you want to speed up)
				if (abs(1.0 - (avrg - self.BiasLevel) / self.OptimalFlat) <= 0.3): # Images within ~ 30% of the optimal flux value
					self.toFlat(img)
					self.Ngood += 1
				else:
					self.delete(img) #otherwise it is not useful and we delete it

				self.exptime = self.exptime * self.OptimalFlat / (avrg - self.BiasLevel) # We adjust the exposure time for the next exposure, so that it is close to the optimal value
				self.log('I',"runMorning avrg %f ngood %d filter %s next exptime %f" % (avrg,self.Ngood,filter,self.exptime))

	def run(self):
		# choose filter sequence..
		if (self.isEvening()):
			return self.runEvening()
		return self.runMorning()

a = Rts2Comm()
a.run()

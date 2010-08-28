#!/usr/bin/python

# Script for obtaining twilight skyflats.
# (C) 2009,2010 Antonio de Ugarte & Petr Kubanek
#
# rts2comm.py is included in RTS2 distribution. You must eithert copy it to the
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
import rts2comm

# email communication
import smtplib
# try pre 4.0 email version
try:
        from email.mime.text import MIMEText
except:
        from email.MIMEText import MIMEText

from datetime import datetime

class FlatAttempt:
	"""Class used to log attempts to produce flat."""
	def __init__ (self, exptime, average, ratio, res):
		self.date = datetime.today()
		self.exptime = exptime
		self.average = average
		self.ratio = ratio
		self.res = res

	def toString(self):
		return "%s %f: %f ratio %f result %s" % (self.date, self.exptime, self.average, self.ratio, self.res)

class Flat:
	"""Flat class. It holds system configuration for skyflats."""
	def __init__(self,filter,binning=None,ngood=None,window=None):
		self.filter = filter
		self.binning = binning
		self.ngood = ngood
		self.window = window
		self.log = []

	def attempt(self, exptime, average, ratio, res):
		self.log.append(FlatAttempt(exptime, average, ratio, res))
	
	def logString(self):
		ret = ""
		for a in self.log:
			ret += a.toString () + '\n'
		return ret

	def signature(self):
		"""Return signature string of given flat image configuration."""
		if (self.binning == None and self.window == None):
			return self.filter
		return '%s_%s_%s' % (self.filter,self.binning,self.window)

class FlatScript (rts2comm.Rts2Comm):
	"""Class for taking and processing skyflats."""
	def __init__(self):
		# Configuration (filters, binning, ..) for evening, we will use
		# reverse for morning. You fill array with Flat objects, which
		# describes configuration. If you do not have filters, use None
		# for filter name.  self.eveningFlats = [Flat(None)] #
		# configuration for camera without filters
		self.eveningFlats = [Flat('Y'),Flat('B'),Flat('b'),Flat('g'),Flat('r'),Flat('i'),Flat('z')]
		
		# Filters for morning - reverse of evening filters - deep copy them first.
		self.morningFlats = self.eveningFlats[:]
		
		# If you would like to take different filters/setting during
		# morning, replace this reverse with definition similar to what
		# was set for self.eveningFlats.
		self.morningFlats.reverse ()

		# If dark exposures for acquired filters should be taken.
		self.doDarks = True

		# rename images which are useless for skyflats to this path.
		# Fill in something (preferably with %f at the end - see man
		# rts2.ini) if you would like to keep copy of images which does
		# not met flat requirements. This is particularly usefull for
		# debugging.
		self.unusableExpression = None 

		# report flat production to this email
		self.email = None

		self.observatoryName = '<please fill here observatory name>'

		self.flat = None

		self.SaturationLevel = 65536 # should be 16bit for the nonEM and 14bit for EM)
		self.OptimalFlat = self.SaturationLevel / 3
		self.optimalRange = 0.3 # range of allowed divertion from OptimalFlat. 0.3 means 30%
		self.allowedOptimalDeviation = 0.1 # deviation allowed from optimalRange to start real flats
		self.BiasLevel = 390 # although this could be set to 0 and should not make much difference
		self.defaultNumberFlats = 10 # Number of flats that we want to obtain
		self.sleepTime = 1 # number of seconds to wait for optimal flat conditions

		self.maxDarkCycles = None

		self.expTimes = range(1,20) # allowed exposure times
		self.expTimes = map(lambda x: x/2.0, self.expTimes)

		self.startExpTime = self.expTimes[0] # starting exposure time. Must be within expTimes[0] .. expTimes[-1]

		self.eveningMultiply = 1 # evening multiplying - good for cameras with long readout times
		self.morningMultiply = 1 # morning multiplying

		self.shiftRa = 10.0 / 3600  # shift after every flat in RA [degrees]
		self.shiftDec = 10.0 / 3600 # shift after every flat in DEC [degrees]

		# self.waitingSubWindow = '100 100 200 200' # x y w h of subwindow while waiting for good flat level
		self.waitingSubWindow = None # do not use subwindow to wait for flats
		self.isSubWindow = False

		self.flatNum = 0
		self.flatImages = []
		self.usedExpTimes = []

	def optimalExpTime(self,ratio,expMulti):
		exptime = expMulti * self.exptime / ratio # We adjust the exposure time for the next exposure, so that it is close to the optimal value
		if (exptime < self.expTimes[0]):
			if (self.isEvening ()):
				return self.startExpTime
			return self.expTimes[0]
		if (exptime > self.expTimes[-1]):
			if (self.isEvening ()):
		  		return self.expTimes[-1]
			return self.exptime  # the caller decides on exptime whether to continue or to stop

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
		if (self.flat.window is None):
			self.setValue('WINDOW','-1 -1 -1 -1')
		else:
			self.setValue('WINDOW',self.flat.window)
		self.isSubWindow = False

	def unusableImage(self,imgname):
		"""Properly dispose image which cannot be used for flats."""
		if (self.unusableExpression is None):
			return self.delete(imgname)
		return self.rename(imgname,self.unusableExpression)

	def acquireImage(self):
		"""Acquires images for flats. Return 0 if image was added to flats, 1 if it was too brigth, -1 if it was too dark."""
		self.setValue('exposure',self.exptime)
		img = self.exposure(self.beforeReadout)
		avrg = self.getValueFloat('average') # Calculate average of image (can be just the central 100x100pix if you want to speed up)
		ratio = (avrg - self.BiasLevel) / self.OptimalFlat
		ret = None
		expMulti = 1

		if (not (self.isSubWindow)):
			if (self.isEvening()):
				expMulti = self.eveningMultiply
			else:
				expMulti = self.morningMultiply

		if (ratio <= 0): # special case, ratio is bellow bias
			self.log('W','average is bellow bias: average %f bias %f. Please adjust BiasLevel in flats script.' % avrg, self.BiasLevel)
			self.unusableImage(img)
			ratio = 0.000000001
			ret = -1
		elif (abs(1.0 - ratio) <= self.optimalRange): # Images within optimalRange of the optimal flux value
			ret = 0
		  	if (self.isSubWindow):
				self.fullWindow()
				self.unusableImage(img)
			else:
				flatName = self.toFlat(img)
				self.flatImages[self.flatNum].append(flatName)
				# add used exposure time - if it does not exists
				try:
					self.usedExpTimes.index(self.exptime)
				except ValueError:
					self.usedExpTimes.append(self.exptime)
		elif (abs(1.0 - ratio) < (self.optimalRange + self.allowedOptimalDeviation)):
			if (self.isSubWindow):
				self.fullWindow()
			self.unusableImage(img)
			# we believe that the next image will be good one..
			ret = 0
		else:
			self.unusableImage(img) #otherwise it is not useful and we get rid of it
			if (ratio > 1.0):
				ret = 1
			else:
				ret = -1

		self.exptime = self.optimalExpTime(ratio,expMulti)

		# if the image falls within reasonable boundaries, took full image
		if (self.exptime > self.expTimes[0] and self.exptime < self.expTimes[-1]):
			if (self.isSubWindow):
				self.fullWindow()
			# do not overide status here 

		# from ret to brightness
		brightness = 'OK'
		if (ret < 0):
			brightness = 'dim'
		elif (ret > 0):
			brightness = 'bright'

		self.log('I',"run ratio %f avrg %f ngood %d filter %s next exptime %f ret %s" % (ratio,avrg,len(self.flatImages[self.flatNum]),self.flat.filter,self.exptime,brightness))
		return ret

	def setConfiguration(self):
		if (self.flat.filter != None):
			self.setValue('filter',self.flat.filter)
		if (self.flat.binning != None):
			self.setValue('binning',self.flat.binning)
		else:
			self.setValue('binning',0)
		if (self.flat.ngood != None):
			self.numberFlats = self.flat.ngood
		else:
			self.numberFlats = self.defaultNumberFlats
	
	def execute(self, evening):
		self.exptime = self.startExpTime

		if (not ((self.waitingSubWindow is None) or (self.isSubWindow))):
			self.isSubWindow = True
			self.setValue('WINDOW',self.waitingSubWindow)

		while (len(self.flatImages[self.flatNum]) < self.numberFlats): # We continue until we have enough flats
			imgstatus = self.acquireImage()
			if (evening):
				if (imgstatus == -1 and self.exptime >= self.expTimes[-1]):
					# too dim image and exposure time change cannot correct it
					return
				elif (imgstatus == 1):
					time.sleep(self.sleepTime)
				# 0 mean good image, just continue..
			else:
				# morning
				if (imgstatus == 1 and self.exptime <= self.expTimes[0]):
					# too bright image and exposure time change cannot correct it
					return
				elif (imgstatus == -1):
					time.sleep(self.sleepTime) # WAIT sleepTime seconds (we would wait to until the sky is a bit brighter
				# good image, just continue as usuall
	
	def takeFlats(self, evening):
		for self.flatNum in range(0,len(self.usedFlats)): # starting from the bluest and ending with the redest
		  	self.flat = self.usedFlats[self.flatNum]
			self.flatImages.append([])
			self.setConfiguration()
			self.execute(evening)

	def takeDarks(self):
		"""Take flats dark images in spare time."""
		self.setValue('SHUTTER','DARK')
		i = 0
		if (len(self.usedExpTimes) == 0):
			self.usedExpTimes = [self.expTimes[0],self.expTimes[-1]]
		while (True):
			if (self.maxDarkCycles is not None and i >= self.maxDarkCycles):
				return
			i += 1
			for exp in self.usedExpTimes:
				sun_alt = self.getValueFloat('sun_alt','centrald')
				next_t = self.getValueInteger('next','EXEC')
				if (sun_alt >= 0.5 or not (next_t == 2 or next_t == -1)):
					self.setValue('SHUTTER','LIGHT')
					return
				self.setValue('exposure',exp)
				dark = self.exposure()
				self.toDark(dark)

	def createMasterFits(self,of,files):
		"""Process acquired flat images."""
		import numpy
		import os
		import pyfits

		f = pyfits.fitsopen(files[0])
		d = numpy.empty([len(files),len(f[0].data),len(f[0].data[0])])
		d[0] = f[0].data / numpy.mean(f[0].data)
		for x in range(1,len(files)):
		  	f = pyfits.fitsopen(files[x])
			d[x] = f[0].data / numpy.mean(f[0].data)
		if (os.path.exists(of)):
	  		self.log('I',"removing %s" % (of))
			os.unlink(of)
		f = pyfits.open(of,mode='append')
		m = numpy.median(d,axis=0)
		# normalize
		m = m / numpy.max(m)
		i = pyfits.PrimaryHDU(data=m)
		f.append(i)
		self.log('I','writing %s of min: %f max: %f mean: %f std: %f median: %f' % (of,numpy.min(m),numpy.max(m),numpy.mean(m),numpy.std(m),numpy.median(numpy.median(m))))
		f.close()

	def run(self):
		# make sure we are taking light images..
		self.setValue('SHUTTER','LIGHT')
		# choose filter sequence..
		if (self.isEvening()):
		  	self.usedFlats = self.eveningFlats
			self.takeFlats(True)
		else:
		  	self.usedFlats = self.morningFlats
			self.takeFlats(False)

		if (self.doDarks):
			self.log('I','finished flats, taking calibration darks')
			self.takeDarks()

		self.log('I','producing master flats')

		# basic processing of taken flats..
		goodFlats = []
		badFlats = []
		for i in range(0,len(self.flatImages)):
		  	sig = self.usedFlats[i].signature()
		  	if (len(self.flatImages[i]) >= 3):
			  	self.log('I',"creating master flat for %s" % (sig))
				self.createMasterFits('/tmp/master_%s.fits' % (sig), self.flatImages[i])
				goodFlats.append(self.usedFlats[i])
			else:
			  	badFlats.append(self.usedFlats[i])

		if (self.email != None):
			msg = 'Flats finished at %s.\n\nGood flats: %s\nBad flats: %s\n\n' % (datetime.today(),string.join(map(Flat.signature,goodFlats),';'),string.join(map(Flat.signature,badFlats),';'))
			for flat in self.usedFlats:
				msg += "\n\n" + flat.signature() + ':\n' + flat.logString()

			mimsg = MIMEText(msg)
			mimsg['Subject'] = 'Flats report from %s' % (self.observatoryName)
			mimsg['To'] = self.email

			s = smtplib.SMTP('localhost')
			s.sendmail('robtel@example.com',self.email.split(','),mimsg.as_string())
			s.quit()

		self.log('I','flat scripts finished, waiting for change of next target')
		while (True):
			sun_alt = self.getValueFloat('sun_alt','centrald')
			next_t = self.getValueInteger('next','EXEC')
			if (sun_alt >= 0.5 or not (next_t == 2 or next_t == -1)):
				self.setValue('SHUTTER','LIGHT')
				return
			time.sleep(10)

a = FlatScript()
a.run()

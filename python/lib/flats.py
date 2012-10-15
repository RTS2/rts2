#!/usr/bin/python

# Script for obtaining twilight skyflats.
#
# If you would like to customize this file for your setup, please use something
# similar to flat.py. This file is sometimes updated and changes made to
# configuration might get lost.
#
# If you would like to process images, you need to have numpy and pyfits
# installaed. Please see median.py in RTS2/scripts for more details.
#
# As with all scripts intended to be called by RTS2 exe script command, you can
# test this script by calling it and verifiing that it prints something what
# does not look like error message on standard output.
#
# (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
# (C) 2009,2010 Antonio de Ugarte & Petr Kubanek <petr@kubanek.net>
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

import sys
import string
import time
import scriptcomm

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
		return "%s %.2f: %f ratio %f result %s" % (self.date, self.exptime, self.average, self.ratio, self.res)

class Flat:
	"""Flat class. It holds system configuration for skyflats."""
	def __init__(self,filter,binning=None,ngood=None,window=None,expTimes=None):
		self.filter = filter
		self.binning = binning
		self.ngood = ngood
		self.window = window
		self.expTimes = expTimes
		self.attempts = []

	def attempt(self, exptime, average, ratio, res):
		self.attempts.append(FlatAttempt(exptime, average, ratio, res))
	
	def attemptString(self):
		ret = ""
		for a in self.attempts:
			ret += a.toString () + '\n'
		return ret

	def signature(self):
		"""Return signature string of given flat image configuration."""
		if (self.binning == None and self.window == None):
			return self.filter
		return '%s_%s_%s' % (self.filter,self.binning,self.window)

class FlatScript (scriptcomm.Rts2Comm):
	"""Class for taking and processing skyflats.

	:param eveningFlats: flats for evening. An array of Flat classes.
	:param morningFlats: flats for morning. An arra of Flat class. If it is None, reveresed :param:`eveningFlats`
	    is used
	:param maxBias: maximal number of bias (0 sec) frames
	:param maxDarks: maximal number of dark frames. Dark exposures are the same as used for skyflats
	:param expTimes: exposure times for flats attempts"""

	def __init__(self,eveningFlats=[Flat(None)],morningFlats=None,maxBias=0,maxDarks=0,expTimes=range(1,20)):
		scriptcomm.Rts2Comm.__init__(self)
		# Configuration (filters, binning, ..) for evening, we will use
		# reverse for morning. You fill array with Flat objects, which
		# describes configuration. If you do not have filters, use None
		# for filter name.  self.eveningFlats = [Flat(None)] #
		# configuration for camera without filters
		self.eveningFlats = eveningFlats
	
		if morningFlats is None:
			# Filters for morning - reverse of evening filters - deep copy them first.
			self.morningFlats = self.eveningFlats[:]
			self.morningFlats.reverse ()
		else:
		  	self.morningFlats = morningFlats
	
		# maximal number of bias frames
		self.maxBias = maxBias

		# maximal number of dark frames
		self.maxDarks = maxDarks

		# rename images which are useless for skyflats to this path.
		# Fill in something (preferably with %f at the end - see man
		# rts2.ini) if you would like to keep copy of images which does
		# not met flat requirements. This is particularly usefull for
		# debugging.
		self.unusableExpression = None 

		self.flat = None

		# fill in default flat and other levels
		self.flatLevels()

		self.defaultExpTimes = expTimes
		self.expTimes = self.defaultExpTimes
		self.startExpTime = self.expTimes[0] # starting exposure time. Must be within expTimes[0] .. expTimes[-1]
		
		self.waitingSubWindow = None # do not use subwindow to wait for flats
		self.isSubWindow = False

		self.flatNum = 0
		self.flatImages = []
		self.usedExpTimes = []

		# dark filter
		self.df = None

		self.goodFlats = []
		self.badFlats = []
		self.is_evening = self.isEvening()

	def flatLevels(self,optimalFlat=65536/3,optimalRange=0.3,allowedOptimalDeviation=0,biasLevel=0,defaultNumberFlats=9,sleepTime=1,eveningMultiply=1,morningMultiply=1,shiftRa=10/3600.0,shiftDec=10/3600.0):
		"""Set flat levels. Adjust diferent parameters of the algorithm.

		:param optimalFlat:  optimal (target) flat value. You would like to see something like 20k for ussual 16bit CCDs.
		:param optimalRange: set optimal range. Images within this range will be taken as good. If you set optimalFlat to 20k, and optimalRange to 0.5, then images within 20k-0.5*20k:20k+0.5*20k (=10k:30k) range will be taken as good flats
		:param allowedOptimalDeviation: allowed deviation from optimal range to think that the next image will be good one.
		:param biasLevel: image bias level. It is substracted from the image. Default to 0.
		:param defaultNumberFlats: number of target flat images. Default to 9. After :param:`defaultNumberFlats` flat images are acquired in given filter, next flat target is used.
		:param sleepTime: time in seconds to sleep between attempt exposures. The algorithm does not sleep if good flats are acquired.
		:param eveningMultiply: multiplication factor for evening exposure times
		:param morningMultiply: multiplication factor for morning exposure times
		:param shiftRa: shift in RA after exposure, to prevent stars occuring at the same spot
		:param shiftDec: shift in DEC after exposure, to prevent stars occuring at the same spot
		"""
		self.optimalFlat = optimalFlat
		self.optimalRange = optimalRange
		self.allowedOptimalDeviation = allowedOptimalDeviation
		self.biasLevel = biasLevel
		self.defaultNumberFlats = defaultNumberFlats
		self.sleepTime = sleepTime

		self.eveningMultiply = eveningMultiply
		self.morningMultiply = morningMultiply

		self.shiftRa = shiftRa
		self.shiftDec = shiftDec

	def darkFilter(self,df):
		self.df = df
	
	def setSubwindow(self,waitingSubWindow):
		"""Set flat subwindow. Subwindow is used to wait for correct exposure time.

		:param waitingSubWindow: the subwindow, specified as string 'x y w h' - e.g. '100 100 200 200' is the correct parameter value."""
		self.waitingSubWindow = waitingSubWindow
	
	def optimalExpTime(self,ratio,expMulti):
		"""Get optimal exposure time. ratio is the ratio of image
		average vs. optimal flat image - e.g. ratio is 1 if the image is the optimal skyflat image, < 1 if
		it is too dim and > 1 if it is too bright.

		expMulti is used as slope parameter to adjust morning or evening exposure times on systems with 
		a long readout time. It is generally >=1 at evening and <= 1 at morning. Its purpose is to adjust for
		time lost during readout of the detector - at evening as sky gets dimmer, exposure time of the next
		frame must be increased to get proper average value. It should
		be left to == 1 at beginning, and etimated if needed. The current algorithm behaves on usuall setups fine
		if CCD readout time is < ~1 minute.
		  
		Returns either one of the self.expTimes values, or value bigger
		than self.expTimes[-1] if image is too dim and new images
		should not be attempted."""

		exptime = expMulti * self.exptime / ratio # adjust the exposure time for the next exposure, so that it is close to the optimal value. expMulti adjust for time lost during CCD readout
		if exptime < self.expTimes[0]:
			# too dim image. return the first value, will wait at
			# morning for brighter sky, will move to next filter at
			# evening
			return self.expTimes[0]
	
		if exptime > self.expTimes[-1]: # too dim image
			if self.is_evening == False:
				 # if that happens at morning, return low
				 # exposure time for probing The objective is
				 # to stay at low exposure time as long as the
				 # images are too dim, so the algorithm will
				 # cover period when the brightness of the
				 # morning sky is enought to provide a good
				 # skyflat at long eposure time
		  		return self.expTimes[0]
			# the caller decides on exptime whether to continue or to stop
			return exptime

		# extreme cases, e.g. estimated exposure time laying outside of
		# allowed exposure range, were sorted above. The algorithm now
		# search for exposure time lower than the estimated exposure
		# time - so the returned exposure time is one in the expTimes
		# array.  This measurement is indended to set skyflat at
		# "standartize" exposure times, so only limited number of dark
		# images has to be taken to properly calibrate images.
		lastE = self.expTimes[0]
		for e in self.expTimes:
			if exptime < e:
				return e if self.is_evening else lastE
			lastE = e

		return self.expTimes[-1]
	
	def beforeReadout(self):
		if (self.isSubWindow == True):
			return
		if (self.shiftRa != 0 or self.shiftDec != 0):
			self.incrementValueType(scriptcomm.DEVICE_TELESCOPE,"OFFS",self.shiftRa.__str__() + ' ' + self.shiftDec.__str__())

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
		ratio = (avrg - self.biasLevel) / self.optimalFlat
		ret = None
		expMulti = 1

		if not (self.isSubWindow):
			if self.is_evening:
				expMulti = self.eveningMultiply
			else:
				expMulti = self.morningMultiply

		if (ratio <= 0): # special case, ratio is bellow bias
			self.log('W','average is bellow bias: average %f bias %f. Please adjust biasLevel in flats script.' % (avrg, self.biasLevel))
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
			# we believe that the next image will be good .. but it still must fit inside range
			ret = 2
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

		# test if "next good" does fit inside range
		if ret == 2:
			if self.exptime > self.expTimes[-1]:
				ret = -1
				self.log('I','next image should be good, but it has too long exposure')
			elif self.exptime < self.expTimes[0]:
				ret = 1
				self.log('I','next image should be good, but it has too short exposure')
			else:
				ret = 0
				self.log('I','next image should be good one')

		# from ret to brightness
		brightness = 'OK'
		if (ret < 0):
			brightness = 'dim'
		elif (ret > 0):
			brightness = 'bright'

		self.log('I',"run ratio %f avrg %f ngood %d filter %s next exptime %f ret %s" % (ratio,avrg,len(self.flatImages[self.flatNum]),self.flat.filter,self.exptime,brightness))
		self.flat.attempt(self.exptime,ratio,avrg,brightness)
		return ret

	def setConfiguration(self):
		if self.flat.filter is not None:
			self.setValue('filter',self.flat.filter)
		if self.flat.binning is not None:
			self.setValue('binning',self.flat.binning)
		else:
			self.setValue('binning',0)
		if self.flat.ngood is not None:
			self.numberFlats = self.flat.ngood
		else:
			self.numberFlats = self.defaultNumberFlats
		if self.flat.expTimes is not None:
			self.expTimes = self.flat.expTimes
		else:
			self.expTimes = self.defaultExpTimes

		self.startExpTime = self.expTimes[0]
	
	def execute(self, evening):
		self.exptime = self.startExpTime

		if (not ((self.waitingSubWindow is None) or (self.isSubWindow))):
			self.isSubWindow = True
			self.setValue('WINDOW',self.waitingSubWindow)

		while (len(self.flatImages[self.flatNum]) < self.numberFlats): # We continue until we have enough flats
			imgstatus = self.acquireImage()
			if (evening):
				if (imgstatus == -1 and self.exptime >= self.expTimes[-1]) or self.exptime >= self.expTimes[-1]:
					# too dim image and exposure time change cannot correct it
					return
				elif imgstatus == 1:
					time.sleep(self.sleepTime)
				# 0 mean good image, just continue..
			else:
				# morning
				if (imgstatus == 1 and self.exptime <= self.expTimes[0]) or self.exptime < self.expTimes[0]:
					# too bright image and exposure time change cannot correct it
					return
				elif imgstatus == -1:
					time.sleep(self.sleepTime) # WAIT sleepTime seconds (we would wait to until the sky is a bit brighter
				# good image, just continue as usuall
	
	def takeFlats(self, evening):
		for self.flatNum in range(0,len(self.usedFlats)): # starting from the bluest and ending with the redest
		  	self.flat = self.usedFlats[self.flatNum]
			self.flatImages.append([])
			self.setConfiguration()
			self.execute(evening)

	def takeBias(self,usedConfigs):
		self.setValue('SHUTTER','DARK')
		self.setValue('exposure',0)
		i = 0
		while i < self.maxBias:
		  	i += 1
		  	for x in usedConfigs:
			  	if x[0] is None:
					self.setValue('binning',0)
				else:
					self.setValue('binning',x[0])
				if x[1] is None:
					self.setValue('WINDOW','-1 -1 -1 -1')
				else:
					self.setValue('WINDOW',x[1])
				bias = self.exposure()
				self.toDark(bias)

	def takeDarks(self,usedConfigs):
		"""Take flats dark images in spare time."""
		self.setValue('SHUTTER','DARK')
		if not (self.df is None):
			self.setValue('filter',self.df)
		i = 0
		if (len(self.usedExpTimes) == 0):
			self.usedExpTimes = [self.expTimes[0],self.expTimes[-1]]
		while i < self.maxDarks:
			i += 1
			for exp in self.usedExpTimes:
				sun_alt = self.getValueFloat('sun_alt','centrald')
				next_t = self.getValueInteger('next','EXEC')
				if (sun_alt >= -0.5 or not (next_t == 2 or next_t == 1 or next_t == -1)):
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

		f = pyfits.open(files[0])
		d = numpy.empty([len(files),len(f[0].data),len(f[0].data[0])])
		d[0] = f[0].data / numpy.mean(f[0].data)
		for x in range(1,len(files)):
		  	f = pyfits.open(files[x])
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

	def getData(self, domeDevice='DOME', tmpDirectory='/tmp/'):
		# make sure we are taking light images..
		self.setValue('SHUTTER','LIGHT')
		# choose filter sequence..
		if self.is_evening:
		  	self.usedFlats = self.eveningFlats
			self.takeFlats(True)
			self.log('I','finished skyflats')
		else:
		  	self.usedFlats = self.morningFlats
			self.takeFlats(False)
			if domeDevice:
				self.log('I','finished skyflats, closing dome')
				self.sendCommand('close_for 3600',domeDevice)
			else:
				self.log('I','finished skyflats')

		# configurations which were used..
		usedConfigs = []
		for i in range(0,len(self.flatImages)):
			if len(self.flatImages[i]) >= 3:
			  	conf = [self.usedFlats[i].binning,self.usedFlats[i].window]
				try:
					i = usedConfigs.index(conf)
				except ValueError,v:
					usedConfigs.append(conf)
		
		if self.maxBias > 0:
		  	self.log('I','taking calibration bias')
			self.takeBias(usedConfigs)

		if self.maxDarks > 0:
			self.log('I','taking calibration darks')
			self.takeDarks(usedConfigs)

		self.log('I','producing master flats')

		# basic processing of taken flats..
		for i in range(0,len(self.flatImages)):
		  	sig = self.usedFlats[i].signature()
		  	if len(self.flatImages[i]) >= 3:
			  	self.log('I',"creating master flat for %s" % (sig))
				self.createMasterFits(tmpDirectory + '/master_%s.fits' % (sig), self.flatImages[i])
				self.goodFlats.append(self.usedFlats[i])
			else:
			  	self.badFlats.append(self.usedFlats[i])

	def run(self, domeDevice='DOME', tmpDirectory='/tmp/', receivers=None, subject='Skyflats report'):
		try:
			self.getData(domeDevice, tmpDirectory)
		except scriptcomm.Rts2NotActive,noa:
			self.log('W','flat script interruped')

			if receivers:
				self.sendEmail(receivers,subject)
			return

		if receivers:
			self.sendEmail(receivers,subject)

		self.finish ()

	def sendEmail(self,email,observatoryName):
		msg = ''
		try:
			msg = 'Flats finished at %s.\n\nGood flats: %s\nBad flats: %s\n\n' % (datetime.today(),string.join(map(Flat.signature,self.goodFlats),';'),string.join(map(Flat.signature,self.badFlats),';'))
			for flat in self.usedFlats:
				msg += "\n\n" + flat.signature() + ':\n' + flat.attemptString()
		except TypeError,te:
			msg = 'Cannot get flats - good: %s, bad: %s' % (self.goodFlats,self.badFlats)

		mimsg = MIMEText(msg)
		mimsg['Subject'] = 'Flats report from %s' % (observatoryName)
		mimsg['To'] = email

		s = smtplib.SMTP('localhost')
		s.sendmail('robtel@example.com',email.split(','),mimsg.as_string())
		s.quit()

	def finish(self):
		self.log('I','flat scripts finished, waiting for change of next target')
		self.setValue('SHUTTER','LIGHT')
		while (True):
			sun_alt = self.getValueFloat('sun_alt','centrald')
			next_t = self.getValueInteger('next','EXEC')
			if (sun_alt >= 0.5 or not (next_t == 2 or next_t == -1)):
				return
			time.sleep(10)

if __name__ == "__main__":
	a = FlatScript()
	a.run()

#!/usr/bin/python

# targets 557 564 & 1111 for now
# spectroscopic acquisition = centering & slit
#
# TODO:
# if not specified, chose optimum slit
# do dithering along the slit if more exposures requested
# must switch off the lamps if killed (exec sometimes does that)

import imgprocess
import scriptcomm
import pyfits
import numpy
import math
import time

class coloresAcquire (scriptcomm.Rts2Comm):
	def __init__(self):
                scriptcomm.Rts2Comm.__init__(self)
                self.telname = self.getDeviceByType(scriptcomm.DEVICE_TELESCOPE)
                self.imgproc = imgprocess.ImgProcess()
		
		# EXPOSURE TIME
		self.expTime = 300 # [s] one spectrum exposure time (should be somehow changable from outside)

		# SLIT to be used and related parameters
		self.filterName = "open" # [rts2 filter name] which filter to use
		self.slitName = "slit4" # [rts2 filter name] which slit to use
		self.slitExp = 1 # [s] exp to measure the slit (this is different depending on sky brightness), if it is too short slit cannot be reliably detected 
		self.slitPos = 555 # [pixels] but it is going to get detected anyway
		self.slitWidth = 7 # [pixels] but it is going to get measured anyway

		# BIAS exposure, good to know bias when predicting exptimes
		self.biasExp = 0.001 # [s] I do not like exptime = 0, so it is this short time
		self.CCDbias = 400 # [ADU] to be measured later anyway (this %^^%$#^& changes !!)

		# IDENTIFY exposures, need enough light to identify the field
		self.identifyExp = 20 # [s] exp time to identify the field ()

		# CENTERING of the object regardless of astrometry not yet implemented
		# self.centerExp = 2 # [s] exp time to center the object 

		# ARC calibration exposures related parameters
		self.doArcCalib = 0  # 0/1 whether to do lambda-calibration with an arc
		self.baseArcExp = 0.001 # [s] base arc exposure time (to be multiplied by whatever is found necessary to fully expose)
		self.minLightValue = 10000 # [lux] minimum light value to wait for in the arc chamber (when waiting for heat up of the lamp)
		self.exposeArcTo = 10000 # [ADU] tune arc exposures so that they have this maximal value
		self.lampWaitTimeout = 120 # timeout when waiting for the lamp to come up
		
		# bias, slit: vertically average N pixels to get better when searching for slit, computing bias etc.
		self.vertAvg = 128
	
		# this can be remove, I think (carefully)	
		self.totalCorr = [ 0 , 0 ] 

	# to make the code a little more readable
	def setupColoresExp ( self, flt, slit, grism, bpp, exptime ):

		self.setValue('FILTA', flt)
		self.setValue('FILTB', slit)
		self.setValue('FILTC', grism)
                self.setValue('ADCHANEL', bpp) # 0=14 bit 1=16bit
                self.setValue('exposure', exptime)

	# get vertAvg pixel wide area along the central horizontal line of the CCD, average it vertically and return it
	# as a numpy array
	def getCCDCut ( self, image ):

		f = pyfits.open ( image )

		width=len(f[0].data)
	        height=len(f[0].data[0])
	
	        l = numpy.zeros(width)
	
		for i in range ( 0, width ):
			for j in range ( int(height/2-self.vertAvg/2-1), int(height/2+self.vertAvg/2) ):
				l[i]+=f[0].data[j][i]/self.vertAvg

		f.close()

		return l

	# scan the image center and report FWHM of the object within (same as previous function, optimized differently) 
	def getStarFWHM (self, image, exptime):
			
		W=32
		H=32
	
		f = pyfits.open ( image )

		width=len(f[0].data)
		height=len(f[0].data[0])

		xcenter=f[0].header['slitposx']
		ycenter=512

		l = numpy.zeros(W)
		b1 = numpy.zeros(W)
		b2 = numpy.zeros(W)

		for i in range ( 0, W ):
			for j in range ( int(height/2-H/2-1), int(height/2+H/2) ):
				b1[i]+=f[0].data[j][xcenter-3*W/2-1+i]/W

		for i in range ( 0, W ):
			for j in range ( int(height/2-H/2-1), int(height/2+H/2) ):
				b2[i]+=f[0].data[j][xcenter+W/2-1+i]/W

		for i in range ( 0, W ):
			for j in range ( int(height/2-H/2-1), int(height/2+H/2) ):
				l[i]+=f[0].data[j][xcenter-W/2-1+i]/W

		f.close()

		bg = numpy.median(b1)/2 + numpy.median(b2)/2
		l -= bg
		dl = numpy.median(numpy.abs(b1-numpy.median(b1)))/2 + numpy.median(numpy.abs(b2-numpy.median(b2)))/2
		# and maximum
		ml = numpy.amax(l)

		snr = ml / dl
		#exp = exp * 30 * dl / ml

		q=0.0
		ww=0.0
		i=0
		A=0

		for ll in l:
			if(ll>6*dl):
				q+=i*ll
				ww+=ll
			A+=ll
			i+=1

		self.log('I','acquire: snr, exptime {0} {1}'.format(snr, exptime))

		exptime *= math.sqrt(30/snr)
		self.log('I','acquire: snr, exptime2 {0} {1}'.format(snr, exptime))
                #if exptime>0: 
		self.setValue('exposure', exptime)

		if(ww>0):
			self.log('I','acquire: fwhm {0} {1}'.format(A/ml, q/ww, snr, exptime))
			return (A/ml,exptime)
		else:
			return (-1,exptime)

	
	# dump CCD contents, to make sure there is no acumulated charge	-- this makes not much sense in normal
	# circumstances as the CCD cleans itself, but when the lamp is on it may be important
	def dumpCCD (self ):
		# save the set up exposure and insert 0
                self.setValue('exposure', 0)
	
		# shoot and forget three images	
		image = self.exposure()
		self.delete(image)
                image = self.exposure()
		self.delete(image)
                image = self.exposure()
		self.delete(image)

	def getBias ( self ):

		self.setupColoresExp ( 'open', 'closed', 'open', 0, self.biasExp )		
		self.dumpCCD ()		
                self.setValue('exposure', self.biasExp)
                image = self.exposure()
		
		l = self.getCCDCut ( image )
		self.CCDbias = numpy.median(l)

		self.log('I','acquire: CCDbias={0}'.format(self.CCDbias))
		self.toArchive(image)
		
	def getArc ( self ):
		
		self.setupColoresExp ( self.filterName, self.slitName, 'grism1', 0, self.baseArcExp )		
		self.dumpCCD ()		

		newExpTime=self.baseArcExp

		# i = max number of iterations, a = maximum in the image
		i=0
		a=0
	
		while a < 0.95 * self.exposeArcTo and i < 5: # for i in range ( 0, 3 ):

                	self.setValue('exposure', newExpTime)
			image = self.exposure()
			l = self.getCCDCut ( image )

			a = numpy.amax(l)
			newExpTime *= (self.exposeArcTo-self.CCDbias)/(a-self.CCDbias)
		
			self.log('I','acquire: getArc a={0} bias={1} newExpTime={2}'.format(a,self.CCDbias,newExpTime))

			if  a > 0.95*self.exposeArcTo :
				self.toArchive(image)
			else :
				self.delete(image)
			i+=1

		# by now the exposure should be stabilized on a nice value, so lets do a 10x more to see weaker lines too (it costs us nothing)
		self.setValue('exposure', newExpTime * 10)
	        image = self.exposure()
                l = self.getCCDCut ( image )
		self.toArchive(image)

	def getSlit ( self ):

		exp=self.slitExp
		snr=1
		
		# get the slit with at least 30sigma	
		while snr < 30 :

			self.setupColoresExp ( 'open', self.slitName, 'open', 0, exp )		

			image = self.exposure()
			l = self.getCCDCut ( image )
			self.delete(image)

			# remove background
			l -= numpy.median(l)
			# get noise
			dl = numpy.median(numpy.abs(l))
			# and maximum
			ml = numpy.amax(l)

			snr = ml / dl
			exp = exp * 30 * dl / ml 

		q=0.0
		ww=0.0
		i=0
		A=0

		for ll in l:
			if(ll>6*dl):
				q+=i*ll
				ww+=ll
			A+=ll
			i+=1

		# almost there
		self.slitWidth=A/ml
		self.slitPos=q/ww

		ww=0    
		q=0

		# redo the same but only in "3-sigma" neighborhood of the 
		# center (to kill the possible particles)
		for i in range(int(self.slitPos-3*self.slitWidth),int(self.slitPos+3*self.slitWidth)):
			q+=i*l[i] # weighted sum
			ww+=l[i] # weight & area

		self.slitWidth=ww/ml
		self.slitPos=q/ww
                self.setValue('slitposx', self.slitPos )
		self.log('I','acquire: {0} is at x={1}, {2} px wide'.format(self.slitName,self.slitPos, self.slitWidth))

	# use astrometry to recenter & identify the field (via corr_)
	def identifyAndCorrect ( self, limit ):

		# wait for the telescope to be ready
                ret = self.waitIdle(self.telname,300)

		# and get the image
                image = self.exposure()

		# this would be a prefferable way of sending the slitpos to the astrometric routine but it does not work
		# but it is likely that we do not have the original fits here but a mere copy, so whatever we write into
		# it gets lost and does not reach astrometry.py
		#
		# [it is now set up as a value inside the camera and autosaved by fits writer]
		#
		# f=pyfits.open(image)
		# f[0].header.set('SLITPOS',self.slitPos,'Slit position as measured by acquire.getSlit()') # new pyfits
		# f[0].header.update('SLITPOS',self.slitPos) # old pyfits
		# f.close()	

		# try astrometry	
		try:
                	offs = self.imgproc.run(image)
		
		except:
			self.log('I','acquire: astrometry did not succeed')
			self.toTrash(image)
			return 100
			
		self.toArchive(image)

		self.log('I','acquire: returned offsets {0} {1} arcsec'.format(offs[2]*3600, offs[3]*3600))


 		# /4 is a MAGIC no one really cares that much about centering alonf the slit, so it may be 4x less precise
		offx=max([math.fabs(offs[3]),math.fabs(offs[2])/4])
		self.log('I','acquire: offx {0} arcsec'.format(offx*3600))

		# if no correction needed, stop
		if offx>limit:

			# get current corrections from the telescope
			buf=self.getValue('CORR_',self.telname).split();
			self.totalCorr[0] = float(buf[0]);
			self.totalCorr[1] = float(buf[1]);

			# jinak by mne celkem neurazelo delat to jako pole, ale nejak se mi nedari to ukecat
			self.totalCorr[0] += offs[2]
			self.totalCorr[1] += offs[3]

			# jde to i bez tlumeni, ale necham to tady, muze se hodit
			# tlumeni atanem: 1800 = 3600/2 da 50% korekci pri 2" odchylky
			# self.totalCorr[0] +=  offs[2]*math.fabs(math.atan(offs[2]*1800)/1.5708)
			# self.totalCorr[1] +=  offs[3]*math.fabs(math.atan(offs[3]*1800)/1.5708)

			self.log('I','acquire: totalCorr={0}'.format(str(self.totalCorr)))
			
			# send corrections to the telescope	
			self.setValue('CORR_','{0} {1}'.format(self.totalCorr[0], self.totalCorr[1]),self.telname)

		# Return the total offset to permit a cyclic repetition until matched
		return offx

	# refocus using the object in the center
	def doRefocus( self ):
		# assume the system is ready in terms of setup (after recenter it should be)
		self.log('I','acquire: Refocus started')
		
		# wait for the telescope to be ready
                ret = self.waitIdle(self.telname,300)

		# basic exptime...
		exptime=2.0 # s
		step = 15

		self.setupColoresExp ( 'open', 'open', 'open', 1, exptime )		
               
		# initial calib. 
		image = self.exposure()
		(fwhm1,exptime) = self.getStarFWHM (image, exptime)
		self.delete (image)
		self.log('I','acquire: exposure3 {0}'.format(exptime))

		# and get the image
		# focuser offset
		self.setValue('FOC_TOFF', +step,'F0')
                ret = self.waitIdle('F0',30)
                image = self.exposure()
		(fwhm1,exptime) = self.getStarFWHM (image, exptime)
		self.delete (image)

		self.setValue('FOC_TOFF', 0,'F0')
                ret = self.waitIdle('F0',30)
                image = self.exposure()
		(fwhm2,exptime) = self.getStarFWHM (image, exptime)
		self.delete (image)
	
		self.setValue('FOC_TOFF', -step,'F0')
                ret = self.waitIdle('F0',30)
                image = self.exposure()
		(fwhm3,exptime) = self.getStarFWHM (image, exptime)
		self.delete (image)
	
		foff=self.getValueFloat('FOC_FOFF','F0')

		while fwhm1 > 0 and fwhm2 > 0 and fwhm3 > 0:  
			
			if fwhm1 < fwhm2 and fwhm1 < fwhm3 :  # tj. ohnisko je pred fwhm1 - nejmensi je jednicka
				# posunout, zapomenout trojku, 
				self.log('I','acquire: Refocus go up')
				fwhm3=fwhm2
				fwhm2=fwhm1
				foff += step
				self.setValue('FOC_TOFF', +step,'F0')
				self.setValue('FOC_FOFF', foff,'F0')
				ret = self.waitIdle('F0',30)
		                image = self.exposure()
                		(fwhm1,exptime) = self.getStarFWHM (image, exptime)
                		self.delete (image)
				continue
			
			if fwhm1 > fwhm3 and fwhm2 > fwhm3 : # tj. onisko je za fwhm3 - nejmensi je trojka
				# posunout, zapomenout jednicku, 
				self.log('I','acquire: Refocus go down')
				fwhm1=fwhm2
				fwhm2=fwhm3
				foff -= step
				self.setValue('FOC_TOFF', -step,'F0')
				self.setValue('FOC_FOFF', foff,'F0')
				ret = self.waitIdle('F0',30)
		                image = self.exposure()
                		(fwhm3,exptime) = self.getStarFWHM (image, exptime)
                		self.delete (image)
				continue

		#	if ( fwhm2 < fwhm3 and fwhm2 < fwhm1 and fwhm3 < fwhm1 ) or (fwhm2 < fwhm3 and fwhm2 < fwhm1 and fwhm1 < fwhm2 ): 
				# ohnisko je mezi 2 a 3  - nejmensi je dvojka a 
				# ohnisko je mezi 1 a 2  - nejmensi je dvojka a 
				# a proste se na to vyserem a koncime, s nastavenim na prostredek
			self.log('I','acquire: Refocus stop here')
			break

		self.setValue('FOC_TOFF', 0,'F0')

		self.log('I','acquire: Refocus finished w1={0} w2={1} w3={2} off={3}'.format(fwhm1, fwhm2, fwhm3, foff))

	# do dithering cycle (offs) this is not ok, im afraid 
	def doDither ( self, n ):
                self.setValue('OFFS','{0} {1}'.format(ra_o,dec_o),self.telname)
                ret = self.waitIdle(self.telname,300)
                self.log('D','acquire: offseting telescope {0} returns {1}'.format(self.telname,ret))
                #self.setValue('OFFS','0 0',self.telname)

	def doArc ( self ):
		self.log('I','acquire: Taking lambda calibration images')

		self.log('I','acquire: power-on the ArHg lamp')
		self.setValue('H','on','COLLAMP')

		self.log('I','acquire: insert the mirror')
		self.setValue('mirror','off','COLORES')

		self.log('I','acquire: wait for the lamp (max {0}s)'.format(self.lampWaitTimeout))
		light=0
		spentTime=0
		timeToSleep=1		

		while light<self.minLightValue and spentTime < self.lampWaitTimeout :
			light=self.getValueFloat('L2','COLORES')
			time.sleep(timeToSleep)
			spentTime += timeToSleep
			
		if spentTime >= self.lampWaitTimeout:
			self.log('I','acquire: timeout when waiting for the lamp (continuing normally)')
		else :
			self.log('I','acquire: spent {0}s waiting for the lamp'.format(spentTime))

		light = self.getValueFloat('L2','COLORES')
		self.log('I','acquire: get the arc with light={0}'.format(light))
		self.getArc()
		
		self.log('I','acquire: power-off the lamp')
		self.setValue('H','off','COLLAMP')

		self.log('I','acquire: remove the mirror')
		self.setValue('mirror','on','COLORES')
	
	def run (self):
		
		self.target = self.getValueInteger('current','EXEC')
		self.log('I','acquire: *** start with target={0} ***'.format(self.target))

		# this should do no harm when on selector and ..and it does nothing
		self.setValue('next', self.target , 'EXEC')
			
                ret = self.waitIdle(self.telname,300)

		# get the CCD bias
		self.getBias()

		# get an image of the slit, and identify the coordinate where we are going to point
		self.getSlit()

		# get an arc image (arc has to heat up a little), that time can be used to make an image of the slit
		if self.doArcCalib:		
			self.doArc()
	
		first=1

		# repeat the observation cycle
		while self.getValueInteger('next','EXEC') == self.target or first == 1:  # until selector likes us to go

			first=0
			# first identify the field and the object, get it to the requested pixel! 
			self.setupColoresExp ( 'open', 'open', 'open', 1, self.identifyExp )		
			dist=100
			limit=3.0/3600.0 
			retries=5 # max tries, then fail
			while dist>limit and retries>0:
				dist=self.identifyAndCorrect(limit) 
				retries -= 1

			self.doRefocus()

			# first identify the field and the object, get it to the requested pixel! 
			self.setupColoresExp ( 'open', 'open', 'open', 1, self.identifyExp )		
			dist=100
			limit=1.0/3600.0 
			retries=5 # max tries, then fail
			while dist>limit and retries>0:
				dist=self.identifyAndCorrect(limit) 
				retries -= 1

			# TODO get fwhm & decide the optimum slit 

			# TODO guide the object to the slit position (no astrometry shuld be necessary here, + less exposure needed)

			# if it was not a failure
			if retries>0:

				self.log('I','acquire: get the thrugh-slit image (distance={0} arcsec)'.format(str(dist*3600)))
				self.setupColoresExp ( 'open', self.slitName, 'open', 1, self.identifyExp )		
				image = self.exposure()
				self.toArchive(image)

				# fourth put in the grism & start the long exposure
				self.log('I','acquire: expose spectrum exptime={0}s'.format(self.expTime))
				self.setupColoresExp ( self.filterName, self.slitName, 'grism1', 1, self.expTime)		
				image = self.exposure()
				self.toArchive(image)
			
				# TODO decide a possible dithering movement, change the CCD target coords 

			else : 
				self.log('I','acquire: failed to identify the field')

                # this is after the end of loop
		self.log('I','acquire: *** end ***')

a = coloresAcquire()
a.run()
	

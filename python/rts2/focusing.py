#!/usr/bin/env python
#
# Autofocosing routines.
#
# You will need: scipy matplotlib sextractor
# This should work on Debian/ubuntu:
# sudo apt-get install python-matplotlib python-scipy python-pyfits sextractor
#
# If you would like to see sextractor results, get DS9 and pyds9:
#
# http://hea-www.harvard.edu/saord/ds9/
#
# Please be aware that current sextractor Ubuntu packages does not work
# properly. The best workaround is to install package, and the overwrite
# sextractor binary with one compiled from sources (so you will have access
# to sextractor configuration files, which program assumes).
#
# (C) 2002-2008 Stanislav Vitek
# (C) 2002-2010 Martin Jelinek
# (C) 2009-2010 Markus Wildi
# (C) 2010-2014 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
# (C) 2010      Francisco Forster Buron, Universidad de Chile
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

from . import scriptcomm
from . import sextractor
sepPresent = False
try:
	import sep
	sepPresent = True
except Exception as ex:
	pass

from matplotlib.pyplot import *
from scipy import *
from scipy import optimize
import numpy

LINEAR = 0
"""Linear fit"""
P2 = 1
"""Fit using 2 power polynomial"""
P4 = 2
"""Fit using 4 power polynomial"""
H3 = 3
"""Fit using general Hyperbola (three free parameters)"""
H2 = 4
"""Fit using Hyperbola with fixed slope at infinity (two free parameters)"""

class Focusing (scriptcomm.Rts2Comm):
	"""Take and process focussing data."""

	def __init__(self,exptime = 2,step=1,attempts=20,filterGalaxies=False):
		scriptcomm.Rts2Comm.__init__(self)
		self.exptime = 2 # 60 # 10
		self.focuser = self.getValue('focuser')
		try:
			self.getValueFloat('AF_step',self.focuser)
		except NameError:
			self.step = 1 # 0.2
		else:
			self.step = self.getValueFloat('AF_step',self.focuser)
		try:
			self.getValueFloat('AF_attempts',self.focuser)
		except NameError:
			self.attempts = 20 #30 # 20
		else:
			self.attempts = self.getValueInteger('AF_attempts',self.focuser)
		# if |offset| is above this value, try linear fit
		self.linear_fit = self.step * self.attempts / 2.0
		# target FWHM for linear fit
		self.linear_fit_fwhm = 3.5
		self.filterGalaxies = filterGalaxies

	def doFit(self,fit):
		b = None
		errfunc = None
		fitfunc_r = None
		p0 = None

		# try to fit..
		# this function is for flux.. 
		#fitfunc = lambda p, x: p[0] * p[4] / (p[4] + p[3] * (abs(x - p[1])) ** (p[2]))

		# prepare fit based on its type..
		if fit == LINEAR:
			fitfunc = lambda p, x: p[0] + p[1] * x
			errfunc = lambda p, x, y: fitfunc(p, x) - y # LINEAR - distance to the target function
			p0 = [1, 1]
			fitfunc_r = lambda x, p0, p1: p0 + p1 * x
		elif fit == P2:
			fitfunc = lambda p, x: p[0] + p[1] * x + p[2] * (x ** 2)
			errfunc = lambda p, x, y: fitfunc(p, x) - y # P2 - distance to the target function
			p0 = [1, 1, 1]
			fitfunc_r = lambda x, p0, p1, p2 : p0 + p1 * x + p2 * (x ** 2)
		elif fit == P4:
			fitfunc = lambda p, x: p[0] + p[1] * x + p[2] * (x ** 2) + p[3] * (x ** 3) + p[4] * (x ** 4)
			errfunc = lambda p, x, y: fitfunc(p, x) - y # P4 - distance to the target function
			p0 = [1, 1, 1, 1, 1]
			fitfunc_r = lambda x, p0, p1: p0 + p1 * x + p2 * (x ** 2) + p3 * (x ** 3) + p4 * (x ** 4)
		elif fit == H3:
			fitfunc = lambda p, x: sqrt(p[0] ** 2 + p[1] ** 2 * (x - p[2])**2)
			errfunc = lambda p, x, y: fitfunc(p, x) - y # H3 - distance to the target function
			p0 = [400., 3.46407715307, self.fwhm_MinimumX]  # initial guess based on real data
			fitfunc_r = lambda x, p0, p1, p2 : sqrt(p0 ** 2 + p1 ** 2 * (x - p2) ** 2)
		elif fit == H2:
			fitfunc = lambda p, x: sqrt(p[0] ** 2 + 3.46407715307 ** 2 * (x - p[1])**2) # 3.46 based on H3 fits
			errfunc = lambda p, x, y: fitfunc(p, x) - y # H2 - distance to the target function
			p0 = [400., self.fwhm_MinimumX]  # initial guess based on real data
			fitfunc_r = lambda x, p0, p1 : sqrt(p0 ** 2 + 3.46407715307 ** 2 * (x - p1) ** 2)
		else:
			raise Exception('Unknow fit type {0}'.format(fit))

		self.fwhm_poly, success = optimize.leastsq(errfunc, p0[:], args=(self.focpos, self.fwhm))

		b = None

		if fit == LINEAR:
			b = (self.linear_fit_fwhm - self.fwhm_poly[0]) / self.fwhm_poly[1]
		elif fit == H3:
			b = self.fwhm_poly[2]
			self.log('I', 'found minimum FWHM: {0}'.format(abs(self.fwhm_poly[0])))
			self.log('I', 'found slope at infinity: {0}'.format(abs(self.fwhm_poly[1])))
		elif fit == H2:
			b = self.fwhm_poly[1]
			self.log('I', 'found minimum FWHM: {0}'.format(abs(self.fwhm_poly[0])))
		else:
			b = optimize.fmin(fitfunc_r,self.fwhm_MinimumX,args=(self.fwhm_poly), disp=0)[0]
		self.log('I', 'found FWHM minimum at offset {0}'.format(b))
		return b

	def tryFit(self,defaultFit):
		"""Try fit, change to linear fit if outside allowed range."""
		b = self.doFit(defaultFit)
		if (abs(b - numpy.average(self.focpos)) >= self.linear_fit):
			self.log('W','cannot do find best FWHM inside limits, trying H2 fit - best fit is {0}, average focuser position is {1}'.format(b, numpy.average(self.focpos)))
			b = self.doFit(H2)
			if (abs(b - numpy.average(self.focpos)) >= self.linear_fit):
				self.log('W','cannot do find best FWHM inside limits, trying linear fit - best fit is {0}, average focuser position is {1}'.format(b, numpy.average(self.focpos)))
				b = self.doFit(LINEAR)
				return b,LINEAR
			return b,H2
		return b,defaultFit


	def doFitOnArrays(self,fwhm,focpos,defaultFit):
		self.fwhm = array(fwhm)
		self.focpos = array(focpos)
		self.fwhm_MinimumX = 0
		min_fwhm=fwhm[0]
		for x in range(0,len(fwhm)):
			if fwhm[x] < min_fwhm:
				self.fwhm_MinimumX = x
				min_fwhm = fwhm[x]
		return self.tryFit(defaultFit)

	def findBestFWHM(self,tries,defaultFit=H3,min_stars=95,ds9display=False,threshold=2.7,deblendmin=0.03):
		# X is FWHM, Y is offset value
		self.focpos=[]
		self.fwhm=[]
		fwhm_min = None
		self.fwhm_MinimumX = None
		keys = list(tries.keys())
		keys.sort()
		sextr = sextractor.Sextractor(threshold=threshold,deblendmin=deblendmin)
		for k in keys:
			try:
				sextr.runSExtractor(tries[k])
				fwhm,fwhms,nstars = sextr.calculate_FWHM(min_stars,self.filterGalaxies)
			except Exception as ex:
				self.log('W','offset {0}: {1}'.format(k,ex))
				continue
			self.log('I','offset {0} fwhm {1} with {2} stars'.format(k,fwhm,nstars))
			focpos.append(k)
			fwhm.append(fwhm)
			if (fwhm_min is None or fwhm < fwhm_min):
				fwhm_MinimumX = k
				fwhm_min = fwhm
		return focpos,fwhm,fwhm_min,fwhm_MinimumX

	def __sepFindFWHM(self,tries):
		from astropy.io import fits
		import math
		import traceback
		focpos=[]
		fwhm=[]
		fwhm_min=None
		fwhm_MinimumX=None
		keys = list(tries.keys())
		keys.sort()
		ln2=math.log(2)
		for k in keys:
			try:
				fwhms=[]
				ff=fits.open(tries[k])
				# loop on images..
				for i in range(1,len(ff)):
					data=ff[i].data
					bkg=sep.Background(numpy.array(data,numpy.float))
					sources=sep.extract(data-bkg, 5.0 * bkg.globalrms)
					for s in sources:
						fwhms.append(2 * math.sqrt(ln2 * (s[12]**2 + s[13]**2)))
				im_fwhm=numpy.median(fwhms)
				# find median from fwhms measurements..
				self.log('I','offset {0} fwhm {1} with {2} stars'.format(k,im_fwhm,len(fwhms)))
				focpos.append(k)
				fwhm.append(im_fwhm)
				if (fwhm_min is None or im_fwhm < fwhm_min):
					fwhm_MinimumX = k
					fwhm_min = im_fwhm
			except Exception as ex:
				self.log('W','offset {0}: {1} {2}'.format(k,ex,traceback.format_exc()))
		return focpos,fwhm,fwhm_min,fwhm_MinimumX


	def findBestFWHM(self,tries,defaultFit=H3,min_stars=15,ds9display=False,threshold=2.7,deblendmin=0.03):
		# X is FWHM, Y is offset value
		self.focpos=[]
		self.fwhm=[]
		self.fwhm_min = None
		self.fwhm_MinimumX = None
		if sepPresent:
			self.focpos,self.fwhm,self.fwhm_min,self.fwhm_MinimumX = self.__sepFindFWHM(tries)
		else:
			self.focpos,self.fwhm,self.fwhm_min,self.fwhm_MinimumX = self.__sexFindFWHM(tries,threshold,deblendmin)
		self.focpos = array(self.focpos)
		self.fwhm = array(self.fwhm)

		return self.tryFit(defaultFit)

	def beforeReadout(self):
		self.current_focus = self.getValueFloat('FOC_POS',self.focuser)
		if (self.num == self.attempts):
			self.setValue('FOC_TOFF',0,self.focuser)
		else:
			self.off += self.step
			self.setValue('FOC_TOFF',self.off,self.focuser)

	def takeImages(self):
		self.setValue('exposure',self.exptime)
		self.setValue('SHUTTER','LIGHT')
		self.off = -1 * self.step * (self.attempts / 2)
		self.setValue('FOC_TOFF',self.off,self.focuser)
		tries = {}
		# must be overwritten in beforeReadout
		self.current_focus = None

		for self.num in range(1,self.attempts+1):
			self.log('I','starting {0}s exposure on offset {1}'.format(self.exptime,self.off))
			img = self.exposure(self.beforeReadout,'%b/focusing/%N/%o/%f')
			tries[self.current_focus] = img

		self.log('I','all focusing exposures finished, processing data')

		return self.findBestFWHM(tries)

	def run(self):
		self.focuser = self.getValue('focuser')
		# send to some other coordinates if you wish so, or disable this for target for fixed coordinates
		self.altaz (89,90)
		b,fit = self.takeImages()
		if fit == LINEAR:
			self.setValue('FOC_DEF',b,self.focuser)
			b,fit = self.takeImages()

		self.setValue('FOC_DEF',b,self.focuser)

	def plotFit(self,b,ftype):
		"""Plot fit graph."""
		fitfunc = None

		if ftype == LINEAR:
			fitfunc = lambda p, x: p[0] + p[1] * x 
		elif ftype == P2:
			fitfunc = lambda p, x: p[0] + p[1] * x + p[2] * (x ** 2)
		elif ftype == P4:
			fitfunc = lambda p, x: p[0] + p[1] * x + p[2] * (x ** 2) + p[3] * (x ** 3) + p[4] * (x ** 4)
		elif ftype == H3:
			fitfunc = lambda p, x: sqrt(p[0] ** 2 + p[1] ** 2 * (x - p[2]) ** 2)
		elif ftype == H2:
			fitfunc = lambda p, x: sqrt(p[0] ** 2 + 3.46407715307 ** 2 * (x - p[1]) ** 2) # 3.46 based on HYPERBOLA fits
		else:
			raise Exception('Unknow fit type {0}'.format(ftype))

		x = linspace(self.focpos.min() - 1, self.focpos.max() + 1)

		plot (self.focpos, self.fwhm, "r+", x, fitfunc(self.fwhm_poly, x), "r-")

		show()

#!/usr/bin/python

# Autofocosing
#
# (C) 2002-2008 Stanislav Vitek
# (C) 2002-2010 Martin Jelinek
# (C) 2009-2010 Markus Wildi
# (C) 2010      Petr Kubanek, Institute of Physics

import rts2comm
import sextractor

from pylab import *
from scipy import *
from scipy import optimize

class Focusing (rts2comm.Rts2Comm):
	"""Take and process focussing data."""
	def __init__(self):
		self.exptime = 10
		self.step= 1
		self.attempts = 20
		self.focuser = 'F0'

	def findBestFWHM(self,tries):
		# X is FWHM, Y is offset value
		self.focpos=[]
		self.fwhm=[]
		fwhm_min = None
		fwhm_MinimumX = None
		for k in tries.keys():
			self.focpos.append(k)
			fwhm = sextractor.getFWHM(tries[k],10)
			self.log('I','offset {0} fwhm {1}'.format(k,fwhm))
			self.fwhm.append(fwhm)
			if (fwhm_min is None or fwhm < fwhm_min):
				fwhm_MinimumX = k
				fwhm_min = fwhm
		self.focpos = array(self.focpos)
		self.fwhm = array(self.fwhm)

		# try to fit..
		# this function is for flux.. 
		#fitfunc = lambda p, x: p[0] * p[4] / (p[4] + p[3] * (abs(x - p[1])) ** (p[2]))

		fitfunc = lambda p, x: p[0] + p[1] * x + p[2] * (x ** 2) + p[3] * (x ** 3) + p[4] * (x ** 4)
		errfunc = lambda p, x, y: fitfunc(p, x) - y # Distance to the target function

		p0 = [1, 1, 1, 1, 1]
		self.fwhm_poly, success = optimize.leastsq(errfunc, p0[:], args=(self.focpos, self.fwhm))

		fitfunc_r = lambda x, p0, p1, p2, p3, p4: p0 + p1 * x + p2 * (x ** 2) + p3 * (x ** 3) + p4 * (x ** 4)

		b = optimize.fmin(fitfunc_r,fwhm_MinimumX,args=(self.fwhm_poly), disp=0)[0]
		self.log('I', 'found FHWM minimum at offset {0}'.format(b))
		self.doubleValue('best_offset','offset from last focusing',b)
		return b

	def beforeReadout(self):
		if (self.num == self.attempts):
			self.setValue('FOC_FOFF',0,self.focuser)
		else:
			self.setValue('FOC_FOFF',self.off)

	
	def run(self):
		self.setValue('exposure',self.exptime)
		self.setValue('SHUTTER','LIGHT')
		self.off = -1 * self.step * (self.attempts / 2)
		self.setValue('FOC_FOFF',self.off,self.focuser)
		tries = {}

		for self.num in range(1,self.attempts+1):
		  	self.log('I','starting {0}s exposure on offset {1}'.format(self.exptime,self.off))
			img = self.exposure(self.beforeReadout)
			tries[self.off] = img
			self.off += self.step

		self.log('I','all focusing exposures finished, processing data')

		self.findBestFWHM(tries)

if __name__ == "__main__":
	a = Focusing()
	a.run ()

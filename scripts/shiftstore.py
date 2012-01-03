#!/usr/bin/python
#
# Shift-store focusing.
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
# properly. The best workaround is to install package, and then overwrite
# sextractor binary with one compiled from sources (so you will have access
# to sextractor configuration files, which program assumes).
#
# (C) 2011  Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place - Suite 330, Boston, MA  02111-1307, USA.

import sextractor
import sys
import focusing
import numpy
from scipy import array

class ShiftStore:
	"""
	Shift-store focusing. Works in the following steps
	    - extract sources (with sextractor)
	    - order sources by magnitude/flux
	    - try to find row for the brightests source
	      - filter all sources too far away in X
	      - try to find sequence, assuming selected source can be any in
		the sequence
	    - run standard fit on images (from focusing.py) Parameters
	      governing the algorithm are specified in ShiftStore constructor.

	@param shifts shifts performed between exposures, in pixels. Lenght of this array is equal to ((number of sources in a row) - 1).
	"""

	def __init__(self,shifts=[50,50,50,50,50,50,50,50,50,50]):
		self.objects = None
		self.sequences = []
		self.xsep = self.ysep = 5
		self.shifts = shifts
		self.focpos = range(0,len(self.shifts) + 1)

	def testObjects(self,x,can,i):
		"""
		Test if there is sequence among candidates matching expected
		shifts.  Candidates are stars not far in X coordinate from
		source (here we assume we are looking for vertical lines on
		image). Those are ordered by y and searched for stars at
		expected positions. If multiple stars falls inside this box,
		then the one closest in magnitude/brightness estimate is
		selected.
		"""
		ret = []
		# here we assume y is third, and some brightnest estimate fourth member in x
		yi = x[2]  # expected y position
		xb = x[3]  # expected brightness
		# calculate first expected shift..
		for j in range(0,i):
			yi -= self.shifts[j]
		# go through list, check for candidate stars..
		cs = None
		sh = 0
		# now interate through candidates
		for j in range(0,len(can)):
		  	# if the current shift index is equal to expected source position...
			if sh == i:
				# append x to sequence, and increase sh (and expected Y position)
				yi += self.shifts[sh]
				sh += 1
				ret.append(x)
		  	# get close enough..
			# please note that this algorithm is not looking for partial 
			if abs(can[j][2] - yi) < self.ysep:
				# find all other sources in vicinity..
		  		k = None
				cs = can[j] # _c_andidate _s_tar
				for k in range(j+1,len(can)):
				  	# something close enough..
					if abs(can[k][2] - yi) < self.ysep:
						if abs(can[k][3] - xb) < abs (cs[3] - xb):
							cs = can[k]
					else:
						# otherwise don't care..
						break
				# append candidate star
				ret.append(cs)
				if k is not None:
					j = k
				# don't exit if the algorithm make it to end of shifts
				try:
					yi += self.shifts[sh]
				except IndexError,ie:
					return ret
				sh += 1
		return ret


	def findRowObjects(self,x):
		"""
		Find objects in row. Search for sequence of stars, where x fit
		as one member. Return the sequence, or None if the sequence
		cannot be found."""

		xid = x[0]   # running number
		xcor = x[1]  # X coordinate
		can = []     # canditate stars
		for y in self.objects:
			if xid != y[0] and abs(xcor - y[1]) < self.xsep:
				can.append(y)
		# sort by Y axis..
		can.sort(cmp=lambda x,y: cmp(x[2],y[2]))
		# assume selected object is one in shift sequence
		# place it at any possible position in shift sequence, and test if the sequence can be found
		for i in range(0,len(self.shifts) + 1):
			# test if sequence can be found..
			ret = self.testObjects(x,can,i)
			# and if it is found, return it
			if len(ret) == len(self.shifts) + 1:
				return ret
		# cannot found sequnce, so return None
		return None

	def runOnImage(self,fn,interactive=False):
		"""
		Run algorithm on image. Extract sources with sextractor, and
		pass them through sequence finding algorithm, and fit focusing position.
		"""

		c = sextractor.Sextractor(fn,['NUMBER','X_IMAGE','Y_IMAGE','MAG_BEST','FLAGS','CLASS_STAR','FWHM_IMAGE','A_IMAGE','B_IMAGE'],sexpath='/usr/bin/sextractor',sexconfig='/usr/share/sextractor/default.sex',starnnw='/usr/share/sextractor/default.nnw')
		c.runSExtractor()


		self.objects = c.objects
		# sort by flux/brightness
		self.objects.sort(cmp=lambda x,y:cmp(x[3],y[3]))

		print 'from {0} extracted {1} sources'.format(fn,len(c.objects))


  		d = ds9()
		# display in ds9
		d.set('file {0}'.format(fn))

		for x in self.objects:
			d.set('regions','image; point {0} {1} # point=x 5 color=red'.format(x[1],x[2]))

		sequences = []
		usednum = []

		for x in self.objects:
			# do not examine already used objects..
			if x[0] in usednum:
				continue

		  	# find object in a row..
			b = self.findRowObjects(x)
			if b is None:
				continue
			sequences.append(b)
			d.set('regions select none')
			d.set('regions','image; circle {0} {1} 20 # color=yellow tag = sel'.format(x[1],x[2]))
			for obj in b:
				usednum.append(obj[0])
				d.set('regions','image; circle {0} {1} 15 # color=blue tag = sel'.format(obj[1],obj[2]))
			print 'best mag: ',x[3]
			d.set('regions select group sel')
			d.set('regions delete select')
			for obj in b:
				d.set('regions','image; circle {0} {1} 10 # color = green'.format(obj[1],obj[2]))
			if len(sequences) > 15:
				break
		# if enough sequences were found, process them and try to fit results
		if len(sequences) > 10:
			# get median of FWHM from each sequence
			fwhm=[]
			for x in range(0,len(self.shifts) + 1):
				fa = []
				for o in sequences:
					fa.append(o[x][6])
				m = numpy.median(fa)
				fwhm.append(m)
			# fit it
			foc = focusing.Focusing()

			res,ftype = foc.doFitOnArrays(fwhm,self.focpos,focusing.H2)
			print res,ftype

			if interactive:
				foc.plotFit(res,ftype)

if __name__ == "__main__":
  	# test method
	from ds9 import *
	sc = ShiftStore()
  	for fn in sys.argv[1:]:
		sc.runOnImage(fn,True)

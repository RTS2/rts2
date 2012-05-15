#!/usr/bin/python
# (C) 2010, Markus Wildi, markus.wildi@one-arcsec.org
# (C) 2011-2012, Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
#   usage 
#   astrometry.py fits_filename
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2, or (at your option)
#   any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software Foundation,
#   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#   Or visit http://www.gnu.org/licenses/gpl.html.
#

__author__ = 'kubanek@fzu.cz'

import os
import shutil
import string
import subprocess
import sys
import re
import time
import pyfits
import tempfile
import numpy
import math

import dms

class WCSAxisProjection:
	def __init__(self,fkey):
		self.wcs_axis = None
		self.projection_type = None
		self.sip = False

		for x in fkey.split('-'):
			if x == 'RA' or x == 'DEC':
				self.wcs_axis = x
			elif x == 'TAN':
				self.projection_type = x
			elif x == 'SIP':
				self.sip = True
		if self.wcs_axis is None or self.projection_type is None:
			raise Exception('uknown projection type {0}'.format(fkey))

def xy2wcs(x,y,fitsh):
	"""Transform XY pixel coordinates to WCS coordinates"""
	wcs1 = WCSAxisProjection(fitsh['CTYPE1'])
	wcs2 = WCSAxisProjection(fitsh['CTYPE2'])
	# retrieve CD matrix
	cd = numpy.array([[fitsh['CD1_1'],fitsh['CD1_2']],[fitsh['CD2_1'],fitsh['CD2_2']]])
	# subtract reference pixel
	xy = numpy.array([x,y]) - numpy.array([fitsh['CRPIX1'],fitsh['CRPIX2']])
	xy = numpy.dot(cd,xy)

	if wcs1.wcs_axis == 'RA' and wcs2.wcs_axis == 'DEC':
		dec = xy[1] + fitsh['CRVAL2']
		if wcs1.projection_type == 'TAN':
			if abs(dec) != 90:
				xy[0] /= math.cos(math.radians(dec))
		return [xy[0] + fitsh['CRVAL1'],dec]

	if wcs1.wcs_axis == 'DEC' and wcs2.wcs_axis == 'RA':
		dec = xy[0] + fitsh['CRVAL1']
		if wcs2.projection_type == 'TAN':
			if abs(dec) != 90:
				xy[1] /= math.cos(math.radians(dec))
		return [xy[1] + fitsh['CRVAL2'],dec]
	raise Exception('unsuported axis combination {0} {1]'.format(wcs1.wcs_axis,wcs2.wcs_axis))

def cd2crota(fitsh):
	"""Read FITS CDX_Y headers, returns rotangs"""
	cd1_1 = fitsh['CD1_1']
	cd1_2 = fitsh['CD1_2']
	cd2_1 = fitsh['CD2_1']
	cd2_2 = fitsh['CD2_2']
	return (math.atan2(cd2_1,cd1_1),math.atan2(-cd1_2,cd2_2))

class AstrometryScript:
	"""calibrate a fits image with astrometry.net."""
	def __init__(self,fits_file,odir=None,scale_relative_error=0.05,astrometry_bin='/usr/local/astrometry/bin'):
		self.scale_relative_error=scale_relative_error
		self.astrometry_bin=astrometry_bin

		self.fits_file = fits_file
		self.odir = odir
		if self.odir is None:
			self.odir=tempfile.mkdtemp()

		self.infpath=self.odir + '/input.fits'
		shutil.copy(self.fits_file, self.infpath)

	def run(self,scale=None,ra=None,dec=None,replace=False,verbose=False):

		solve_field=[self.astrometry_bin + '/solve-field', '-D', self.odir,'--no-plots', '--no-fits2fits']

		if scale is not None:
			scale_low=scale*(1-self.scale_relative_error)
			scale_high=scale*(1+self.scale_relative_error)
			solve_field.append('-u')
			solve_field.append('app')
			solve_field.append('-L')
			solve_field.append(str(scale_low))
			solve_field.append('-H')
			solve_field.append(str(scale_high))

		if ra is not None and dec is not None:
			solve_field.append('--ra')
			solve_field.append(str(ra))
			solve_field.append('--dec')
			solve_field.append(str(dec))
			solve_field.append('--radius')
			solve_field.append('5')

		solve_field.append(self.infpath)

		if verbose:
			print 'running',' '.join(solve_field)
	    
		proc=subprocess.Popen(solve_field,stdout=subprocess.PIPE,stderr=subprocess.PIPE)

		radecline=re.compile('Field center: \(RA H:M:S, Dec D:M:S\) = \(([^,]*),(.*)\).')

		ret = None

		while True:
			a=proc.stdout.readline()
			if a == '':
				break
			if verbose:
				print a
			match=radecline.match(a)
			if match:
				ret=[dms.parseDMS(match.group(1)),dms.parseDMS(match.group(2))]
		if replace and ret is not None:
			shutil.move(self.odir+'/input.new',self.fits_file)
	       
		# cleanup
		shutil.rmtree(self.odir)
		
		return ret

	# return offset from 
	def getOffset(self,x=None,y=None):
		ff=pyfits.fitsopen(self.fits_file,'readonly')
		fh=ff[0].header
		ff.close()

		if x is None:
			x=fh['NAXIS1']/2.0
		if y is None:
			y=fh['NAXIS2']/2.0

		rastrxy = xy2wcs(x,y,fh)
		ra=fh['OBJRA']
		dec=fh['OBJDEC']

		return (ra-rastrxy[0],dec-rastrxy[1])

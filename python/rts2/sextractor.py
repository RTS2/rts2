# Sextractor-Python wrapper.
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
# (C) 2010-2012  Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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
import subprocess
import os
import tempfile
import traceback

class Sextractor:
	"""Class for a catalogue (SExtractor result)"""
	def __init__(self, fields=['NUMBER', 'FLUXERR_ISO', 'FLUX_AUTO', 'X_IMAGE', 'Y_IMAGE', 'MAG_BEST', 'FLAGS', 'CLASS_STAR', 'FWHM_IMAGE', 'A_IMAGE', 'B_IMAGE','EXT_NUMBER'], sexpath='sextractor', sexconfig='/usr/share/sextractor/default.sex', starnnw='/usr/share/sextractor/default.nnw', threshold=2.7, deblendmin = 0.03, saturlevel=65535, verbose=False):
		self.sexpath = sexpath
		self.sexconfig = sexconfig
		self.starnnw = starnnw

		self.fields = fields
		self.objects = []
		self.threshold = threshold
		self.deblendmin = deblendmin
		self.saturlevel = saturlevel

		self.verbose = verbose

	def get_field(self,fieldname):
		return self.fields.index(fieldname)

	def runSExtractor(self,filename):
	    	pf,pfn = tempfile.mkstemp()
		ofd,output = tempfile.mkstemp()
		pfi = os.fdopen(pf,'w')
		for f in self.fields:
			pfi.write(f + '\n')
		pfi.flush()

		cmd = [self.sexpath, filename, '-c', self.sexconfig, '-PARAMETERS_NAME', pfn, '-DETECT_THRESH', str(self.threshold), '-DEBLEND_MINCONT', str(self.deblendmin), '-SATUR_LEVEL', str(self.saturlevel), '-FILTER', 'N', '-STARNNW_NAME', self.starnnw, '-CATALOG_NAME', output]
		if not(self.verbose):
			cmd.append('-VERBOSE_TYPE')
			cmd.append('QUIET')
		try:
			proc = subprocess.Popen(cmd)
			proc.wait()
		except OSError as err:
			print('canot run command: "', ' '.join(cmd), '", error ',err, file=sys.stderr)
			raise err

		# parse output
		self.objects = []
		of = os.fdopen(ofd,'r')
		while (True):
		 	x=of.readline()
			if self.verbose:
				print(x, end=' ')
			if x == '':
				break
			if x[0] == '#':
				continue
			self.objects.append(list(map(float,x.split())))
	
		# unlink tmp files
		pfi.close()
		of.close()

		os.unlink(pfn)
		os.unlink(output)

	def sortObjects(self,col):
	        """Sort objects by given collumn."""
	    	self.objects.sort(cmp=lambda x,y: cmp(x[col],y[col]))

	def reverseObjects(self,col):
		"""Reverse sort objects by given collumn."""
		self.objects.sort(cmp=lambda x,y: cmp(x[col],y[col]))
		self.objects.reverse()

	def filter_galaxies(self,limit=0.2):
		"""Filter possible galaxies"""
		try:
			i_class = self.get_field('CLASS_STAR')
			ret = []
			for x in self.objects:
				if x[i_class] > limit:
					ret.append(x)
			return ret
		except ValueError as ve:
			print('result does not contain CLASS_STAR')
			traceback.print_exc()
	
	def get_FWHM_stars(self,starsn=None,filterGalaxies=True,segments=None):
		"""Returns candidate stars for FWHM calculations. """
		if len(self.objects) == 0:
			raise Exception('Cannot find FWHM on empty source list')

		obj = None
		if filterGalaxies:
			obj = self.filter_galaxies()
			if len(obj) == 0:
				raise Exception('Cannot find FWHM - all detected sources were filtered out as galaxies')
		else:
			obj = self.objects

		try:
			# sort by magnitude
			i_mag_best = self.get_field('MAG_BEST')
			obj.sort(cmp=lambda x,y: cmp(x[i_mag_best],y[i_mag_best]))
			fwhmlist = []

			a = 0
			b = 0

			i_flags = self.get_field('FLAGS')
			i_class = self.get_field('CLASS_STAR')
			i_seg = self.get_field('EXT_NUMBER')

			for x in obj:
				if segments and x[i_seg] not in segments:
					continue
				if x[i_flags] == 0 and (filterGalaxies == False or x[i_class] != 0):
					fwhmlist.append(x)
					if starsn and len(fwhmlist) >= starsn:
						break
				else:
					if self.verbose:
						print('rejected - FLAGS:', x[i_flags], ', CLASS_STAR:', x[i_class], 'line ', x)

			return fwhmlist
		except ValueError as ve:
			traceback.print_exc()
			return []


	def calculate_FWHM(self,starsn=None,filterGalaxies=True,segments=None):
		obj = self.get_FWHM_stars(starsn,filterGalaxies,segments)
		try:
			i_fwhm = self.get_field('FWHM_IMAGE')
			import numpy
			fwhms = [x[i_fwhm] for x in obj]
 			return numpy.median(fwhms), numpy.std(fwhms), len(fwhms)
 		#	return numpy.average(obj), len(obj)
		except ValueError as ve:
			traceback.print_exc()
			raise Exception('cannot find FWHM_IMAGE value')

#!/usr/bin/env python

# g(pl)-Point - GPLed Telescope pointing model fit, as described in paper by Marc Buie:
#
# ftp://ftp.lowell.edu/pub/buie/idl/pointing/pointing.pdf 
#
# (C) 2015-2016 Petr Kubanek <petr@kubanek.net>
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

import sys
import numpy as np
import libnova
import string

from math import radians,degrees,cos,sin,tan,sqrt,atan2,acos
from scipy.optimize import leastsq

import re

def flip_ra(a_ra,dec):
	if abs(dec) > 90:
		return (a_ra + 180) % 360
	return a_ra

def flip_dec(a_dec,dec):
	if dec > 90:
		return 180 - a_dec
	elif dec < -90:
		return -180 - a_dec
	return a_dec

def print_model_input(filename,first):
	hdulist = fits.open(filename)
	h = hdulist[0].header
	w = wcs.WCS(h)
	ra,dec = w.all_pix2world(2000,2000,0)
	tar_telra = float(h['TAR_TELRA'])
	tar_teldec = float(h['TAR_TELDEC'])
	if first:
		print "#observatory",h['SITELONG'],h['SITELAT'],h['ELEVATION']
	print(h['IMGID'],h['JD'],h['LST'],tar_telra,tar_teldec,h['AXRA'],h['AXDEC'],ra,dec)

def normalize_az_err(errs):
	return np.array(map(lambda x: x if x < 180 else x - 360, errs % 360))

def normalize_ha_err(errs):
	return np.array(map(lambda x: x if x < 180 else x - 360, errs % 360))

class ExtraParam:
	"""Extra parameter - term for model evaluation"""
	def __init__(self,ede):
		p = ede.split(':')
		if len(p) != 3:
			raise Exception('invalid extra parameter format - {0}'.format(ede))

		self.function = p[0]
		self.param = p[1].split(';')
		self.consts = map(float,p[2].split(';'))

	def __str__(self):
		return '{0}\t{1}\t{2}'.format(self.function,';'.join(map(str,self.param)),';'.join(map(str,self.consts)))

# Computes, output, concetanetes and plot pointing models.
class GPoint:
	# @param   verbose  verbosity of the
	def __init__(self,verbose=0,latitude=None,longitude=None,altitude=None):
		self.aa_ha = None
		self.verbose = verbose
		self.indata = []
		# telescope latitude - north positive
		self.latitude = self.def_latitude = latitude
		self.longitude = self.def_longitude = longitude
		self.altitude = self.def_altitude = altitude
		self.altaz = False # by default, model GEM
		if latitude is not None:
			self.latitude_r = np.radians(latitude)
		self.best = None
		self.name_map = None
		# addtional terms for ra-dec model
		# addtional terms for alt-az model
		self.extra_az_start = 9
		self.extra_az = []
		self.extra_el_start = 9
		self.extra_el = []

	def equ_to_hrz(self,ha,dec):
		""" Transform HA-DEC (in radians) vector to ALT-AZ (in degrees) vector"""
		A = np.sin(self.latitude_r) * np.sin(dec) + np.cos(self.latitude_r) * np.cos(dec) * np.cos(ha)
		alt = np.arcsin(A)

		Z = np.arccos(A)
		Zs = np.sin(Z)
		As = (np.cos(dec) * np.sin(ha)) / Zs;
		Ac = (np.sin(self.latitude_r) * np.cos(dec) * np.cos(ha) - np.cos(self.latitude_r) * np.sin(dec)) / Zs;
		Aa = np.arctan2(As,Ac)

		return np.degrees(alt),(np.degrees(Aa) + 360) % 360

	def hrz_to_equ(self,az,alt):
		""" Transform AZ-ALT (in radians) vector to HA-DEC (in degrees) vector"""

		ha = np.arctan2(np.sin(az), (np.cos(az) + np.sin(self.latitude_r) + np.tan(alt) * np.cos(self.latitude_r)))
		dec = np.sin(self.latitude_r) * np.sin(alt) - np.cos(self.latitude_r) * np.cos(alt) * np.cos(az)
		dec = np.arcsin(dec)

		return np.degrees(ha),np.degrees(dec)

	def model_ha(self,params,a_ha,a_dec):
		return - params[4] \
			- params[5]/np.cos(a_dec) \
			- params[6]*np.tan(a_dec) \
			- (params[1]*np.sin(a_ha) - params[2]*np.cos(a_ha)) * np.tan(a_dec) \
			- params[3]*np.cos(self.latitude_r)*np.sin(a_ha) / np.cos(a_dec) \
			- params[7]*(np.sin(self.latitude_r) * np.tan(a_dec) + np.cos(self.latitude_r) * np.cos(a_ha))

	def model_dec(self,params,a_ha,a_dec):
		return - params[0] \
			- params[1]*np.cos(a_ha) \
			- params[2]*np.sin(a_ha) \
			- params[3]*(np.cos(self.latitude_r) * np.sin(a_dec) * np.cos(a_ha) - np.sin(self.latitude_r) * np.cos(a_dec)) \
			- params[8]*np.cos(a_ha)

	def get_extra_val_altaz(self,e,az,el,num):
		if e.param[num] == 'az':
			return az
		elif e.param[num] == 'el':
			return el
		elif e.param[num] == 'zd':
			return (np.pi / 2) - el
		else:
			sys.exit('unknow parameter {0}'.format(e.param[num]))

	def cal_extra_azalt(self,e,az,el):
		if e.function == 'sin':
			return np.sin(e.consts[0] * self.get_extra_val_altaz(e,az,el,0))
		elif e.function == 'cos':
			return np.cos(e.consts[0] * self.get_extra_val_altaz(e,az,el,0))
		elif e.function == 'tan':
			return np.tan(e.consts[0] * self.get_extra_val_altaz(e,az,el,0))
		elif e.function == 'sincos':
			return np.sin(e.consts[0] * self.get_extra_val_altaz(e,az,el,0)) * np.cos(e.consts[1] * self.get_extra_val_altaz(e,az,el,1))
		else:
			sys.exit('unknow function {0}'.format(e.function))

	def add_extra_az(self,e):
		self.extra_az.append(ExtraParam(e))
		self.extra_el_start += 1

	def add_extra_el(self,e):
		self.extra_el.append(ExtraParam(e))

	def model_az(self,params,a_az,a_el):
		ret = - params[0] \
			- params[1]*np.sin(a_az)*np.tan(a_el) \
			- params[2]*np.cos(a_az)*np.tan(a_el) \
			- params[3]*np.tan(a_el) \
			+ params[4]/np.cos(a_el)
		pi = self.extra_az_start
		for e in self.extra_az:
			ret += params[pi] * self.cal_extra_azalt(e, a_az, a_el)
			pi += 1
		return ret

	def model_el(self,params,a_az,a_el):
		ret = - params[5] \
			- params[2]*np.sin(a_az) \
			+ params[6]*np.cos(a_el) \
			+ params[7]*np.cos(a_az) \
			+ params[8]*np.sin(a_az)
		pi = self.extra_el_start
		for e in self.extra_el:
			ret += params[pi] * self.cal_extra_azalt(e, a_az, a_el)
			pi += 1
		return ret

	# Fit functions.
	# a_ha - target HA (hour angle)
	# r_ha - calculated (real) HA
	# a_dec - target DEC
	# r_dec - calculated (real) DEC
	# DEC > 90 or < -90 means telescope flipped (DEC axis continues for modelling purposes)
	def fit_model_ha(self,params,a_ha,r_ha,a_dec,r_dec):
		return a_ha - r_ha + self.model_ha(params,a_ha,a_dec)

	def fit_model_dec(self,params,a_ha,r_ha,a_dec,r_dec):
		return a_dec - r_dec + self.model_dec(params,a_ha,a_dec)

	def fit_model_gem(self,params,a_ra,r_ra,a_dec,r_dec):
		if self.verbose:
			print 'computing', self.latitude, self.latitude_r, params, a_ra, r_ra, a_dec, r_dec
#		return np.concatenate((self.fit_model_ha(params,a_ra,r_ra,a_dec,r_dec),self.fit_model_dec(params,a_ra,r_ra,a_dec,r_dec)))
		return libnova.angular_separation(np.degrees(a_ra + self.model_ha(params,a_ra,a_dec)),np.degrees(a_dec + self.model_dec(params,a_ra,a_dec)),np.degrees(r_ra),np.degrees(r_dec))

	def fit_model_az(self,params,a_az,r_az,a_el,r_el):
		return a_az - r_az + self.model_az(params,a_az,a_el)

	def fit_model_el(self,params,a_az,r_az,a_el,r_el):
		return a_el - r_el + self.model_el(params,a_az,a_el)

	def fit_model_altaz(self,params,a_az,r_az,a_el,r_el):
		if self.verbose:
			print 'computing', self.latitude, self.latitude_r, params, a_az, r_az, a_el, r_el
#		return np.concatenate((self.fit_model_ha(params,a_ra,r_ra,a_dec,r_dec),self.fit_model_dec(params,a_ra,r_ra,a_dec,r_dec)))
		return libnova.angular_separation(np.degrees(a_az + self.model_az(params,a_az,a_el)),np.degrees(a_el + self.model_el(params,a_az,a_el)),np.degrees(r_az),np.degrees(r_el))

	# open file, produce model
	# expected format:
	#  Observation	  MJD	   LST-MNT RA-MNT   DEC-MNT   AXRA	  AXDEC   RA-TRUE  DEC-TRUE
	## observatory <longitude> <latitude> <altitude>
	#02a57222e0002o 57222.260012 233.8937 275.7921  77.0452  -55497734  -46831997 276.0206  77.0643
	# or for alt-az
	#  Observation	  MJD	   LST-MNT   AZ-MNT   ALT-MNT   AXAZ	  AXALT   AZ-TRUE  ALT-TRUE
	## altaz <longitude> <latitude> <altitude>
	#02a57222e0002o 57222.260012 233.8937 275.7921  77.0452  -55497734  -46831997 276.0206  77.0643
	#skip first line, use what comes next. Make correction on DEC based on axis - if above zeropoint + 90 deg, flip DEC (DEC = 180 - DEC)
	def process_files(self,filenames,flips):
		obsmatch = re.compile('#\s*(\S*)\s+(\S*)\s+(\S*)\s+(\S*)\s*')

		frmt = "astrometry"

		rdata = []

		for filename in filenames:
			f = open(filename)
			# skip first line
			f.readline()
			line = f.readline()
			while not(line == ''):
				if line[0] == '#':
					m = obsmatch.match(line)
					if m:
						if m.group(1) in ['observatory','gem']:
							self.altaz = False
						elif m.group(1) in ['altaz']:
							self.altaz = True
						elif m.group(1) in ['altaz-manual']:
							self.altaz = True
							frmt = "manual"
						else:
							line = f.readline()
							continue

						if self.latitude is None:
							self.latitude=m.group(3)
						elif self.def_latitude is None and self.latitude!=m.group(2):
							sys.exit('Cannot (yet) perform calculation on two different latitudes: {0} {1}'.format(self.latitude,m.group(3)))
						# others are not yet used..will be interesting for refraction, if included in model
						if self.longitude is None:
							self.longitude=float(m.group(2))
						if self.altitude is None:
							self.altitude=float(m.group(4))
				else:
					s = line.split()
					self.indata.append(s)
					rdata.append(s[:9])

				line = f.readline()

			f.close()
		
		if self.verbose:
			print "Input data",rdata

		if self.latitude is None:
			sys.exit("You must specify latitude! Either through --latitude option, or in input file (on #observatory line).")

		self.latitude_r = np.radians(float(self.latitude))

		data = []

		if self.altaz:
			if frmt == "manual":
				data = [(float(a_az), float(a_alt), float(a_az) + float(e_az), float(a_alt) + float(e_alt), sn, float(mjd)) for sn,mjd,ra,dec,e_alt,e_az,a_alt,a_az in rdata]
			else:
				data = [(float(a_az), float(a_alt), float(r_az), float(r_alt), sn, float(mjd)) for sn,mjd,lst,a_az,a_alt,ax_az,ax_alt,r_az,r_alt in rdata]
		else:
			# data = [(float(lst) - flip_ra(float(a_ra),float(a_dec)), float(a_dec), float(lst) - float(r_ra), flip_dec(float(r_dec),float(a_dec)), sn, float(mjd)) for sn,mjd,lst,a_ra,a_dec,ax_ra,ax_dec,r_ra,r_dec in rdata]
			data = [(float(lst) - float(a_ra), float(a_dec), float(lst) - flip_ra(float(r_ra),float(a_dec)), flip_dec(float(r_dec),float(a_dec)), sn, float(mjd)) for sn,mjd,lst,a_ra,a_dec,ax_ra,ax_dec,r_ra,r_dec in rdata]
			if flips == 'east':
				data = [d for d in data if abs(d[1]) > 90]
			elif flips == 'west':
				data = [d for d in data if abs(d[1]) < 90]

		a_data = np.array(data)
		if self.verbose:
			print "Parsed data",a_data

		if self.altaz:
			self.aa_az = np.array(a_data[:,0],np.float)
			self.aa_alt = np.array(a_data[:,1],np.float)
			self.ar_az = np.array(a_data[:,2],np.float)
			self.ar_alt = np.array(a_data[:,3],np.float)
		else:	
			self.aa_ha = np.array(a_data[:,0],np.float)
			self.aa_dec = np.array(a_data[:,1],np.float)
			self.ar_ha = np.array(a_data[:,2],np.float)
			self.ar_dec = np.array(a_data[:,3],np.float)
	
		self.mjd = np.array(a_data[:,5],np.float)

		# prepare for X ticks positions
		last_mjd = 0
		last_mjd_hour = 0
		self.mjd_ticks = {}
		self.mjd_hours = {}

		for m in range(0,len(self.mjd)):
			jd = self.mjd[m]
			if last_mjd != round(jd):
				last_mjd = round(jd)
				self.mjd_ticks[m] = last_mjd
			if last_mjd_hour != round(jd * 24):
				last_mjd_hour = round(jd * 24)
				self.mjd_hours[m] = jd


		if self.altaz:
			self.rad_aa_az = np.radians(self.aa_az)
			self.rad_aa_alt = np.radians(self.aa_alt)
			self.rad_ar_az = np.radians(self.ar_az)
			self.rad_ar_alt = np.radians(self.ar_alt)

			# transform to ha/dec
			self.aa_ha,self.aa_dec = self.hrz_to_equ(self.rad_aa_az,self.rad_aa_alt)
			self.ar_ha,self.ar_dec = self.hrz_to_equ(self.rad_ar_az,self.rad_ar_alt)
		else:
			self.rad_aa_ha = np.radians(self.aa_ha)
			self.rad_aa_dec = np.radians(self.aa_dec)
			self.rad_ar_ha = np.radians(self.ar_ha)
			self.rad_ar_dec = np.radians(self.ar_dec)

			# transform to alt/az
			self.aa_alt,self.aa_az = self.equ_to_hrz(self.rad_aa_ha,self.rad_aa_dec)
			self.ar_alt,self.ar_az = self.equ_to_hrz(self.rad_ar_ha,self.rad_ar_dec)

		self.diff_ha = self.aa_ha - self.ar_ha
		self.diff_dec = self.aa_dec - self.ar_dec
		self.diff_angular = libnova.angular_separation(self.aa_ha,self.aa_dec,self.ar_ha,self.ar_dec)

		self.diff_alt = self.aa_alt - self.ar_alt
		self.diff_az = normalize_az_err(self.aa_az - self.ar_az)

	def fit(self):
		if self.altaz:
			par_init = np.array([0] * (9 + len(self.extra_az) + len(self.extra_el)))
			self.best,self.cov,self.info,self.message,self.ier = leastsq(self.fit_model_altaz,par_init,args=(self.rad_aa_az,self.rad_ar_az,self.rad_aa_alt,self.rad_ar_alt),full_output=True,maxfev=10000)
		else:
			par_init = np.array([0,0,0,0,0,0,0,0,0])
			self.best,self.cov,self.info,self.message,self.ier = leastsq(self.fit_model_gem,par_init,args=(self.rad_aa_ha,self.rad_ar_ha,self.rad_aa_dec,self.rad_ar_dec),full_output=True,maxfev=10000)

		if self.verbose:
			print self.message,self.ier,self.info

		if self.altaz:

			self.f_model_az = self.fit_model_az(self.best,self.rad_aa_az,self.rad_ar_az,self.rad_aa_alt,self.rad_ar_alt)
			self.f_model_alt = self.fit_model_el(self.best,self.rad_aa_az,self.rad_ar_az,self.rad_aa_alt,self.rad_ar_alt)

			self.diff_model_az = np.degrees(self.f_model_az)
			self.diff_model_alt = np.degrees(self.f_model_alt)

			self.am_ha,self.am_dec = self.hrz_to_equ(self.rad_ar_az - self.f_model_az,self.rad_ar_alt - self.f_model_alt)

			self.diff_model_ha = normalize_ha_err(self.am_ha - self.ar_ha)
			self.diff_model_dec = self.am_dec - self.ar_dec

			self.diff_model_angular = self.fit_model_altaz(self.best,self.rad_aa_az,self.rad_ar_az,self.rad_aa_alt,self.rad_ar_alt)
		else:
			# feed parameters to diff, obtain model differences. Closer to zero = better
			self.f_model_ha = self.fit_model_ha(self.best,self.rad_aa_ha,self.rad_ar_ha,self.rad_aa_dec,self.rad_ar_dec)
			self.f_model_dec = self.fit_model_dec(self.best,self.rad_aa_ha,self.rad_ar_ha,self.rad_aa_dec,self.rad_ar_dec)

			self.diff_model_ha = np.degrees(self.f_model_ha)
			self.diff_model_dec = np.degrees(self.f_model_dec)

			self.am_alt,self.am_az = self.equ_to_hrz(self.rad_ar_ha - self.f_model_ha,self.rad_ar_dec - self.f_model_dec)

			self.diff_model_alt = self.am_alt - self.ar_alt
			self.diff_model_az = normalize_az_err(self.am_az - self.ar_az)

			self.diff_model_angular = self.fit_model_gem(self.best,self.rad_aa_ha,self.rad_ar_ha,self.rad_aa_dec,self.rad_ar_dec)

		return self.best

	def print_params(self):
		print "Best fit",np.degrees(self.best)
		if self.verbose:
			print self.cov
			print self.info
			print self.message
			print self.ier

		if self.altaz:
			print 'Zero point in AZ (") {0}'.format(degrees(self.best[0])*60.0)
			print 'Zero point in ALT (") {0}'.format(degrees(self.best[4])*60.0)
	
		else:
			print 'Zero point in DEC (") {0}'.format(degrees(self.best[0])*60.0)
			print 'Zero point in RA (") {0}'.format(degrees(self.best[4])*60.0)
			i = sqrt(self.best[1]**2 + self.best[2]**2)
			print 'Angle between true and instrumental poles (") {0}'.format(degrees(i)*60.0)
			print self.best[1],i,self.best[1]/i,acos(self.best[1]/i),atan2(self.best[2],self.best[1])
			print 'Angle between line of pole and true meridian (deg) {0}'.format(degrees(atan2(self.best[2],self.best[1]))*60.0)
			print 'Telescope tube droop in HA and DEC (") {0}'.format(degrees(self.best[3])*60.0)
			print 'Angle between optical and telescope tube axes (") {0}'.format(degrees(self.best[5])*60.0)
			print 'Mechanical orthogonality of RA and DEC axes (") {0}'.format(degrees(self.best[6])*60.0)
			print 'Dec axis flexure (") {0}'.format(degrees(self.best[7])*60.0)
			print 'HA encoder scale error ("/degree) {0}'.format(degrees(self.best[8])*60.0)

			print 'DIFF_MODEL RA',' '.join(map(str,self.diff_model_ha*3600))
			print 'DIFF_MODEL DEC',' '.join(map(str,self.diff_model_dec*3600))
			print self.get_model_type(),' '.join(map(str,self.best))

	def get_model_type(self):
		if self.altaz:
			return 'RTS2_ALTAZ'
		else:
			return 'RTS2_GEM'

	def print_stat(self):
		# calculates root mean squeare of vector/array
		def RMS(vector):
			return np.sqrt(np.mean(np.square(vector)))

		def print_vect_stat(v):
			return 'MIN {0} MAX {1} MEAN {2} RMS {3}'.format(np.min(v),np.max(v),np.mean(v),RMS(v))

		if self.verbose:
			print 'DIFF_RA',print_vect_stat(self.diff_ha*3600)
			print 'DIFF_DEC',print_vect_stat(self.diff_dec*3600)
			print 'DIFF_ANGULAR',print_vect_stat(self.diff_angular*3600)

		print 'RMS RA DIFF',print_vect_stat(self.diff_ha*3600)
		print 'RMS RA CORRECTED DIFF',print_vect_stat(self.diff_ha*np.cos(self.aa_dec)*3600)
		print 'RMS DEC DIFF RMS',print_vect_stat(self.diff_dec*3600)
		print 'RMS AZ DIFF RMS',print_vect_stat(self.diff_az*3600)
		print 'RMS AZ CORRECTED DIFF RMS',print_vect_stat(self.diff_az*np.cos(self.aa_alt)*3600)
		print 'RMS ALT DIFF RMS',print_vect_stat(self.diff_alt*3600)
		print 'RMS ANGULAR SEP DIFF',print_vect_stat(self.diff_angular*3600)

		if self.best is not None:
			print 'RMS MODEL RA DIFF',print_vect_stat(self.diff_model_ha*3600)
			print 'RMS MODEL RA CORRECTED DIFF',print_vect_stat(self.diff_model_ha*np.cos(self.aa_dec)*3600)
			print 'RMS MODEL DEC DIFF',print_vect_stat(self.diff_model_dec*3600)
			print 'RMS MODEL AZ DIFF',print_vect_stat(self.diff_model_az*3600)
			print 'RMS MODEL AZ CORRECTED DIFF',print_vect_stat(self.diff_model_az*np.cos(self.aa_dec)*3600)
			print 'RMS MODEL ALT DIFF',print_vect_stat(self.diff_model_alt*3600)
			print 'RMS MODEL ANGULAR SEP DIFF',print_vect_stat(self.diff_model_angular*3600)

	# set X axis to MJD data
	def set_x_axis(self,plot):
		import pylab
		def mjd_formatter(x, pos):
			try:
				return self.mjd_ticks[int(x)]
			except KeyError,ke:
				try:
					return self.mjd[int(x)]
				except IndexError,ie:
					return x

		plot.xaxis.set_major_formatter(pylab.FuncFormatter(mjd_formatter))
		plot.set_xticks(self.mjd_ticks.keys())
		plot.set_xticks(self.mjd_hours.keys(),minor=True)
		plot.grid(which='major')
		return plot

	# set Y axis to arcsec distance
	def set_y_axis(self,plot):
		import pylab
		def arcmin_formatter(x, pos):
			return "{0}'".format(int(x / 60))
		ymin,ymax = plot.get_ylim()
		numticks = len(plot.get_yticks())
		mtscale = max(60,60 * int(abs(ymax - ymin) / numticks / 60))
		plot.set_yticks(np.arange(ymin - ymin % 60, ymax - ymax % 60, mtscale))
		plot.set_yticks(np.arange(ymin - ymin % 60, ymax - ymax % 60, mtscale / 6.0), minor=True)
		plot.yaxis.set_major_formatter(pylab.FuncFormatter(arcmin_formatter))
		return plot

	def plot_alt_az(self,grid,contour='',pfact=4):
		import pylab
		polar = pylab.subplot2grid(self.plotgrid,grid[:2],colspan=grid[2],rowspan=grid[3],projection='polar')
		polar.plot(np.radians(self.aa_az - 90),90 - self.aa_alt,'r.')
		polar.plot(np.radians(self.ar_az - 90),90 - self.ar_alt,'g.')
		polar.set_rmax(90)
		polar.set_xticklabels(['E','SE','S','SW','W','NW','N','NE'])
		if contour:
			X = np.radians(self.ar_az - 90)
			Y = 90 - self.ar_alt

			if contour == 'model':
				Z = self.diff_model_angular * 3600
				polar.set_title('Model differences')
			elif contour == 'real':
				Z = self.diff_angular * 3600
				polar.set_title('Real differences')

			xi = np.linspace(np.radians(-90),np.radians(271),num = 360 * pfact)
			yi = np.linspace(min(Y),max(Y),num = 90 * pfact)
			zi = pylab.griddata(X, Y, Z, xi, yi, interp='linear')
			ctf = polar.contourf(xi,yi,zi,cmap='hot')
			cbar = pylab.colorbar(ctf, orientation='horizontal', pad=0.05)
			cbar.set_ticks(range(0,int(max(Z)),60))
			cbar.ax.set_xticklabels(map("{0}'".format,range(0,int(max(Z) / 60))))
		else:
			polar.set_title('Alt-Az distribution')
                return polar

	def __get_data(self,name):
		if self.name_map is None:
			# maps name to data,plot style,label
			name_map = {
				'alt-err':[self.diff_alt*3600,'r.','Alt error'],
				'az-err':[self.diff_az*3600,'y.','Az error'],
				'dec-err':[self.diff_dec*3600,'b.','Dec error'],
				'ha-err':[self.diff_ha*3600,'g.','HA error'],
				'mjd':[self.mjd,'m','MJD'],
				'num':[range(len(self.mjd)),'m','Number'],
				'paz':[self.aa_az,'rx','Azimuth'],
				'az':[self.aa_az,'rx','Azimuth'],
				'alt':[self.aa_alt,'yx','Altitude'],
				'dec':[self.aa_dec,'bx','Dec'],
				'ha':[self.aa_ha,'gx','HA']
			}
			# append model output only if model was fitted
			if self.best is not None:
				name_map.update({
					'alt-merr':[self.diff_model_alt*3600,'r*','Alt model error'],
					'az-merr':[self.diff_model_az*3600,'y*','Az model error'],
					'dec-merr':[self.diff_model_dec*3600,'b*','Dec model error'],
					'ha-merr':[self.diff_model_ha*3600,'g*','HA model error'],
					'model-err':[self.diff_model_angular*3600,'c+','Model angular error'],
					'real-err':[self.diff_angular*3600,'c+','Real angular error']
				})
		return name_map[string.lower(name)]

	def plot_data(self,p,nx,ny,band):
		xdata = self.__get_data(nx)
		ydata = self.__get_data(ny)
		p.plot(xdata[0],ydata[0],ydata[1])
		p.set_xlabel(xdata[2])
		p.set_ylabel(ydata[2])
		if band is not None:
			import matplotlib.patches as patches
			band = float(band)
			p.add_patch(patches.Rectangle((min(xdata[0]), -band), max(xdata[0]) - min(xdata[0]), 2*band, alpha=0.7, facecolor='red', edgecolor='none'))
		return p

	def __gen_plot(self,plots,band):
		import pylab
		plot = []
		grid = []

		i = 0
		rows = 1
		cols = 1
		# find grid size
		for mg in plots.split(','):
			g = [i,0,1,1]
			try:
				pmg = mg.split('%')
				grids = pmg[1].split(':')
				g = map(int,grids) + g[len(grids):]
                                plot.append(pmg[0])
				grid.append(g)
			except IndexError,ie:
				plot.append(mg)
				grid.append(g)
			lg = grid[-1]
			rows = max(lg[0] + lg[2],rows)
			cols = max(lg[1] + lg[3],cols)
			i += 1

		self.plotgrid = (rows,cols)
		for i in range(0,len(plot)):
			axnam=plot[i].split(':')
			if len(axnam) < 2:
				sys.exit('invalid plot name - {0} does not contain at least one :'.format(plot[i]))
			g = grid[i]
			p = pylab.subplot2grid(self.plotgrid,g[:2],colspan=g[2],rowspan=g[3])
			if axnam[0] == 'paz':
				ax=axnam[1].split('-')
				if ax[0] == 'contour':
					self.plot_alt_az(g,ax[1])
				else:
					self.plot_alt_az(g)
			else:
				for j in axnam[1:]:
					self.plot_data(p,axnam[0],j,band)

	def plot(self,plots,band=None,ofile=None):
		import pylab
		self.__gen_plot(plots,band)
		pylab.tight_layout()
		if ofile is None:
			pylab.show()
		else:
			pylab.savefig(ofile)

	def plotoffsets(self,best,ha_start,ha_end,dec):
		import pylab

		ha_range = np.arange(ha_start,ha_end,0.02)
		ha_offsets = []
		dec_offsets = []
		for ha in ha_range:
			ha_offsets.append(self.model_ha(best,ha,dec))
			dec_offsets.append(self.model_dec(best,ha,dec))

		pylab.plot(ha_range,np.array(ha_offsets) * 3600.0,'b-',ha_range, np.array(dec_offsets) * 3600.0,'g-')
		pylab.show()

	def save(self,fn):
		"""Save model to file."""
		f = open(fn,'w+')
		f.write(self.get_model_type() + ' ')
		f.write(' '.join(map(lambda x:str(x),self.best[:self.extra_az_start)))
		bi = self.extra_az_start
		for e in self.extra_az:
			f.write('\n{0}\t{1}'.format(self.best[bi],e))
			bi += 1
		for e in self.extra_el:
			f.write('\n{0}\t{1}'.format(self.best[bi],e))
			bi += 1
		f.close()


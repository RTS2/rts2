# Tracking & Pointing Verification and Performance
#
# (C) 2016-2017 Petr Kubanek <petr@rts2.org>
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

from __future__ import print_function

import math
import sys
from . import altazpath
from . import brights
from . import libnova
from . import spiral
import rts2.rtsapi
from . import kmparse
import time
import os.path
from . import scat
import numpy

import gettext
gettext.install('rts2-build-model-tool')

def wait_for_key(t):
	"""Wait for key press for maximal t seconds."""
	from select import select
	while t > 0:
		print(_("Hit enter to interrupt the sequence within {0} seconds...\r").format(t), end=' ')
		sys.stdout.flush()
		rlist, wlist, xlist = select([sys.stdin], [], [], 1)
		if rlist:
			print(_("Interrupted. Now either find the bright stars typing alt-az offsets, or type c to continue"))
			sys.stdin.readline()
			return True
		t -= 1
	return False

class TPVP:
	def __init__(self,jsonProxy,camera,sleeptime=20,telescope=None,verbose=0,catalog='BSC'):
		# wide magnitude limit is the default
		self.__mag_max = -10
		self.__mag_min = 10
		self.j = jsonProxy
		self.fov_center = None
		self.__verifywcs_asecpix = None
		self.__verifywcs_rotang = None
		self.__verifywcs_flip = None
		self.camera = camera
		self.sleeptime = sleeptime
		self.verbose = verbose
		self.catalog = catalog
		if self.catalog == 'hip':
			self.hipparcos = rts2.scat.Hipparcos('/home/petr/hipparcos/hipparcos')
		if telescope is None:
			try:
				self.telescope = self.j.getDevicesByType(rts2.rtsapi.DEVICE_TYPE_MOUNT)[0]
			except IndexError as ie:
				raise Exception(_('cannot find any telescope'))
		else:
			self.telescope = telescope
		self.mount_type = self.j.getValue(self.telescope, 'MOUNT', True)
		print(_('Telescope type {0} (0-GEM, 1-AltAz, 2-AltAlt)'.format(self.mount_type)))
		if self.mount_type not in [0,1]:
			raise Exception(_('Unsuported mount type: {0}').format(self.mount_type))	

		print(_('using telescope {0} for pointing').format(self.telescope))

	def __stable(self,st):
		if st <= 0:
			return
		for sec in range(int(st),0,-1):
			print(_('waiting {0} seconds                                                                \r'.format(sec)), end=' ')
			sys.stdout.flush()
			time.sleep(1)
			self.j.refresh(self.telescope)
			if self.j.getState(self.telescope) & 0x01000620 == 0x00000020:
				print(_('stable telescope                                                                      '))
				return
		print(_('Telescope still unstable                        '))
		time.sleep(st - int(st))

	def zero_offsets(self):
		if self.mount_type == 0:
			self.j.setValue(self.telescope,'OFFS','0 0')
		elif self.mount_type == 1:
			self.j.setValue(self.telescope,'AZALOFFS','0 0')

	def set_mags(self,mmax,mmin):
		self.__mag_max = mmax
		self.__mag_min = mmin

	def set_wcs(self,fov_center,asecpix,rotang,flip):
		self.fov_center = fov_center
		self.__verifywcs_asecpix = asecpix
		self.__verifywcs_rotang = rotang
		self.__verifywcs_flip = flip

	def get_radec_line(self):
		self.j.refresh()
		jd = self.j.getValue(self.telescope,'JD')
		ori = self.j.getValue(self.telescope,'ORI')
		radec = self.j.getValue(self.telescope,'TEL')
		lst = self.j.getValue(self.telescope,'LST')
		offs = self.j.getValue(self.telescope,'OFFS')
		return '\t'.join(map(str,[jd,lst,ori['ra'],ori['dec'],radec['ra'],radec['dec'],ori['ra'] + offs['ra'],ori['dec'] + offs['dec']]))

	def get_altazm_line(self):
		self.j.refresh()
		jd = self.j.getValue(self.telescope,'JD')
		ori = self.j.getValue(self.telescope,'ORI')
		altaz = self.j.getValue(self.telescope,'TEL_')
		offs = self.j.getValue(self.telescope,'AZALOFFS')
		return '\t'.join(map(str,[jd,ori['ra'],ori['dec'],offs['alt'],offs['az'],altaz['alt'],altaz['az']]))

	def __run_spiral_gem(self,timeout,last_step=0,maxsteps=500):
		"""Runs spiral pointing to find the star."""
		s = spiral.Spiral(1,1)
		x = 0
		y = 0
		step_ra = 0.08
		step_dec = 0.08
		dec=self.j.getValue(self.telescope,"TEL",refresh_not_found=True)['dec']
		cosd = math.cos(math.radians(dec))
		step_ra /= cosd
		print(_('Scaling RA by factor {0:.10f} to {1:.2f}').format(cosd,step_ra))
		for i in range(maxsteps):
			a,e = s.get_next_step()
			x += a
			y += e
			if i < last_step:
				continue
			print(_('step {0} next {1} {2} radec {3:.3f} {4:.3f}').format(i,x,y,x*step_ra,y*step_dec))
			self.j.setValue(self.telescope, 'OFFS', '{0} {1}'.format(x*step_ra,y*step_dec))
			if wait_for_key(timeout):
				return i
		print(_('spiral ends..'))
		return i
	
	def __run_spiral_altaz(self,timeout,last_step=0,maxsteps=500):
		"""Runs spiral pointing to find the star."""
		s = spiral.Spiral(1,1)
		x = 0
		y = 0
		step_alt = 0.08
		step_az = 0.08
		alt=self.j.getValue(self.telescope,"TEL_",refresh_not_found=True)['alt']
		cosa = math.cos(math.radians(alt))
		step_az /= cosa
		print(_('Scaling azimuth by factor {0:.10f} to {1:.2f}').format(cosa,step_az))
		for i in range(maxsteps):
			a,e = s.get_next_step()
			x += a
			y += e
			if i < last_step:
				continue
			print(_('step {0} next {1} {2} altaz {3:.3f} {4:.3f}').format(i,x,y,x*step_alt,y*step_az))
			self.j.setValue(self.telescope, 'AZALOFFS', '{0} {1}'.format(x*step_alt,y*step_az))
			if wait_for_key(timeout):
				return i
		print(_('spiral ends..'))
		return i

	def run_spiral(self,timeout,last_step=0,maxsteps=500):
		if self.mount_type == 0:
			self.__run_spiral_gem(timeout,last_step,maxsteps)
		elif self.mount_type == 1:
			self.__run_spiral_altaz(timeout,last_step,maxsteps)
	
	def tel_hrz_to_equ(self,alt,az):
		self.j.refresh()
		lst = self.j.getValue(self.telescope,'LST')
		lat = self.j.getValue(self.telescope,'LATITUDE')
	
		ha,dec = libnova.hrz_to_equ(az,alt,lat)
		ra = (lst - ha) % 360.0
		return ra,dec
	
	def find_bright_star(self,alt,az,minalt=None):
		from . import bsc
		ra,dec = self.tel_hrz_to_equ(alt,az)
		lst = self.j.getValue(self.telescope,'LST')
		lat = self.j.getValue(self.telescope,'LATITUDE')
		print(_('Looking for star around RA {0:.3f} DEC {1:.2f} (LST {2:.3f}), magnitude {3:.2f} to {4:.2f}').format(ra,dec,lst,self.__mag_max,self.__mag_min))
		# find bsc..
		bsc = None
		if self.catalog == 'hip':
			bsc=self.hipparcos.search_catalogue(ra, dec, 10,self.__mag_max, self.__mag_min)
			if len(bsc) == 0:
				return None
			print(_('Found HIP #{0} at RA {1:.3f} DEC {2:.2f} mag {3:.2f} proper motion RA {4:.3f} Dec {5:.3f} arcsec/year').format(bsc[0].xno,numpy.degrees(bsc[0].sra0),numpy.degrees(bsc[0].sdec0),bsc[0].mag,numpy.degrees(bsc[0].xrpm),numpy.degrees(bsc[0].xdpm)))
		else:
			bsc=bsc.find_nearest(ra,dec,self.__mag_max,self.__mag_min,lst,lat,minalt)
			print(_('Found BSC #{0} at RA {1:.3f} DEC {2:.2f} mag {3:.2f} proper motion RA {4:.3f} Dec {5:.3f} arcsec/year').format(bsc[0],bsc[1],bsc[2],bsc[3],bsc[7],bsc[8]))
		return bsc
	
	def __check_model_firstline(self,modelname):
		oa = open(modelname,'a')
		if oa.tell() == 0:
			self.j.refresh()
			oa.write('# model file created on {0}\n'.format(time.strftime('%c')))
			lat = self.j.getValue(self.telescope,'LATITUDE')
			lng = self.j.getValue(self.telescope,'LONGITUD')
			alt = self.j.getValue(self.telescope,'ALTITUDE')
			if self.mount_type == 0:
				oa.write('# gem {0} {1} {2}\n'.format(lng,lat,alt))
			elif self.mount_type == 1:
				oa.write('# altaz-manual {0} {1} {2}\n'.format(lng,lat,alt))
			oa.flush()
		oa.close()
	
	def __save_modeline(self,modelname,mn):
		if self.mount_type == 0:
			modline = self.get_radec_line()
		elif self.mount_type == 1:
			modline = self.get_altazm_line()
		if modelname is None:
			print(_('model line {0}').format(modline))
			return
		print(_('adding to aling file {0}').format(modline))
		oa = open(modelname, 'a')
		oa.write('{0}\t{1}\n'.format(mn,modline))
		oa.flush()
		oa.close()

	def __get_mn(self,mn):
		if mn is None:
			for mn in range(1,999):
				if os.path.isfile('model_{0:03}.fits'.format(mn)) == False:
					break
			print(_('run #{0}').format(mn))
		return mn

	def __run(self,tarf_ra,tarf_dec,pm_ra,pm_dec,timeout,modelname,maxspiral,imagescript,mn,useDS9):
		import ds9
		d = None
		if useDS9:
			d = ds9.ds9('Model')
	
		self.j.executeCommand(self.telescope, _('move_pm {0} {1} {2} {3}').format(tarf_ra,tarf_dec,pm_ra,pm_dec))
		time.sleep(2)
		self.j.refresh(self.telescope)
		tmout = 120
	
		tel=self.j.getValue(self.telescope,'TEL',True)
		hrz=self.j.getValue(self.telescope,'TEL_')
		while tmout > 0 and self.j.getState(self.telescope) & 0x01000020 == 0x01000000:
			self.j.refresh(self.telescope)
			tel=self.j.getValue(self.telescope,'TEL')
			hrz=self.j.getValue(self.telescope,'TEL_')
			print(_('moving to {0:.4f} {1:.4f}...at {2:.4f} {3:.4f} HRZ {4:.4f} {5:.4f}\r').format(tarf_ra,tarf_dec,tel['ra'],tel['dec'],hrz['alt'],hrz['az']), end=' ')
			sys.stdout.flush()
			time.sleep(1)
			tmout -= 1
	
		if tmout <= 0:
			print(_('destination not reached, continue with new target                         '))
			return None,None

		self.__stable(self.sleeptime)
	
		print(_('moved to {0:.4f} {1:.4f}...at {2:.4f} {3:.4f} HRZ {4:.4f} {5:.4f}                      ').format(tarf_ra,tarf_dec,tel['ra'],tel['dec'],hrz['alt'],hrz['az']))
		if imagescript is not None:
			print(_('taking script {0}').format(imagescript))
			fn = 'model_{0:03}.fits'.format(mn)
			os.system("rts2-scriptexec --reset -d {0} -s '{1}' -e '{2}'".format(self.camera,imagescript,fn))
			if useDS9 is not None:
				d = ds9.ds9('Model')
				d.set('file {0}'.format(fn))
			return fn,mn
		print(_('Slew finished, starting search now'))
		next_please = False
		last_step = 0
		while next_please == False:
			skip_spiral = False
			if last_step <= 0:
				self.zero_offsets()
				skip_spiral = wait_for_key(7)
				last_step = 0
			if skip_spiral == False:
				last_step = self.run_spiral(timeout,last_step,maxspiral)
			while True:
				print(_('Now either type offsets (commulative, arcmin), m when star is centered, r to repeat from 0 steps, z to zero offsets, or s to skip this field and hit enter'))
				ans = sys.stdin.readline().rstrip()
				if ans == '':
					continue
				elif ans == 'm':
					self.__check_model_firstline(modelname)
					self.__save_modeline(modelname,mn)
					mn += 1
					next_please = True
					break
				elif ans == 's':
					next_please = True
					print(_('skipping this field, going to the next target'))
					break
				elif ans[0] == 'r':
					ls = ans.split()
					try:
						if len(ls) == 2:
							last_step -= int(ls[1])
						else:
							last_step = 0
						break
						print(_('going back to step {0}').format(last_step))
					except Exception as ex:
						print(_('invalid r command format: {0}').format(ans))
						continue
				elif ans == 'c':
					print(_('continuing..'))
					last_step -= 1
					break
				elif ans == 'z':
					print(_('zeroing offsets'))
					self.zero_offsets()
					break
				try:
					azo,alto = ans.split()
					if self.mount_type == 0:
						print(_('offseting RA {0} DEC {1} arcmin').format(azo,alto))
						self.j.executeCommand(self.telescope,'X AZALOFFS += {0} {1}'.format(float(azo)/60.0,float(alto)/60.0))
					elif self.mount_type == 1:
						print(_('offseting AZ {0} ALT {1} arcmin').format(azo,alto))
						self.j.executeCommand(self.telescope,'X AZALOFFS += {0} {1}'.format(float(azo)/60.0,float(alto)/60.0))
	
				except Exception as ex:
					print(_('unknow command {0}, please try again').format(ans))
		return None,None
	
	def run_manual_altaz(self,alt,az,timeout,modelname,maxspiral,imagescript,mn,useDS9,minalt):
		s = None

		mn = self.__get_mn(mn)

		if maxspiral >= -1:
			print(_('Next model point at altitude {0:.3g} azimuth {1:.3g}').format(alt,az))
			s = self.find_bright_star(alt,az,minalt)
			if self.catalog == 'hip':
				tarf_ra = numpy.degrees(s[0].sra0)
				tarf_dec = numpy.degrees(s[0].sdec0)
				pm_ra = numpy.degrees(s[0].xrpm)
				pm_dec = numpy.degrees(s[0].xdpm)
			else:
				tarf_ra = s[1]
				tarf_dec = s[2]
				pm_ra = s[7]
				pm_dec = s[8]
		else:
			tarf_ra,tarf_dec = self.tel_hrz_to_equ(alt,az)
			pm_ra = 0
			pm_dec = 0

		fn,mn=self.__run(tarf_ra,tarf_dec,pm_ra,pm_dec,timeout,modelname,maxspiral,imagescript,mn,useDS9)
		return fn,mn,s

	def run_manual_bsc(self,bscnum,timeout,modelname,maxspiral,imagescript,mn,useDS9):
		mn = self.__get_mn(mn)

		from . import bsc
		s = bsc.get_star(bscnum)
		tarf_ra = s[1]
		tarf_dec = s[2]
		pm_ra = s[7]
		pm_dec = s[8]

		fn,mn=self.__run(tarf_ra,tarf_dec,pm_ra,pm_dec,timeout,modelname,maxspiral,imagescript,mn,useDS9)
		return fn,mn,s

	def __get_offset_by_image(self,fn,useDS9,mn,center):
		return brights.add_wcs(fn, self.__verifywcs_asecpix, self.__verifywcs_rotang, self.__verifywcs_flip, self.verbose, False, useDS9, 'wcs_{0:03}.fits'.format(mn), center=center)
	
	def __verify(self,mn,timeout,imagescript,useDS9,maxverify,verifyradius,minflux):
		from astropy.io import fits
		flux_history = []
		flux_ratio_history = []
		history_x = []
		history_y = []
		history_alt = []
		history_az = []
		for vn in range(maxverify):
			self.__stable(self.sleeptime)
			# verify ve really center on star
			vfn = 'verify_{0:03}_{1:02}.fits'.format(mn,vn)
			print(_('taking verify exposure {0} # {1}         ').format(vfn,vn))
			if os.path.isfile(vfn):
				print(_('removing {0}').format(vfn))
				os.unlink(vfn)
			os.system("rts2-scriptexec --reset -d {0} -s '{1}' -e '{2}'".format(self.camera,imagescript,vfn))
			vhdu = fits.open(vfn)
			b_x,b_y,b_flux,b_flux_ratio = brights.find_brightest(vfn, vhdu, 1, useDS9)
			if self.fov_center is None:
				off_x = vhdu[0].header['NAXIS1'] / 2.0 - b_x
				off_y = vhdu[0].header['NAXIS2'] / 2.0 - b_y
			else:
				off_x = self.fov_center[0] - b_x
				off_y = self.fov_center[1] - b_y
			pixdist = math.sqrt(off_x ** 2 + off_y ** 2)
			print(_('brightest X {0:.2f} Y {1:.2f} offset from center {2:.2f} {3:.2f} distance {4:.2f} flux {5:g} {6:.2f}').format(b_x, b_y, off_x, off_y, pixdist, b_flux, b_flux_ratio))
			if minflux is not None and b_flux < minflux:
				print(_('brightest star too faint, its flux is {0}, should be at least {1}').format(b_flux, minflux))
				return False,flux_history,flux_ratio_history,history_x,history_y,history_alt,history_alt
	
			flux_history.append(b_flux)
			flux_ratio_history.append(b_flux_ratio)
			history_x.append(off_x)
			history_y.append(off_y)
			# calculate offsets in alt-az, increment offsets
			off_radec,off_azalt,flux,flux_ratio,first_xy = self.__get_offset_by_image(vfn,useDS9,mn,self.fov_center)
			if off_radec is None:
				return False,flux_history,flux_ratio_history,history_x,history_y,history_alt,history_az
			print(_('Brightest flux {0:.2f}').format(flux))
			history_alt.append(off_azalt[1])
			history_az.append(off_azalt[0])
			if pixdist < verifyradius:
				print(_('converged'))
				return True,flux_history,flux_ratio_history,history_x,history_y,history_alt,history_az
			if self.mount_type == 0:
				print(_('Incrementing offset by RA {0:.3f} DEC {1:.3f} arcsec').format(off_radec[0] * 3600, off_radec[1] * 3600))
				self.j.incValue(self.telescope,'OFFS','{0} {1}'.format(off_radec[0], off_radec[1]))
			elif self.mount_type == 1:
				print(_('Incrementing offset by alt {0:.3f} az {1:.3f} arcsec').format(off_azalt[1] * 3600, off_azalt[0] * 3600))
				self.j.incValue(self.telescope,'AZALOFFS','{0} {1}'.format(off_azalt[1], off_azalt[0]))
	
		return False,flux_history,flux_ratio_history,history_x,history_y,history_alt,history_az

	def run_verify_brigths(self,timeout,path,modelname,imagescript,useDS9,maxverify,verifyradius,maxspiral,minflux,minalt):
		selr.zero_offsets()
		for p in path:
			if type(p) == int:
				fn,mn,bsc = self.run_manual_bsc(p,timeout,None,-1,imagescript,None,useDS9)
			else:
				fn,mn,bsc = self.run_manual_altaz(p[0],p[1],timeout,None,-1,imagescript,None,useDS9,minalt)

			off_radec,off_azalt,flux,flux_ratio,first_xy = self.__get_offset_by_image(fn,useDS9,mn,self.fov_center)
			if minflux is not None and (flux is None or flux < minflux) and maxspiral >= -1:
				print(_('Bright star not found on the first image, running spiral search'))
				last_step = 1
				while last_step < maxspiral and (flux is None or flux < minflux):
					self.run_spiral(timeout,last_step,last_step+1)
					self.__stable(self.sleeptime)
					print(_('taking script {0}').format(imagescript))
					fn = 'spiral_{0:03}_{1:03}.fits'.format(mn,last_step)
					os.system("rts2-scriptexec --reset -d {0} -s '{1}' -e '{2}'".format(self.camera,imagescript,fn))
					if useDS9:
						import ds9
						d = ds9.ds9('Model')
						d.set('file {0}'.format(fn))
					off_radec,off_azalt,flux,flux_ratio,first_xy = self.__get_offset_by_image(fn,useDS9,mn,self.fov_center)
					print(_('Brightest in {0} flux {1:.1f}').format(fn,flux))
					last_step += 1
	
			if off_radec is None or flux is None or (minflux is not None and flux < minflux):
				print(_('Bright star not found - continue'))
				if modelname is not None:
					modelf = open(modelname,'a')
					if type(p) == int:
						modelf.write('# bright star not found on image - continue, target BS RA {0:.2f} DEC {1:.2f} mag {2:.2f}\n'.format(bsc[1],bsc[2],bsc[3]))
					else:
						modelf.write('# bright star not found on image - continue, target ALT {0:.2f} AZ {1:.2f} BS RA {2:.2f} DEC {3:.2f} mag {4:.2f}\n'.format(p[0],p[1],bsc[1],bsc[2],bsc[3]))
					modelf.close()
				continue

			if self.mount_type == 0:
				print(_('Will offset by RA {0:.3f} DEC {1:.3f} arcsec').format(off_radec[0] * 3600, off_radec[1] * 3600))
				self.j.incValue(self.telescope,'OFFS','{0} {1}'.format(off_radec[0], off_radec[1]))
			elif self.mount_type == 1:
				print(_('Will offset by alt {0:.3f} az {1:.3f} arcsec').format(off_azalt[1] * 3600, off_azalt[0] * 3600))
				self.j.incValue(self.telescope,'AZALOFFS','{0} {1}'.format(off_azalt[1], off_azalt[0]))
			ver,flux_history,flux_ratio_history,history_x,history_y,history_alt,history_az = self.__verify(mn,timeout,imagescript,useDS9,maxverify,verifyradius,minflux)
			if modelname is not None:
				self.__check_model_firstline(modelname)
				modelf = open(modelname,'a')
				if type(p) == int:
					modelf.write('# BSC #{0} RA {1:.5f} DEC {2:.5f} mag {3} flux history {4} flux ratio history {5}\n'.format(p,bsc[1],bsc[2],bsc[3],','.join(map(str,flux_history)),','.join(map(str,flux_ratio_history))))
				else:
					modelf.write('# BSC #{0} alt {1:.3f} az {2:.3f} mag {3} flux history {4} flux ratio history {5}\n'.format(bsc[0],p[0],p[1],bsc[3],','.join(map(str,flux_history)),','.join(map(str,flux_ratio_history))))
				modelf.write('# f_alt {0:.3f}" f_az {1:.3f}" fx {2} fy {3} x {4} y {5} alt {6} az {7} \n'.format(off_azalt[1] * 3600,off_azalt[0] * 3600,first_xy[0],first_xy[1],','.join(map(str,history_x)),','.join(map(str,history_y)),','.join(['{0:.3f}"'.format(x*3600) for x in history_alt]),','.join(['{0:.3f}"'.format(x*3600) for x in history_az])))
				# comment lines
				if ver == False and maxverify > 0:
					modelf.write('# ')
				modelf.close()
			self.__save_modeline(modelname,mn)

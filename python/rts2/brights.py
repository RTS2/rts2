# Bright stars handling.
#
# (C) 2016 Petr Kubanek <petr@kubanek.net>
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

import sep
import os
import numpy as np
from astropy import wcs
from astropy.io import fits
import libnova

__DS9 = 'brights'

def find_stars_on_data(data, verbose = 0, useDS9 = False):
	bkg = sep.Background(data)
	bkg.subfrom(data)
	thres = 1.5 * bkg.globalrms
	if verbose > 1:
		print 'global average background: {0:.2f} rms: {1:.3f} threshold: {2:.3f}'.format(bkg.globalback, bkg.globalrms, thres)
	objects = sep.extract(data, thres)
	# order by flux
	if len(objects) == 0:
		return []
	return sorted(objects, cmp=lambda x,y: cmp(y['flux'],x['flux']))

def find_stars(fn, hdu = None, verbose = 0, useDS9 = False, cube = None):
	"""Find stars on the image. Returns flux ordered list of stars."""
	if hdu is None:
		hdu = fits.open(fn)
	if cube is None:
		data = np.array(hdu[0].data,np.int32)
	else:
		data = np.array(hdu[0].data[cube],np.int32)

	return find_stars_on_data(data, verbose, useDS9)

def get_brightest(s_objects, verbose = 0, useDS9 = False, exclusion = None):
	if len(s_objects) == 0:
		return None, None, None, None
	b_x = s_objects[0]['x']
	b_y = s_objects[0]['y']
	b_flux = s_objects[0]['flux']
	if verbose:
		print 'detected {0} objects'.format(len(s_objects))

	if exclusion is not None:
		def dist(x1,y1,x2,y2):
			return np.sqrt((x1 - x2) ** 2 + (y1 - y2) ** 2)

		found = False

		for i in range(0, len(s_objects) / 2):
			b_x = s_objects[i]['x']
			b_y = s_objects[i]['y']
			b_flux = s_objects[i]['flux']
			for j in range(i, len(s_objects)):
				if i != j:
					s_x = s_objects[j]['x']
					s_y = s_objects[j]['y']
					if dist(s_x,s_y,b_x,b_y) < exclusion:
						if verbose:
							print 'rejecting star at {0} {1}, too close to {2} {3}'.format(b_x, b_y, s_x, s_y)
							break
			if j == len(s_objects) - 1:
				found = True
				break

		if found == False:
			return None, None, None, None

	if verbose:
		print 'brightest at {0:.2f} {1:.2f}'.format(b_x,b_y)
		if verbose > 1:
			for o in s_objects:
				print 'object {0}'.format(o)

	bb_flux = b_flux
	if len(s_objects) > 1:
		bb_flux = s_objects[1]['flux']
	if useDS9:
		import ds9
		d=ds9.ds9(__DS9)
		d.set('file {0}'.format(fn))
		d.set('regions','image; point({0},{1}) # point=cross 25, color=green'.format(b_x,b_y))
		if verbose > 1:
			w_flux = s_objects[-1]['flux']
			for o in s_objects[1:]:
				w = 1 + (o['flux'] - w_flux) / (bb_flux - w_flux)
				w = np.log(w)
				w = 20 * w
				w = w if w > 1 else 1
				d.set('regions','image; point({0},{1}) # point=cross {2},color=green'.format(o['x'],o['y'],int(w)))
	return b_x,b_y,b_flux,b_flux / bb_flux

def find_brightest_on_data(data, verbose = 0, useDS9 = False, exclusion = None):
	s_objects = find_stars_on_data(data, verbose, useDS9)
	return get_brightest(s_objects, verbose, useDS9, exclusion)

def find_brightest(fn, hdu = None, verbose = 0, useDS9 = False, cube = None):
	"""Find brightest star on the image. Returns tuple of X,Y,flux and ratio of the flux to the second brightest star."""
	s_objects = find_stars(fn, hdu, verbose, useDS9, cube)
	return get_brightest(s_objects, verbose, useDS9)

def add_wcs(fn, asecpix, rotang, flip = '', verbose = 0, dss = False, useDS9 = False, outfn='out.fits', save_regions=None, center=None):
	"""Add WCS solution to the image."""
	import ds9
	d = None
	if useDS9 and (verbose or dss):
		d = ds9.ds9(__DS9)
		d.set('frame delete all')
		d.set('frame new')

	hdu = fits.open(fn)
	x,y,flux,flux_ratio = find_brightest(fn, hdu, verbose, useDS9)
	if x is None:
		return None,None,None,None,None
	b_ra = hdu[0].header['OBJRA']
	b_dec = hdu[0].header['OBJDEC']

	paoff = 0

	try:
		paoff = hdu[0].header['DER1.PA']
	except KeyError,ke:
		print 'cannot find DER1.PA, using defaults'

	if dss:
		d.set('dss size 7 7')
		d.set('dss coord {0} {1} degrees'.format(b_ra, b_dec))

	w = wcs.WCS(naxis=2)
	w.wcs.crpix = [x,y]
	w.wcs.crval = [b_ra,b_dec]

	rt_rad = np.radians(rotang + paoff)
	rt_cos = np.cos(rt_rad)
	rt_sin = np.sin(rt_rad)

	flip_x = -1 if flip.find('X') >= 0 else 1
	flip_y = -1 if flip.find('Y') >= 0 else 1

	w.wcs.cd = (asecpix / 3600.0) * np.array([[flip_x * rt_cos,flip_x * rt_sin], [flip_y * -rt_sin,flip_y * rt_cos]], np.float)
	w.wcs.ctype = ['RA---TAN', 'DEC--TAN']

	if verbose > 1:
		pixcrd = np.array([[x, y], [x + 1, y + 1], [0,0]], np.float)
		print 'some pixels', w.all_pix2world(pixcrd, 1)

	wh = w.to_header()
	for h in wh.items():
		hdu[0].header.append((h[0], h[1], wh.comments[h[0]]))
	hdu.writeto('out.fits', clobber=True)

	if d is not None:
		d.set('frame clear 1')
		d.set('frame 1')
		d.set('file out.fits')
		if dss:
			d.set('frame 2')
			d.set('zoom to fit')
			d.set('match frame wcs')
			d.set('lock frame wcs')
			d.set('frame 1')
		else:
			d.set('zoom to fit')
		d.set('scale zscale')
		d.set('catalog GSC')
		if save_regions:
			d.set('catalog regions')
			d.set('regions select all')
			if os.path.isfile(save_regions):
				os.unlink(save_regions)
			d.set('regions export tsv {0}'.format(save_regions))

	# calculate offsets..ra dec, and alt az
	if center is None:
		offsp = np.array([[x,y], [hdu[0].header['NAXIS1'] / 2.0, hdu[0].header['NAXIS2'] / 2.0]], np.float)
	else:
		offsp = np.array([[x,y], [center[0], center[1]]], np.float)

	radecoff = w.all_pix2world(offsp, 1)

	off_ra = (radecoff[1][0] - radecoff[0][0] + 360.0) % 360.0
	if off_ra > 180.0:
		off_ra -= 360.0
	off_dec = radecoff[1][1] - radecoff[0][1]

	lst = hdu[0].header['LST']
	lat = hdu[0].header['LATITUDE']

	racs = np.array([radecoff[0][0], radecoff[1][0]], np.float)
	decs = np.array([radecoff[0][1], radecoff[1][1]], np.float)
	
	alt,az = libnova.equ_to_hrz(racs,decs,lst,lat)
	off_alt = alt[1] - alt[0]
	off_az = (az[1] - az[0] + 360.0) % 360.0
	if off_az > 180.0:
		off_az -= 360.0

	return (off_ra,off_dec),(off_az,off_alt),flux,flux_ratio,(offsp[1][0] - offsp[0][0],offsp[1][1] - offsp[1][0])

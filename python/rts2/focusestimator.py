#!/usr/bin/env python
#
# Class for various focus quality estimators to be used in autofocusing.
#
# The following python packages are necessary: numpy scipy astropy sep
#
# (C) 2018      Sergey Karpov
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

import posixpath, glob

import numpy as np
from astropy.io import fits
import sep

from scipy.interpolate import UnivariateSpline

# Cropping of overscans if any
def parse_det(string):
    x0,x1,y0,y1 = [int(_)-1 for _ in sum([_.split(':') for _ in string[1:-1].split(',')], [])]

    return x0,x1,y0,y1

def crop_overscans(image, header):
	if not header.get('DATASEC'):
		return image

	x1,x2,y1,y2 = parse_det(header.get('DATASEC'))

	return np.ascontiguousarray(image[y1:y2+1, x1:x2+1])

# Smoothing kernel to improve the detection of objects
def make_kernel(r0=1.0):
	x,y = np.mgrid[np.floor(-3.0*r0):np.ceil(3.0*r0+1), np.floor(-3.0*r0):np.ceil(3.0*r0+1)]
	r = np.hypot(x,y)
	image = np.exp(-r**2/2/r0**2)

	return image

# Simple SEP-based objects detection tailored towards FWHM/HFD estimation
def get_objects_fwhm(image, mask=None, r0=0.5, thresh=2.0, minnthresh=2, minarea=5, relfluxradius=3.0):
	if mask is None:
		mask = np.zeros_like(image, dtype=np.bool)

	kernel = make_kernel(r0)

	bg = sep.Background(image, mask=mask, bw=64, bh=64)
	image1 = image - bg.back()

	obj0 = sep.extract(image1, err=bg.rms(), thresh=thresh, minarea=minarea, mask=mask, filter_kernel=kernel, deblend_cont=1.0)

	fwhm0 = 2.0*np.sqrt(np.hypot(obj0['a'], obj0['b'])*np.log(2))
	fwhm = np.median(fwhm0)

	edge = 3.0*fwhm
	xwin,ywin = obj0['x'], obj0['y']

	idx = (np.round(xwin) > edge) & (np.round(ywin) > edge) & (np.round(xwin) < image.shape[1]-edge) & (np.round(ywin) < image.shape[0]-edge)
	idx &= (obj0['tnpix'] >= minnthresh)

	r0 = sep.flux_radius(image1, xwin[idx], ywin[idx], relfluxradius*fwhm*np.ones_like(xwin[idx]), 0.5, mask=mask)[0]

	return {'x':xwin[idx], 'y':ywin[idx], 'flags':obj0['flag'][idx], 'fwhm':fwhm0[idx], 'hfd':2.0*r0}

###

class FocusEstimator:
	"""Processing sequences of focusing data to derive optimal focus position"""
	def __init__(self, files=None, verbose=True, print_fn=print):
		self._verbose = verbose
		self._print_fn = print_fn

		self.init()

		if files:
			self.processFiles(files)

	def init(self):
		self.images = []
		self.headers = []
		self.filenames = []
		self.focpos = []

		self.valFWHM = []
		self.valNumStars = []
		self.valStd = []
		self.valFFT = []

		self.shouldRefine = False
		self.fitFailed = False
		self.bestPos = None

	def log(self, *args):
		if self._verbose:
			if self._print_fn:
				self._print_fn(*args)
			else:
				print(*args)

	def processDir(self, dirname, mask='*.fits', ccdname=None, verbose=False):
		"""Process all FITS files from given dir"""
		files = sorted(glob.glob(posixpath.join(dirname, mask)))

		self.processFiles(files, ccdname=ccdname, verbose=verbose)

	def processFiles(self, files, extnum=-1, focpos_key='FOC_POS', ccdname=None, verbose=False):
		"""
		Process list of files and estimate focus quality for them.
		Focus position in files has to be monotonously increasing.
		"""

		self.init()

		for filename in files:
			if not posixpath.exists(filename):
				continue

			image = fits.getdata(filename, extnum).astype(np.double)
			header = fits.getheader(filename, extnum)

			if header.get(focpos_key) is None:
				continue

			if ccdname is not None and header.get('CCD_NAME') != ccdname:
				continue

			image = crop_overscans(image, header)

			pos = header.get(focpos_key)

			if len(self.focpos) > 0 and self.focpos[-1] > pos:
				# End of monotonous interval
				break

			# Estimators based on detected stars
			obj = get_objects_fwhm(image)

			value_FWHM = np.median(obj['hfd'])
			value_NumStars = len(obj['x'])

			# Standard deviation
			value_Std = np.std(image)

			# Estimator based on FFT
			img_fft = abs(np.fft.fftshift(np.fft.fft2(image)[1:-1,1:-1]))
			cmask = np.zeros_like(img_fft, dtype=np.bool)
			height,width = img_fft.shape
			cmask[int(0.4*height):int(0.6*height),int(0.4*width):int(0.6*width)] = True

			value_FFT = np.sum(img_fft)
			# value_FFT = 1.0 * np.mean(img_fft[cmask])

			if verbose:
				self.log('%d: %g %g %g %g' % (pos, value_FWHM, value_NumStars, value_Std, value_FFT))

			self.images.append(image)
			self.headers.append(dict(header))
			self.filenames.append(filename)
			self.focpos.append(pos)

			self.valFWHM.append(value_FWHM)
			self.valNumStars.append(value_NumStars)
			self.valStd.append(value_Std)
			self.valFFT.append(value_FFT)

		self.log('Acquired %d focusing points from %d files' % (len(self.focpos), len(files)))

		# Convert everything to NumPy arrays for convenience
		self.images, self.headers, self.filenames, self.focpos, self.valFWHM, self.valNumStars, self.valStd, self.valFFT = [np.array(_) for _ in self.images, self.headers, self.filenames, self.focpos, self.valFWHM, self.valNumStars, self.valStd, self.valFFT]

	def getBestFocus(self, estimator='nstars', fitter='spline'):
		"""
		Analyze the focus quality estimations and get the best value.
		"""
		if not len(self.focpos):
			self.log('Not enough points for focus estimation')
			return None

		self.shouldRefine = False
		self.fitFailed = False

		bestPos = None

		if estimator == 'fwhm':
			val = -self.valFWHM
		elif estimator == 'nstars':
			val = self.valNumStars
		elif estimator == 'std':
			val = self.valStd
		else:
			val = self.valFFT

		self.estimator = estimator
		self.fitter = fitter

		# Peak position in raw data
		midx = np.where(val == np.max(val))[0][0]

		# Handle peak at the range ends
		if midx < 2:
			self.log('Maximum on left edge - can\'t estimate best focus')
			self.shouldRefine = True
			bestPos = self.focpos[midx]

		elif midx >= len(self.focpos) - 2:
			self.log('Maximum on right edge - can\'t estimate best focus')
			self.shouldRefine = True
			bestPos = self.focpos[midx]

		else:
			# Smoothing spline maximum
			spl = UnivariateSpline(self.focpos, (1.0*val - np.min(val))/(np.max(val) - np.min(val)), s=0.01)
			x = np.linspace(np.min(self.focpos), np.max(self.focpos), 500)
			y = spl(x)

			midx = np.where(y == np.max(y))[0]
			bestPos = np.mean(x[midx])

			self.log('Best position from spline fitting', bestPos)

		self.bestPos = bestPos
		return bestPos

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
from scipy.spatial import cKDTree

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
def get_objects_fwhm(image, mask=None, r0=0.5, thresh=2.0, minnthresh=2, minarea=5, relfluxradius=3.0, return_image=False):
	if mask is None:
		mask = np.zeros_like(image, dtype=np.bool)

	bg = sep.Background(image, mask=mask, bw=64, bh=64)
	image1 = image - bg.back()

	for iter in [0,1]:
		kernel = make_kernel(r0)

		obj0 = sep.extract(image1, err=bg.rms(), thresh=thresh, minarea=minarea, mask=mask, filter_kernel=kernel, deblend_cont=1.0)

		fwhm0 = 2.0*np.sqrt(np.hypot(obj0['a'], obj0['b'])*np.log(2))
		fwhm = np.median(fwhm0)

		edge = 3.0*fwhm
		xwin,ywin = obj0['x'], obj0['y']

		idx = (np.round(xwin) > edge) & (np.round(ywin) > edge) & (np.round(xwin) < image.shape[1]-edge) & (np.round(ywin) < image.shape[0]-edge)
		idx &= (obj0['tnpix'] >= minnthresh)

		flux_r0 = sep.flux_radius(image1, xwin[idx], ywin[idx], relfluxradius*fwhm*np.ones_like(xwin[idx]), 0.5, mask=mask)[0]

		if np.median(flux_r0) > 0.5*r0:
			r0 = min(10.0, 0.5*np.median(flux_r0))

			minnthresh = max(minnthresh, 0.5*np.pi*r0**2)
			# print('Increasing smoothing kernel to', r0, '(', np.median(flux_r0), ')')
		else:
			break

	result = {'x':xwin[idx], 'y':ywin[idx], 'flags':obj0['flag'][idx], 'fwhm':fwhm0[idx], 'hfd':2.0*flux_r0}

	if return_image:
		return result, image1
	else:
		return result
###

class FocusEstimator:
	"""Processing sequences of focusing data to derive optimal focus position"""
	def __init__(self, files=None, verbose=True, print_fn=print, r0=0.5, thresh=2.0, minnthresh=2, minarea=5, relfluxradius=3.0):
		self._verbose = verbose
		self._print_fn = print_fn

		self.r0 = r0
		self.thresh = thresh
		self.minnthresh = minnthresh
		self.minarea = minarea
		self.relfluxradius = relfluxradius

		self.init()

		if files:
			self.processFiles(files)

	def init(self):
		self.images = []
		self.processedImages = []
		self.headers = []
		self.filenames = []
		self.focpos = []

		self.objs = []

		self.valFWHM = []
		self.valNumStars = []
		self.valSeqFWHM = []
		self.valSeqNumStars = []
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

			if len(self.focpos) > 0 and self.focpos[-1] >= pos:
				# End of monotonous interval
				break

			# Estimators based on detected stars
			obj,image1 = get_objects_fwhm(image, r0=self.r0, thresh=self.thresh, minnthresh=self.minnthresh, minarea=self.minarea, relfluxradius=self.relfluxradius, return_image=True)

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
			self.processedImages.append(image)
			self.headers.append(dict(header))
			self.filenames.append(filename)
			self.focpos.append(pos)

			self.objs.append(obj)

			self.valFWHM.append(value_FWHM)
			self.valNumStars.append(value_NumStars)
			self.valStd.append(value_Std)
			self.valFFT.append(value_FFT)

		self.log('Acquired %d focusing points from %d files' % (len(self.focpos), len(files)))

		# Convert everything to NumPy arrays for convenience
		self.images, self.processedImages, self.headers, self.filenames, self.focpos, self.objs, self.valFWHM, self.valNumStars, self.valStd, self.valFFT = [np.array(_) for _ in self.images, self.processedImages, self.headers, self.filenames, self.focpos, self.objs, self.valFWHM, self.valNumStars, self.valStd, self.valFFT]

		# Analyze the ensemble of measurement to extract stars visible on several frames
		self.processSequences()

	def processSequences(self, min_length=5):
		"""Analyze lists of stars from different frames and leave only the ones seen many times"""
		fids,xarr,yarr,hfds = [],[],[],[]

		for fid,obj in enumerate(self.objs):
			xarr += list(obj['x'])
			yarr += list(obj['y'])
			hfds += list(obj['hfd'])
			fids += list(np.repeat(fid, len(obj['x'])))

		if not len(xarr):
			self.log("No points detected, skipping sequence analysis")
			self.valSeqFWHM = []
			self.valSeqNumStars = []
			return

		fids,xarr,yarr,hfds = [np.array(_) for _ in fids,xarr,yarr,hfds]
		kd = cKDTree(np.array([xarr,yarr]).T)

		def refine_pos(x,y):
			x1,y1 = [np.median(_) for _ in x,y]
			dx1,dy1 = [1.4826*np.median(np.abs(_)) for _ in x-x1,y-y1]

			dr = 2.0*np.sqrt(dx1*dx1 + dy1*dy1)

			return x1,y1,dr

		vmask = np.zeros_like(xarr, np.bool)
		N0 = 0

		xs,ys,Ns,drs = [],[],[],[]

		dr0 = np.max(hfds) # Initial extraction radius

		for i in xrange(len(vmask)):
			if not vmask[i]:
				ids = kd.query_ball_point([xarr[i],yarr[i]], dr0)

				if len(vmask[ids][vmask[ids] == False]) >= min_length:
					x1,y1,dr1 = refine_pos(xarr[ids],yarr[ids])

					dr1 = min(dr0, dr1)
					ids = kd.query_ball_point([x1,y1], dr1)

				if len(vmask[ids][vmask[ids] == False]) >= min_length:
					xs.append(x1)
					ys.append(y1)
					Ns.append(len(ids))
					drs.append(dr1)
					N0 += 1

				vmask[ids] = True

		self.log("%d sequences longer than %d from %d points on %d frames" % (N0, min_length, len(vmask), len(self.focpos)))

		if len(xs) > 0:
			kds = cKDTree(np.array([xs,ys]).T)
			xs,ys,Ns,drs = [np.array(_) for _ in xs,ys,Ns,drs]

		self.valSeqNumStars = np.zeros_like(self.focpos, dtype=np.int)
		vH = [[] for _ in xrange(len(self.focpos))]

		for _ in xrange(len(xs)):
			ids = kd.query_ball_point([xs[_], ys[_]], drs[_])
			for __ in ids:
				self.valSeqNumStars[fids[__]] += 1
				vH[fids[__]].append(hfds[__])

		self.valSeqFWHM = np.array([np.median(_) for _ in vH])

	def getBestFocus(self, fitter='spline', min_length=5):
		"""
		Analyze the focus quality estimations and get the best value.
		"""
		if not len(self.focpos):
			self.log('Not enough points for focus estimation')
			return None

		self.shouldRefine = False
		self.fitFailed = False

		self.estimator = 'Sequence FWHM'
		self.fitter = fitter
		bestPos = self.headers[0].get('FOC_DEF')

		if not len(self.valSeqNumStars) or np.max(self.valSeqNumStars) == 0:
			self.fitFailed = True
			self.log('Not enough good (sequence detected) stars for focus estimation')
			return bestPos

		# Peak position in raw data
		midx = np.where(self.valSeqNumStars == np.max(self.valSeqNumStars))[0]
		bestPos = np.mean(self.focpos[midx])

		posSpan = np.max(self.focpos) - np.min(self.focpos) # Total span of focusing interval

		if bestPos > np.min(self.focpos) + 0.1*posSpan and bestPos < np.max(self.focpos) - 0.1*posSpan:
			thresh = max(1, np.quantile(self.valSeqNumStars, 1.0-5./len(self.focpos))-1)
			self.log('Threshold for sequence number of stars', thresh)

			idx = np.where(self.valSeqNumStars >= thresh)[0]
			if np.max(idx[1:]-idx[:-1]) > 2:
				self.fitFailed = True
				self.log('Non-continuous interval for focus peak fitting, something is wrong')
			else:
				pos = self.focpos[idx]
				val = self.valSeqFWHM[idx]

				# Smoothing spline maximum
				spl = UnivariateSpline(pos, (1.0*val - np.min(val))/(np.max(val) - np.min(val)), s=0.01)
				x = np.linspace(np.min(pos), np.max(pos), 500)
				y = spl(x)

				midx = np.where(y == np.min(y))[0]
				bestPos = np.mean(x[midx])

				if bestPos < np.min(x) + 0.1*(np.max(x) - np.min(x)) or bestPos > np.max(x) - 0.1*(np.max(x) - np.min(x)):
					self.fitFailed = True
					self.log('Can\'t determine best focus position with spline fitting')
				else:
					self.log('Best position from spline fitting', bestPos)

		# Handle peak at the range ends
		if bestPos < np.min(self.focpos) + 0.1*posSpan:
			self.log('Maximum on left edge - refinement needed')
			self.shouldRefine = True

		elif bestPos > np.max(self.focpos) - 0.1*posSpan:
			self.log('Maximum on right edge - refinement needed')
			self.shouldRefine = True

		self.bestPos = bestPos

		return bestPos

# Local Variables:
# tab-width: 4
# python-indent-offset: 4
# indent-tabs-mode: t
# End:

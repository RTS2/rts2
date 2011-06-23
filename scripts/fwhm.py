#!/usr/bin/python

import sextractor
import sys
from optparse import OptionParser

def processImage(fn,d):
	if d:
		d.set('file ' + fn)

	c = sextractor.Sextractor(fn,['X_IMAGE','Y_IMAGE','MAG_BEST','FLAGS','CLASS_STAR','FWHM_IMAGE','A_IMAGE','B_IMAGE'])
	c.runSExtractor()
	c.sortObjects(2)

	i = 0
	fwhm = 0
	a = 0
	b = 0
	for x in c.objects:
		if (x[3] == 0 and x[4] != 0):
			if d:
				d.set('regions','image; circle {0} {1} 10 # color=green'.format(x[0],x[1]))
			fwhm += x[5]
			a += x[6]
			b += x[7]
			i += 1
		elif d:
			d.set('regions','image; point {0} {1} # point = x 5 color=red'.format(x[0],x[1]))

	if i > 9:
		import pyfits
		import math
		ff = pyfits.fitsopen(fn)
		fwhm /= i
		suffix = '_unknown'
		try:
			suffix = '_' + ff[0].header['CCD_NAME']
		except KeyError,er:
			pass
		print 'double fwhm{0} "calculated FWHM" {1}'.format(suffix,fwhm)
		zd = 90 - ff[0].header['TEL_ALT']
		fz = fwhm * (math.cos(math.radians(zd)) ** 0.6)
		print 'double fwhm_zenith{0} "estimated zenith FWHM" {1}'.format(suffix,fz)
		print 'double fwhm_foc{0} "focuser positon" {1}'.format(suffix,ff[0].header['FOC_POS'])
		print 'double fwhm_nstars{0} "number of stars for FWHM calculation" {1}'.format(suffix,i)
		print 'double fwhm_zd{0} "[deg] zenith distance of the FWHM measurement" {1}'.format(suffix,zd)
		print 'double fwhm_az{0} "[deg] azimuth of the FWHM measurement" {1}'.format(suffix,ff[0].header['TEL_AZ'])
		print 'double fwhm_airmass{0} "airmass of the FWHM measurement" {1}'.format(suffix,ff[0].header['AIRMASS'])

		if d:
			d.set('regions','image; text 100 100 # color=red text={' + ('FWHM {0} zenith {1} foc {2} stars {3}').format(fwhm,fz,ff[0].header['FOC_POS'],i) + '}')

if __name__ == '__main__':
	parser = OptionParser()
	parser.add_option('-d',help='display image and detected stars in DS9',action='store_true',dest='show_ds9',default=False)
	(options,args)=parser.parse_args()

	d = None
	if options.show_ds9:
		import ds9
		d = ds9.ds9()

	for fn in args:
		processImage(fn,d)

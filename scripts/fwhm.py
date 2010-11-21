#!/usr/bin/python

import sextractor
import sys

fn = sys.argv[1]

c = sextractor.Sextractor(fn,['X_IMAGE','Y_IMAGE','MAG_BEST','FLAGS','CLASS_STAR','FWHM_IMAGE','A_IMAGE','B_IMAGE'])
c.runSExtractor()
c.sortObjects(2)

i = 0
fwhm = 0
a = 0
b = 0
for x in c.objects:
  	if (x[3] == 0 and x[4] != 0):
		#d.set('regions', 'image; circle {0} {1} 10'.format(x[0],x[1]))
		fwhm += x[5]
		a += x[6]
		b += x[7]
		i += 1

if i > 9:
	import pyfits
	import math
	ff = pyfits.fitsopen(fn)
	fwhm /= a
	suffix = ff[0].header['CCD_NAME']
	print 'double fwhm{0} "calculated FWHM" {1}'.format(suffix,fwhm)
	zd = 90 - ff[0].header['TEL_ALT']
	print 'double fwhm_zenith{0} "estimated zenith FWHM" {1}'.format(suffix,fwhm * (math.cos(math.radians(zd)) ** 0.6))
	print 'double fwhm_foc{0} "focuser positon" {1}'.format(suffix,ff[0].header['FOC_POS'])
	print 'double fwhm_nstars{0} "number of stars for FWHM calculation" {1}'.format(suffix,i)
	print 'double fwhm_zd{0} "[deg] zenith distance of the FWHM measurement" {1}'.format(suffix,zd)
	print 'double fwhm_az{0} "[deg] azimuth of the FWHM measurement" {1}'.format(suffix,ff[0].header['TEL_AZ'])
	print 'double fwhm_airmass{0} "airmass of the FWHM measurement" {1}'.format(suffix,ff[0].header['AIRMASS'])

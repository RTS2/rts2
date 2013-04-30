#!/usr/bin/env python

import sys
from rts2 import Sextractor
import ds9

def measure(fn):
	s = Sextractor(fields=['X_IMAGE','Y_IMAGE','ALPHA_J2000','DELTA_J2000','ERRX2_IMAGE','ERRY2_IMAGE','MAG_BEST','FWHM_IMAGE'])
	s.runSExtractor(fn)

	print s.objects

	# plot on DS9
	d = ds9.ds9('test_sextractor')

	d.set('file ' + fn)
	for o in s.objects:
		print o
		d.set('regions','J2000; circle {0} {1} {2}p'.format(o[2],o[3],int(abs(o[6]))))
		d.set('regions','J2000; circle {0} {1} {2}p # color=red'.format(o[2],o[3],o[7]))

measure(sys.argv[1])

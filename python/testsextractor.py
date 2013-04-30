#!/usr/bin/env python

import sys
from rts2 import Sextractor
import ds9

def measure(fn):
	s = Sextractor(fields=['X_IMAGE','Y_IMAGE','X_WORLD','Y_WORLD','ERRX2_IMAGE','ERRY2_IMAGE','MAG_BEST'])
	s.runSExtractor(fn)

	print s.objects

	# plot on DS9
	d = ds9.ds9('test_sextractor')

	d.set('file ' + fn)
	for o in s.objects:
		print o
		d.set('regions','circle {0} {1} {2}'.format(o[0],o[1],int(abs(o[6]))))

measure(sys.argv[1])

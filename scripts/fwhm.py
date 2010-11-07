#!/usr/bin/python

# prints FWHM as double parameter, usable for imgproc.

import sextractor
import sys

c = sextractor.Sextractor(sys.argv[1],['X_IMAGE','Y_IMAGE','MAG_BEST','FLAGS','CLASS_STAR','FWHM_IMAGE','A_IMAGE','B_IMAGE'])
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
			print 'double fwhm "calculated FWHM" {0}'.format(fwhm/i)
			break 



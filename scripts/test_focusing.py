#!/usr/bin/python

# Test focusing infrastrucure

import focusing
import sys
import pyfits

from scipy import *
from pylab import *

tries = {}

for fn in sys.argv[1:]:
	ff = pyfits.fitsopen(fn)
	tries[float(ff[0].header['FOC_POS'])]=fn

f = focusing.Focusing()

print f.findBestFWHM(tries)

#fitfunc = lambda p, x: p[0] + p[1] * x + p[2] * (x ** 2) + p[3] * (x ** 3) + p[4] * (x ** 4)
fitfunc = lambda p, x: p[0] + p[1] * x + p[2] * (x ** 2)

x = linspace(f.focpos.min() - 1, f.focpos.max() + 1)

plot (f.focpos, f.fwhm, "ro", x, fitfunc(f.fwhm_poly, x), "r-")

show()

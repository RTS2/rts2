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
	tries[ff[0].header['FOC_FOFF']]=fn

f = focusing.Focusing()

print f.findBestFWHM(tries)

fitfunc = lambda p, x: p[0] + p[1] * x + p[2] * (x ** 2) + p[3] * (x ** 3) + p[4] * (x ** 4)

x = linspace(f.focpos.min(), f.focpos.max())

plot (f.focpos, f.fwhm, "ro", x, fitfunc(f.fwhm_poly, x), "r-")

show()

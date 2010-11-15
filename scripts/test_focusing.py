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

b,ftype = f.findBestFWHM(tries)

print b

fitfunc = None

if ftype == focusing.LINEAR:
	fitfunc = lambda p, x: p[0] + p[1] * x 
elif ftype == focusing.P2:
	fitfunc = lambda p, x: p[0] + p[1] * x + p[2] * (x ** 2)
elif ftype == focusing.P4:
	fitfunc = lambda p, x: p[0] + p[1] * x + p[2] * (x ** 2) + p[3] * (x ** 3) + p[4] * (x ** 4)
elif ftype == focusing.H3:
	fitfunc = lambda p, x: sqrt(p[0] ** 2 + p[1] ** 2 * (x - p[2]) ** 2)
elif ftype == focusing.H2:
	fitfunc = lambda p, x: sqrt(p[0] ** 2 + 3.46407715307 ** 2 * (x - p[1]) ** 2) # 3.46 based on HYPERBOLA fits
else:
	raise Exception('Unknow fit type {0}'.format(ftype))

x = linspace(f.focpos.min() - 1, f.focpos.max() + 1)

plot (f.focpos, f.fwhm, "ro", x, fitfunc(f.fwhm_poly, x), "r-")

show()

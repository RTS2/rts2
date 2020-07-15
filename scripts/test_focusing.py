#!/usr/bin/env python

# Test focusing infrastrucure

import rts2.focusing
import sys
from astropy.io import fits

from scipy import *
from matplotlib.pyplot import *

tries = {}

for fn in sys.argv[1:]:
	ff = fits.open(fn)
	focpos=len(tries)
	try:
		focpos=float(ff[0].header['FOC_POS'])
	except KeyError,ke:
		pass
	tries[focpos]=fn

f = rts2.focusing.Focusing()

b,ftype = f.findBestFWHM(tries)
# for way off-focus, low S/N images..
#b,ftype = f.findBestFWHM(tries,min_stars=10,filterGalaxies=False,threshold=5)

print b

f.plotFit(b,ftype)

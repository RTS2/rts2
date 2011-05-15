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
# for way off-focus, low S/N images..
#b,ftype = f.findBestFWHM(tries,min_stars=10,filterGalaxies=False,threshold=5)

print b

f.plotFit(b,ftype)

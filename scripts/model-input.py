#!/usr/bin/python

from __future__ import division, print_function

import numpy
from astropy import wcs
from astropy.io import fits
import sys

def print_model_input(filename):
	hdulist = fits.open(filename)
	h = hdulist[0].header
	w = wcs.WCS(h)
	ra,dec = w.all_pix2world(2000,2000,0)
	tar_telra = float(h['TAR_TELRA'])
	tar_teldec = float(h['TAR_TELDEC'])
	print(h['IMGID'],h['JD'],h['LST'],tar_telra,tar_teldec,h['AXRA'],h['AXDEC'],ra,dec)

if __name__ == '__main__':
	print('#  Observation	  MJD	   RA-MNT   DEC-MNT LST-MNT	  AXRA	  AXDEC   RA-TRUE  DEC-TRUE')
	for img in sys.argv[1:]:
		print_model_input(img)

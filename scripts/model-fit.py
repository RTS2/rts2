#!/usr/bin/env python

# Telescope pointing model fit, as described in paper by Marc Buie:
#
# ftp://ftp.lowell.edu/pub/buie/idl/pointing/pointing.pdf 
#

import sys
import numpy as np

from math import radians,degrees,cos,sin,tan,sqrt,atan2,acos
from scipy.optimize import leastsq

from pylab import *

# telescope latitude - north positive
latitude = 40.454069
# telescope longitude - not used at the moment
longitude = -110

latitude_r = radians(latitude)

# Fit function.
# a_ra - target HA
# r_ra - calculated (real) HA
# a_dec - target DEC
# r_dec - calculated (real) DEC
# DEC > 90 or < -90 means telescope flipped (DEC axis continues for modelling purposes)
def fit_model(params, a_ra, r_ra, a_dec, r_dec):
        print 'computing', latitude_r, params, a_ra, r_ra, a_dec, r_dec
	diff1 = a_dec - r_dec - params[0] + params[1]*np.cos(a_ra) + params[2]*np.sin(a_ra) + params[3]*(sin(latitude_r) * np.cos(a_dec) - cos(latitude_r) * np.sin(a_dec) * np.cos(a_ra))
	#diff1 = a_dec - r_dec - params[0]
	#diff2 = a_ra - r_ra - params[4]
	diff2 = a_ra - r_ra - params[4] - params[5]/np.cos(a_dec) + params[6]*np.tan(a_dec) - (-params[1]*np.sin(a_ra) + params[2]*np.cos(a_ra)) * np.tan(a_dec) - params[3]*np.cos(latitude_r)*np.sin(a_ra) / np.cos(a_dec) - params[7]*(np.sin(latitude_r) * np.tan(a_dec) + np.cos(a_dec) * np.cos(a_ra)) - params[8] * a_ra
	return np.concatenate((diff2, diff1))


# open file
# expected format:
#  Observation      MJD       RA-MNT   DEC-MNT LST-MNT      AXRA      AXDEC   RA-TRUE  DEC-TRUE
#02a57222e0002o 57222.260012 275.7921  77.0452 233.8937  -55497734  -46831997 276.0206  77.0643
#skip first line, use what comes next. Make correction on DEC based on axis - if above zeropoint + 90 deg, flip DEC (DEC = 180 - DEC)

with open(sys.argv[1]) as f:
	# skip first line
	f.readline()
	line = f.readline()
	rdata = []
	while not(line == ''):
		rdata.append(line.split())
		line = f.readline()

	print rdata

	data = [(float(lst) - float(a_ra), float(a_dec), float(lst) - float(r_ra), float(r_dec), sn) for sn,mjd,a_ra,a_dec,lst,ax_ra,ax_dec,r_ra,r_dec in rdata]

	a_data = np.array(data)
	print a_data
	
	par_init = np.array([0,0,0,0,0,0,0,0,0])
	
	aa_ra = np.radians(np.array(a_data[:,0],np.float))
	aa_dec = np.radians(np.array(a_data[:,1],np.float))
	ar_ra = np.radians(np.array(a_data[:,2],np.float))
	ar_dec = np.radians(np.array(a_data[:,3],np.float))

	diff_ra = np.degrees(aa_ra - ar_ra)
	diff_dec = np.degrees(aa_dec - ar_dec)

	best, cov, info, message, ier = leastsq(fit_model, par_init, args=(aa_ra, ar_ra, aa_dec, ar_dec), full_output=True)

	print "Best fit",np.degrees(best)
	print cov
	print info
	print message
	print ier

	print 'Zero point in DEC (") {0}'.format(degrees(best[0])*60.0)
	print 'Zero point in RA (") {0}'.format(degrees(best[4])*60.0)
	i = sqrt(best[1]**2 + best[2]**2)
	print 'Angle between true and instrumental poles (") {0}'.format(degrees(i)*60.0)
	print best[1], i, best[1]/i, acos(best[1]/i), atan2(best[2], best[1])
	print 'Angle between line of pole and true meridian (deg) {0}'.format(degrees(atan2(best[2], best[1]))*60.0)
	print 'Telescope tube droop in HA and DEC (") {0}'.format(degrees(best[3])*60.0)
	print 'Angle between optical and telescope tube axes (") {0}'.format(degrees(best[5])*60.0)
	print 'Mechanical orthogonality of RA and DEC axes (") {0}'.format(degrees(best[6])*60.0)
	print 'Dec axis flexure (") {0}'.format(degrees(best[7])*60.0)
	print 'HA encoder scale error ("/degree) {0}'.format(degrees(best[8])*60.0)

	print diff_ra * 3600.0
	print diff_dec * 3600.0

	# feed parameters to diff, obtain model differences. Closer to zero = better
	diff_model = np.degrees(fit_model(best, aa_ra, ar_ra, aa_dec, ar_dec))
	print 'DIFF_MODEL ',
	for d in diff_model:
		print d * 3600.0,
	print

	print 'RTS2_MODEL',
	for a in best:
		print a,
	print

	plot(range(0,len(diff_model)), diff_model * 3600.0, 'bo', range(0,len(diff_ra)), diff_ra * 3600.0, 'ro', range(len(diff_ra), len(diff_ra) + len(diff_dec)), diff_dec * 3600.0, 'go')
	show()

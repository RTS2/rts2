#!/usr/bin/env python
import csv
import sys
import numpy as np

from math import radians,degrees,cos,sin,tan
from scipy.optimize import leastsq

latitude = 20.0
lontitude = -110

latitude_r = radians(latitude)

# Fit function.
# a_ra - target RA
# r_ra - calculated (real) RA
# a_dec - target DEC
# r_dec - calculated (real) DEC
def fit_dec(params, a_ra, r_ra, a_dec, r_dec):
	diff1 = a_dec - r_dec - params[0] + params[1]*np.cos(a_ra) + params[2]*np.sin(a_ra) + params[3]*(sin(latitude_r) * np.cos(a_dec) - cos(latitude_r) * np.sin(a_dec) * np.cos(a_ra))
	diff2 = a_ra - r_ra - params[4] - params[5]/np.cos(a_dec) + params[6]*np.tan(a_dec) - (-params[1]*np.sin(a_ra) + params[2]*np.cos(a_ra)) * np.tan(a_dec) + params[3]*np.cos(latitude_r)*np.sin(a_ra) / np.cos(a_dec) - params[7]*(np.sin(latitude_r) * np.tan(a_dec) + np.cos(a_dec) * np.cos(a_ra)) - params[8] * a_ra
	return np.concatenate((diff1, diff2))


with open(sys.argv[1]) as f:
	reader = csv.reader(f, delimiter=' ', skipinitialspace=True)
	data = [(np.float(a_ra), np.float(a_dec), np.float(r_ra), np.float(r_dec), sn, use) for a_ra,a_dec,r_ra,r_dec,sn,use in reader]

	a_data = np.array(data)
	
	par_init = np.array([0,0,0,0,0,0,0,0,0])
	
	aa_ra = np.radians(np.array(a_data[:,0],np.float))
	aa_dec = np.radians(np.array(a_data[:,1],np.float))
	ar_ra = np.radians(np.array(a_data[:,2],np.float))
	ar_dec = np.radians(np.array(a_data[:,3],np.float))

	best, cov, info, message, ier = leastsq(fit_dec, par_init, args=(aa_ra, ar_ra, aa_dec, ar_dec), full_output=True)

	print "Best fit",np.degrees(best)	

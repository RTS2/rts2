#!/usr/bin/env python

from __future__ import print_function

import numpy

class MPCEphem:
    def __init__(self, fname):

#00243              [H= 9.94]
#Date       UT      R.A. (J2000) Decl.    Delta     r     El.    Ph.   V      Sky Motion       Uncertainty info
#            h m s                                                            "/min    P.A.    3-sig/" P.A.
#2017 09 10 000000 12 43 00.8 -05 24 35   3.810   2.927   24.8   8.3  15.8    0.96    113.1       N/A   N/A / Map / Offsets
#....
        data = numpy.loadtxt(fname, dtype={
            'names': ('year', 'month', 'day', 'hms','ra_h', 'ra_m', 'ra_s', 'dec_d', 'dec_m', 'dec_s', 'delta', 'r', 'el', 'ph', 'V', 'sky', 'PA', 'uncerr'),
            'formats': ('i4', 'i2', 'i2', 'S6', 'i2', 'i2', 'f4', 'i2', 'i2', 'i2', 'f4', 'f4', 'f4', 'f4', 'f4', 'f4', 'f4', 'S20')
        })

        self.data = numpy.array(
            [(
                numpy.datetime64('{0}-{1:>02}-{2:>02}T{3}:{4}:{5}.00'.format(year, month, day, hms[:2], hms[2:4], hms[4:6])),
	        ra_h + ra_m / 60.0 + ra_s / 3600.0,
		dec_d + dec_m / 60.0 + dec_s / 3600.0

            ) for year, month, day, hms, ra_h, ra_m, ra_s, dec_d, dec_m, dec_s, delta, r, el, ph, V, sky, PA, uncerr in data],
            dtype=[
                ('date', 'datetime64[ms]'),
                ('ra', 'f4'),
		('dec', 'f4')
            ]
        )

    def interpolate(self, date):
        ra = numpy.interp(date.astype('d'), self.data['date'].astype('d'), self.data['ra'], right=None)
        dec = numpy.interp(date.astype('d'), self.data['date'].astype('d'), self.data['dec'], right=None)
	return (ra, dec)

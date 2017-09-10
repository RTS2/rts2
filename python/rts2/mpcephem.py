# Load MPEC ephemeris, calculates position and sky speeds
#
# (C) 2017 Petr Kubanek <petr@rts2.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


from __future__ import print_function

import numpy

class MPCEphem:
    """Load MPC ephemeris file, allow calculation of interpolated positions."""
    def __init__(self, fname):

#Assuming basic format, with sky motion including PA
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
		dec_d + dec_m / 60.0 + dec_s / 3600.0,
		sky * numpy.sin(numpy.radians(PA)),
		sky * numpy.cos(numpy.radians(PA)),
		V
            ) for year, month, day, hms, ra_h, ra_m, ra_s, dec_d, dec_m, dec_s, delta, r, el, ph, V, sky, PA, uncerr in data],
            dtype=[
                ('date', 'datetime64[ms]'),
                ('ra', 'f4'),
		('dec', 'f4'),
		('ra_motion', 'f4'),
		('dec_motion', 'f4'),
		('V', 'f4')
            ]
        )

    def interpolate(self, date):
        """Interpolates ephemeris to given date."""
        d = date.astype('datetime64[ms]').astype('d')
	if d < self.data['date'][0].astype('d') or d > self.data['date'][-1].astype('d'):
	    return (None, None, None, None, None)

        ra = numpy.interp(d, self.data['date'].astype('d'), self.data['ra'])
        dec = numpy.interp(d, self.data['date'].astype('d'), self.data['dec'])
        ra_motion = numpy.interp(d, self.data['date'].astype('d'), self.data['ra_motion'])
        dec_motion = numpy.interp(d, self.data['date'].astype('d'), self.data['dec_motion'])
        V = numpy.interp(d, self.data['date'].astype('d'), self.data['V'])
	return (ra, dec, ra_motion, dec_motion, V)

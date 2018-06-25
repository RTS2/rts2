# fits2model - tool to convert FITS files to input for model (modelin file)
#
# (C) 2016 Petr Kubanek <petr@kubanek.net>
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

from astropy import wcs
from astropy.io import fits
import sys

ln = 1


def first_line(fn, of):
    """First line for GEM model input."""
    hdu = fits.open(fn)
    h = hdu[0].header
    of.write('#  Observation      JD    MNT-LST   RA-MNT   DEC-MNT       AXRA      AXDEC   RA-TRUE  DEC-TRUE\n')
    of.write('# observatory {0} {1} {2}\n'.format(h['LONGITUD'], h['LATITUDE'], h['ALTITUDE']))


def model_line(fitsname, of, center=None):
    """Model line."""
    global ln
    hdulist = fits.open(fitsname)
    h = hdulist[0].header
    w = wcs.WCS(h)
    if center is None:
        ra, dec = w.all_pix2world(float(h['NAXIS1'])/2, float(h['NAXIS2'])/2, 0)
    else:
        ra, dec = w.all_pix2world(center[0], center[1], 0)
    tar_telra = float(h['TAR_TELRA'])
    tar_teldec = float(h['TAR_TELDEC'])

    try:
        imgid = h['IMGID']
    except KeyError as ke:
        imgid = ln

    if w.wcs.radesys == '':
        of.write('# ' + '\t'.join(map(str, [imgid, h['JD'], h['LST'], tar_telra, tar_teldec, h['AXRA'], h['AXDEC'], ra, dec])))
    else:
        of.write('\t'.join(map(str, [imgid, h['JD'], h['LST'], tar_telra, tar_teldec, h['AXRA'], h['AXDEC'], ra, dec])))

    ln += 1
    of.write('\n')

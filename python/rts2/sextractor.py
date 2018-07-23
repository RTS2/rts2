# Sextractor-Python wrapper.
#
# You will need: scipy matplotlib sextractor
# This should work on Debian/ubuntu:
# sudo apt-get install python-matplotlib python-scipy python-pyfits sextractor
#
# If you would like to see sextractor results, get DS9 and pyds9:
#
# http://hea-www.harvard.edu/saord/ds9/
#
# Please be aware that current sextractor Ubuntu packages does not work
# properly. The best workaround is to install package, and the overwrite
# sextractor binary with one compiled from sources (so you will have access
# to sextractor configuration files, which program assumes).
#
# (C) 2010-2012  Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

from __future__ import print_function

import sys
import subprocess
import os
import tempfile
import numpy
from astropy.io import fits
import rts2.image


def _get_numpy_array(fields):
    __i = ['NUMBER', 'FLAGS', 'CLASS_STAR', 'EXT_NUMBER']
    # make result compatible with sep
    __trans = {
        'X_IMAGE':'x',
        'Y_IMAGE':'y',
        'FLAGS':'flag',
        'NUMBER':'id',
        'MAG_BEST':'mag'
    }
    ret = []
    for f in fields:
        try:
            tf = __trans[f]
        except KeyError:
            tf = f
        if f in __i:
            ret.append((tf, 'i'))
        else:
            ret.append((tf, 'f'))

    return ret

class Sextractor:
    """Class for a catalogue (SExtractor result)"""

    def __init__(self,
        fields=['NUMBER', 'FLUXERR_ISO', 'FLUX_AUTO', 'X_IMAGE', 'Y_IMAGE',
            'MAG_BEST', 'FLAGS', 'CLASS_STAR', 'FWHM_IMAGE', 'A_IMAGE',
            'B_IMAGE', 'EXT_NUMBER'
        ], sexpath='sextractor', sexconfig='/usr/share/sextractor/default.sex',
        starnnw='/usr/share/sextractor/default.nnw', threshold=2.7,
        deblendmin=0.03, saturlevel=65535, verbose=False
    ):
        self.sexpath = sexpath
        self.sexconfig = sexconfig
        self.starnnw = starnnw

        self.fields = fields
        self.objects = None
        self.threshold = threshold
        self.deblendmin = deblendmin
        self.saturlevel = saturlevel

        self.verbose = verbose

    def process(self, filename):
        pf, pfn = tempfile.mkstemp()
        ofd, output = tempfile.mkstemp()
        pfi = os.fdopen(pf, 'w')
        for f in self.fields:
            pfi.write(f + '\n')
        pfi.flush()

        cmd = [
            self.sexpath, filename, '-c', self.sexconfig, 
            '-PARAMETERS_NAME', pfn, '-DETECT_THRESH', str(self.threshold),
            '-DEBLEND_MINCONT', str(self.deblendmin), '-SATUR_LEVEL',
            str(self.saturlevel), '-FILTER', 'N', '-STARNNW_NAME',
            self.starnnw, '-CATALOG_NAME', output
        ]
        if not(self.verbose):
            cmd.append('-VERBOSE_TYPE')
            cmd.append('QUIET')
        try:
            proc = subprocess.Popen(cmd)
            proc.wait()
        except OSError as err:
            logging.error('canot run command {0} : error {1}'.format(
                join(cmd), err
            ))
            raise err

        # parse output
        self.objects = numpy.array([], _get_numpy_array(self.fields))
        of = os.fdopen(ofd, 'r')
        self.objects = numpy.append(
            self.objects,
            numpy.loadtxt(of, dtype=self.objects.dtype)
        )

        # unlink tmp files
        pfi.close()
        of.close()

        os.unlink(pfn)
        os.unlink(output)

        # shift x,y if multiext

        ext = numpy.unique(self.objects['EXT_NUMBER'])

        if len(ext) > 1:
            segoff = 10 ** (len(self.objects) % 10 + 1)
            f = fits.open(filename)
            exts = numpy.array([], dtype = self.objects.dtype)
            for e in ext:
                detsec = rts2.image.parse_detsec(f[e].header['DETSEC'])
                scale, offs = rts2.image.scale_offset(detsec, f[e].data.shape)
                o = self.objects[self.objects['EXT_NUMBER'] == e]
                for c in ['x']:
                    o[c] = o[c] * scale[0] + offs[0]
                for c in ['y']:
                    o[c] = o[c] * scale[1] + offs[1]
                o['id'] += segoff
                exts = numpy.append(exts, o)
            f.close()
            self.objects = exts

    def sort(self, col):
        """Sort objects by given collumn."""
        self.objects = numpy.sort(self.objects, order=col)

    def filter_galaxies(self, limit=0.2):
        """Filter possible galaxies"""
        return self.objects[self.objects['class_star'] > limit]

    def get_FWHM_stars(self, starsn=None, filterGalaxies=True):
        """Returns candidate stars for FWHM calculations. """
        if len(self.objects) == 0:
            raise Exception('Cannot find FWHM on empty source list')

        obj = None
        if filterGalaxies:
            obj = self.filter_galaxies()
            if len(obj) == 0:
                raise Exception('Cannot get FWHM - all sources were galaxies??')
        else:
            obj = self.objects

        # sort by magnitude
        obj = numpy.sort(obj, order='mag')
        obj = obj[obj['flag'] == 0]

        if starsn is None:
            starsn = len(obj)

        fwhmlist = obj[:starsn]

        return fwhmlist

    def calculate_FWHM(self, starsn=None, filterGalaxies=True):
        obj = self.get_FWHM_stars(starsn, filterGalaxies)
        try:
            i_fwhm = self.get_field('FWHM_IMAGE')
            import numpy
            fwhms = [x[i_fwhm] for x in obj]
            return numpy.median(fwhms), numpy.std(fwhms), len(fwhms)
            # return numpy.average(obj), len(obj)
        except ValueError as ve:
            traceback.print_exc()
            raise Exception('cannot find FWHM_IMAGE value')

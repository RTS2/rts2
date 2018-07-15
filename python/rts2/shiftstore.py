# Shift-store focusing.
#
# You will need: scipy matplotlib sep
# This should work on Debian/ubuntu:
# sudo apt-get install python-matplotlib python-scipy python-pyfits sextractor
#
# If you would like to see sextractor results, get DS9 and pyds9:
#
# http://hea-www.harvard.edu/saord/ds9/
#
# Please be aware that current sextractor Ubuntu packages does not work
# properly. The best workaround is to install package, and then overwrite
# sextractor binary with one compiled from sources (so you will have access
# to sextractor configuration files, which program assumes).
#
# (C) 2011  Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place - Suite 330, Boston, MA  02111-1307, USA.

import rts2.sextractor
import sys
import rts2.focusing
import numpy
import logging


class ShiftStore:
    """
    Shift-store focusing. Works in the following steps
        - extract sources (with sextractor)
        - order sources by magnitude/flux
        - try to find row for the brightests source
          - filter all sources too far away in X
          - try to find sequence, assuming selected source can be any in
            the sequence
        - run standard fit on images (from focusing.py) Parameters
          governing the algorithm are specified in ShiftStore constructor.
    Please beware - when using paritial match, you need to make sure the shifts
    will allow unique identification of the sources. It is your responsibility
    to make sure they allow so, the algorithm does not check for this.
    """

    def __init__(
        self, shifts=[100, 50, 50, 50, 50, 50, 50, 50], horizontal=True
    ):
        """
        @param shifts shifts performed between exposures, in pixels.
            Lenght of this array is equal to ((number of sources in a row) - 1).
        @param horizontal  search for horizontal trails (paraller to Y axis).
            If set to False, search for veritical trails (along X axis).
        """
        self.horizontal = horizontal
        self.objects = None
        self.sequences = []
        self.xsep = self.ysep = 5
        self.shifts = shifts
        self.focpos = range(0, len(self.shifts) + 1)

    def test_objects(self, x, can, i, otherc, partial_len=None):
        """
        Test if there is sequence among candidates matching expected
        shifts.  Candidates are stars not far in X coordinate from
        source (here we assume we are looking for vertical lines on
        image; horizontal lines are searched just by changing otherc parameter
        to the other axis index). Those are ordered by y and searched for stars
        at the expected positions. If multiple stars falls inside this box,
        then the one closest in magnitude/brightness estimate is
        selected.

        @param x       star sextractor row; ID X Y brightness estimate
            postion are expected and used
        @param can     catalogue of candidate stars
        @param i       expected star index in shift pattern
        @param otherc  index of other axis. Either 2 or 1
            (for vertical or horizontal shifts)
        @param partial_len allow partial matches (e.g. where stars are missing)
        """
        ret = []
        # here we assume y is otherc (either second or third),
        # and some brightnest estimate fourth member in x
        yi = x[otherc]  # expected other axis position
        xb = x[3]  # expected brightness
        # calculate first expected shift..
        for j in range(0, i):
            yi -= self.shifts[j]
        # go through list, check for candidate stars..
        cs = None
        sh = 0
        pl = 0  # number of partial matches..
        while sh <= len(self.shifts):
            # now interate through candidates
            for j in range(0, len(can)):
                # if the current shift index is equal to expected
                # source position...
                if sh == i:
                    # append x to sequence, and increase sh
                    # (and expected Y position)
                    try:
                        yi += self.shifts[sh]
                    except IndexError as ie:
                        break
                    sh += 1
                    ret.append(x)
                # get close enough..
                if abs(can[j][otherc] - yi) < self.ysep:
                    # find all other sources in vicinity..
                    k = None
                    cs = can[j]  # _c_andidate _s_tar
                    for k in range(j+1, len(can)):
                        # something close enough..
                        if abs(can[k][otherc] - yi) < self.ysep:
                            if abs(can[k][3] - xb) < abs(cs[3] - xb):
                                cs = can[k]
                        else:
                            continue

                    # append candidate star
                    ret.append(cs)
                    if k is not None:
                        j = k
                    # don't exit if the algorithm make it to end of shifts
                    try:
                        yi += self.shifts[sh]
                    except IndexError as ie:
                        break
                    sh += 1

            # no candiate exists
            if partial_len is None:
                if len(ret) == len(self.shifts) + 1:
                    return ret
                return None
            else:
                if len(ret) == len(self.shifts) + 1:
                    return ret
 
            # insert dummy postion..
            if otherc == 2:
                ret.append([None, x[1], yi, None])
            else:
                ret.append([None, yi, x[2], None])

            pl += 1

            try:
                yi += self.shifts[sh]
            except IndexError as ie:
                break
            sh += 1

        # partial_len is not None
        if (len(self.shifts) + 1 - pl) >= partial_len:
            return ret

        return None

    def find_row_objects(self, x, partial_len=None):
        """
        Find objects in row. Search for sequence of stars, where x fit
        as one member. Return the sequence, or None if the sequence
        cannot be found."""

        searchc = 'x'
        otherc = 'y'
        if not(self.horizontal):
            searchc = 'y'
            otherc = 'x' 

        xcor = x[searchc]

        # candidate array should contain x - that's expected
        can = [x for x in self.objects
           if abs(xcor - x[searchc]) < self.xsep
        ]     # canditate stars
        # sort by Y axis..
        can.sort(key=lambda x: x[otherc])
        # assume selected object is one in shift sequence
        # place it at any possible position in shift sequence, and test if the
        # sequence can be found
        if len(can) < len(self.shifts):
            return

	# try to fit sequence. If sequence cannot be fit, carry on - 
	# the middle star in sequence, which the algorithm tries to fit
	# (under assumtion this will be brightest star if it is almost in focus)
	# will be selected sometime
        max_ret = []
        for i in range(0, len(self.shifts) + 1):
            # test if sequence can be found..
            ret = self.test_objects(x, can, i, otherc, partial_len)
            # and if it is found, return it
            if ret is not None:
                if partial_len is None:
                    return ret
                elif len(ret) > len(max_ret):
                    max_ret = ret

        if partial_len is not None and len(max_ret) >= partial_len:
            return max_ret
        # cannot found sequnce, so return None
        return None

    def run_on_image(
        self, fn, partial_len=None, interactive=False, sequences_num=15,
        mag_limit_num=7
    ):
        """
        Run algorithm on image. Extract sources with sextractor, and
        pass them through sequence finding algorithm, and fit focusing position.

        Parameters
        ----------
        fn: name of FITS file to run on
        partial_len:
        interactive:
        sequences_num:
        mag_limit_num:

        Returns
        -------

        """

        c = rts2.sextractor.Sextractor(
                [
                'NUMBER', 'X_IMAGE', 'Y_IMAGE', 'MAG_BEST', 'FLAGS',
                'CLASS_STAR', 'FWHM_IMAGE', 'A_IMAGE', 'B_IMAGE', 'EXT_NUMBER'
            ],
            sexpath='/usr/bin/sextractor',
            sexconfig='/usr/share/sextractor/default.sex',
            starnnw='/usr/share/sextractor/default.nnw'
        )
        c.process(fn)
        # sort by flux/brightness
        c.sort('MAG_BEST')

        self.objects = c.objects

        logging.debug(
            'from {0} extracted {1} sources'.format(fn, len(c.objects)))

        d = None

        if interactive:
            import pyds9
            d = pyds9.DS9()
            # display in ds9
            d.set('mosaicimage {0}'.format(fn))

            for x in self.objects:
                d.set(
                    'regions',
                    'physical; point {0} {1} # point=x 5 color=red'.format(x[1], x[2])
                )

        sequences = []
        usednum = []

        for x in self.objects:
            # do not examine already used objects..
            if x[0] in usednum:
                continue

            # find object in a row..
            b = self.find_row_objects(x, partial_len)
            if b is None:
                continue
            sequences.append(b)
            if d:
                d.set('regions select none')
                d.set(
                    'regions',
                    'physical; circle {0} {1} 20 # color=yellow tag = sel'.format(
                    x[1], x[2])
                )
            for obj in b:
                usednum.append(obj[0])
            if d:
                logging.debug('best mag: %d', x[3])
                d.set('regions select group sel')
                d.set('regions delete select')
                for obj in b:
                    if obj[0] is None:
                        d.set('regions',
                            'physical; point {0} {1} # point=boxcircle 15 color = red'.format(
                            obj[1], obj[2]))
                    else:
                        d.set(
                            'regions',
                            'image; circle {0} {1} 10 # color = green'.format(
                            obj[1], obj[2])
                        )
            if len(sequences) > sequences_num:
                break
        # if enough sequences were found, process them and try to fit results
        if len(sequences) < sequences_num:
            logging.error('only {0} sequnces found, {1} required'.format(
                len(sequences), sequences_num
            ))
        else:
            # get median of FWHM from each sequence
            fwhm = []
            for x in range(0, len(self.shifts) + 1):
                fa = []
                for o in sequences:
                    if o[x][0] is not None:
                        fa.append(o[x][6])
                # if length of magnitude estimates is greater then limit..
                if len(fa) >= mag_limit_num:
                    m = numpy.median(fa)
                    fwhm.append(m)
                else:
                    if interactive:
                        logging.debug(
                            'removing focuser position, because not enough matches were found: %s',
                            self.focpos[x]
                        )
                    self.focpos.remove(self.focpos[x])
            # fit it
            foc = rts2.focusing.Focusing()

            res, ftype = foc.doFitOnArrays(fwhm, self.focpos, rts2.focusing.H2)
            if interactive:
                logging.debug('%s %s', res, ftype)
                foc.plotFit(res, ftype)
        return len(sequences)

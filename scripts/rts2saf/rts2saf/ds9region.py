#!/usr/bin/python
# (C) 2013, Markus Wildi, markus.wildi@bluewin.ch
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2, or (at your option)
#   any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software Foundation,
#   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#   Or visit http://www.gnu.org/licenses/gpl.html.
#

__author__ = 'markus.wildi@bluewin.ch'

import threading
from ds9 import ds9

# on first call of DS9 a frame is already open
first = True

class Ds9DisplayThread(threading.Thread):
    """Thread displays a set of FITS images .

    :var debug: enable more debug output with --debug and --level
    :var dataSxtr: list of :py:mod:`rts2saf.data.DataSxtr`
    :var logger:  :py:mod:`rts2saf.log`

    """


    def __init__(self, debug=False,  dataSxtr=None, logger=None):
        super(Ds9DisplayThread, self).__init__()
        self.debug = debug
        self.dataSxtr = dataSxtr
        self.logger = logger
        self.stoprequest = threading.Event()
        try:
            self.display = ds9()
        except Exception, e:
            self.logger.warn('__init__: forking ds9 failed:\n{0}'.format(e))

    def displayWithRegion(self, dSx = None): 
        """

        :var dSx: :py:mod:`rts2saf.data.DataSxtr`

        
        :return: True if success else False

        """
        global first 
        try:
            if not first:
                self.display.set('frame new')
            else:
                first = False

            #self.display.set('zoom to fit')
            self.display.set('scale zscale')
            self.display.set('file {0}'.format(dSx.fitsFn))
            self.display.set('regions command {{text {0} {1} #text="FOC_POS {2}" color=magenta font="helvetica 15 normal"}}'.format(80,10, int(dSx.focPos)))

        except Exception, e:
            self.logger.warn('analyze: plotting fits with regions failed:\n{0}'.format(e))
            return False

        try:
            i_x = dSx.fields.index('X_IMAGE')
            i_y = dSx.fields.index('Y_IMAGE')
            i_flg = dSx.fields.index('FLAGS')
            i_fwhm = dSx.fields.index('FWHM_IMAGE')

        except Exception, e:
            self.logger.warn('____DisplayThread: retrieving data failed:\n{0}'.format(e))
            return False

        for x in dSx.rawCatalog:
            if not x:
                continue

            if x in dSx.catalog:
                color='green'
                try:
                    self.display.set('regions', 'image; circle {0} {1} {2} # color={{{3}}} text={{{4}}}'.format(x[i_x],x[i_y], x[i_fwhm], color, x[i_fwhm]))
                except Exception, e:
                    self.logger.warn('____DisplayThread: plotting fits with regions failed:\n{0}'.format(e))
                    return False

            elif x[i_flg] == 0:
                color='yellow'
            else:
                color='red'
            try:
                self.display.set('regions', 'image; circle {0} {1} {2} # color={{{3}}}'.format(x[i_x],x[i_y], x[i_fwhm], color))
            except Exception, e:
                self.logger.warn('____DisplayThread: plotting fits with regions failed:\n{0}'.format(e))
                return False
        else:
            return True

        return False

    def run(self):
        """ Display via DS9.  
        """

        # ToDo create new list
        self.dataSxtr.sort(key=lambda x: int(x.focPos))

        for dSx in self.dataSxtr:
            if dSx.fitsFn:
                if not self.displayWithRegion(dSx=dSx):
                    break # something went wrong
            else:
                self.logger.warn('____DisplayThread: OOOOOOOOPS, no file name for fits image number: {0:3d}'.format(dSx.fitsFn))
            
            self.display.set('zoom to fit')
        # do not remove me: enough is not enough
        self.display.set('blink')

    def join(self, timeout=None):
        """Stop thread on request.
        """
        self.display.set('blink no')
        self.display.set('frame delete all')
        if self.debug: self.logger.debug('____DisplayThread: join, timeout {0}, stopping thread on request'.format(timeout))
        self.stoprequest.set()
        super(Ds9DisplayThread, self).join(timeout)





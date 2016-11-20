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

import os
import threading
from pyds9 import DS9

# on first call of DS9 a frame is already open
first = True

class Ds9DisplayThread(threading.Thread):
    """Thread displays a set of FITS images .

    :var debug: enable more debug output with --debug and --level
    :var dataSxtr: list of :py:mod:`rts2saf.data.DataSxtr`
    :var logger:  :py:mod:`rts2saf.log`

    """
    def __init__(self, debug=False, logger=None):
        super(Ds9DisplayThread, self).__init__()
        self.debug = debug
        self.logger = logger
        self.stoprequest = threading.Event()
        try:
            self.display = DS9()
        except Exception as e:
            self.logger.warn('__init__: forking ds9 failed:\n{0}'.format(e))

    def displayWithRegion(self, fitsFn=None,x=None,y=None,color='blue'): 
        global first 
        try:
            if not first:
                self.display.set('frame new')
            else:
                first = False

            #self.display.set('zoom to fit')
            self.display.set('scale zscale')
            self.display.set('file {0}'.format(fitsFn))

        except Exception as e:
            self.logger.warn('analyze: plotting fits with regions failed:\n{0}'.format(e))
            return False

        try:
            self.display.set('regions', 'image; circle {0} {1} {2} # color={{{3}}}'.format(x,y, 20, color))
        except Exception as e:
            self.logger.warn('____DisplayThread: plotting fits with regions failed:\n{0}'.format(e))
            return False

        return True

    def run(self, fitsFn=None,x=None,y=None,color='blue'):
        if fitsFn:
            if not os.path.isfile(fitsFn):
                if len(fitsFn)>0:
                    self.logger.debug('ds9region::run: {0} does not exist'.format(fitsFn))
                else:
                    self.logger.debug('ds9region::run: empty file name received')
                    
                return
            
            if not self.displayWithRegion(fitsFn=fitsFn,x=x,y=y,color=color):
                #self.logger.debug('____DisplayThread: OOOOOOOOPS')
                return
            
            self.display.set('zoom to fit')

    def join(self, timeout=None):
        """Stop thread on request.
        """
        self.display.set('blink no')
        self.display.set('frame delete all')
        if self.debug: self.logger.debug('____DisplayThread: join, timeout {0}, stopping thread on request'.format(timeout))
        self.stoprequest.set()
        super(Ds9DisplayThread, self).join(timeout)





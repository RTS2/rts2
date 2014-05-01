#!/usr/bin/python
#
# (C) 2013, Markus Wildi
#
#   
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
"""Check if RTS2 devices are writable, print a summary of the focus run."
"""
__author__ = 'wildi.markus@bluewin.ch'

import sys
import os
import errno
from rts2saf.timeout import timeout

class CheckDevices(object):
    """Check the presence of the devices and filter slots.

    :var debug: enable more debug output with --debug and --level
    :var proxy: :py:mod:`rts2.proxy`
    :var blind: blind - flavor of focus run
    :var verbose: True, more debug output
    :var ccd: :py:mod:`rts2saf.devices.CCD`
    :var ftws: list of  :py:mod:`rts2saf.devices.FilterWheel`
    :var foc: :py:mod:`rts2saf.devices.Focuser`
    :var logger:  :py:mod:`rts2saf.log`

    """
  
    def __init__(self, debug=False, proxy=None, blind=None, verbose=None, ccd=None, ftws=None, foc=None, logger=None):
        self.debug=debug
        self.blind=blind
        self.verbose=verbose
        self.ccd=ccd
        self.ftws=ftws
        self.foc=foc
        self.logger=logger
        self.proxy= proxy 

    def summaryDevices(self):
        """Log a summary of the upcoming focus run.
        """
        # log INFO a summary, after dropping multiple empty slots
        img=0
        self.logger.info('summaryDevices: focus run without multiple empty slots:')
        for k, ftw in enumerate(self.ftws):
            # count only wheels with more than one filters (one slot must be empty)
            # the first filter wheel in the list 
            if ftw.name in 'FAKE_FTW' or len(ftw.filters)>1 or k==0: # at least an empty slot must be present 
                info = str()
                for ft in ftw.filters:
                    info += 'summaryDevices: {0:8s}: {1:8s}'.format(ftw.name, ft.name)
                    if self.blind:
                        info += '{0:2d} steps, between: [{1:5d},{2:5d}]\n'.format(len(self.foc.focToff), min(self.foc.focToff), max(self.foc.focToff))
                        img += len(self.foc.focToff)
                    else:
                        info += '{0:2d} steps, FOC_TOFF: [{1:5d}, {2:5d}], '.format(len(ft.focToff), min(ft.focToff), max(ft.focToff))
                        info += 'FOC_POS: [{1:5d},{2:5d}], FOC_DEF: {3:5d}, '.format(len(ft.focToff), self.foc.focDef + min(ft.focToff), self.foc.focDef + max(ft.focToff), self.foc.focDef)
                        info += 'Filter Offset: {0:5.0f}\n'.format(ft.OffsetToEmptySlot)
                        img += len(ft.focToff)
                else:
                    self.logger.info('{0}'.format(info))
            else:
                self.logger.info('summaryDevices: {0:8s}: {1} has only empty slots'.format(ftw.name, [x.name for x in ftw.filters]))
                

        self.logger.info('summaryDevices: taking {0} images in total'.format(img))
        

    def printProperties(self):
        """Log rts2saf device properties.
        """
        # Focuser
        self.logger.debug('printProperties: {} name'.format(self.foc.name))
        self.logger.debug('printProperties: {} resolution'.format(self.foc.resolution))
        self.logger.debug('printProperties: {} absLowerLimit'.format(self.foc.absLowerLimit))
        self.logger.debug('printProperties: {} absUpperLimit'.format(self.foc.absUpperLimit))
        self.logger.debug('printProperties: {} speed'.format(self.foc.speed))
        self.logger.debug('printProperties: {} stepSize'.format(self.foc.stepSize))
        self.logger.debug('printProperties: {} temperatureCompensation'.format(self.foc.temperatureCompensation))
        if self.blind:
            self.logger.debug('printProperties: focToff, steps: {0}, between: {1} and {2}'.format(len(self.foc.focToff), min(self.foc.focToff), max(self.foc.focToff)))
        else:
            self.logger.debug('printProperties: focToff, set by filters if not --blind is specified')
            
        self.logger.debug('printProperties: {} focDef'.format(self.foc.focDef))
        self.logger.debug('printProperties: {} focMn'.format(self.foc.focMn))
        self.logger.debug('printProperties: {} focMx'.format(self.foc.focMx))
        # CCD
        self.logger.debug('')
        self.logger.debug('printProperties:CCD: {} name'.format(self.ccd.name))
        self.logger.debug('printProperties:CCD: {} binning'.format(self.ccd.binning))
        self.logger.debug('printProperties:CCD: {} window'.format(self.ccd.window))
        self.logger.debug('printProperties:CCD: {} pixelSize'.format(self.ccd.pixelSize))
        self.logger.debug('printProperties:CCD: {} baseExposure'.format(self.ccd.baseExposure))
        #Filter

    @timeout(seconds=10, error_message=os.strerror(errno.ETIMEDOUT))
    def __deviceWriteAccessCCD(self):
        ccdOk=False
        if self.verbose: self.logger.debug('__deviceWriteAccessCCD: asking   from {0}: calculate_stat'.format(self.ccd.name))
        cs=self.proxy.getDevice(self.ccd.name)['calculate_stat'][1]
        if self.verbose: self.logger.debug('__deviceWriteAccessCCD: response from {0}: calculate_stat: {1}'.format(self.ccd.name, cs))
        
        try:
            self.proxy.setValue(self.ccd.name,'calculate_stat', 3) # no statistics
            self .proxy.setValue(self.ccd.name,'calculate_stat', str(cs))
            ccdOk= True
        except Exception, e:
            self.logger.error('__deviceWriteAccessCCD: CCD {0} is not writable: {1}'.format(self.ccd.name, repr(e)))

        if ccdOk:
            self.logger.debug('__deviceWriteAccessCCD: CCD {} is writable'.format(self.ccd.name))
            
        return ccdOk
    @timeout(seconds=2, error_message=os.strerror(errno.ETIMEDOUT))
    def __deviceWriteAccessFoc(self):
        focOk=False
        focDef=self.proxy.getDevice(self.foc.name)['FOC_DEF'][1]
        try:
            self.proxy.setValue(self.foc.name,'FOC_DEF', focDef+1)
            self.proxy.setValue(self.foc.name,'FOC_DEF', focDef)
            focOk= True
        except Exception, e:
            self.logger.error('__deviceWriteAccessFoc: {0} is not writable: {1}'.format(self.foc.name, repr(e)))
        if focOk:
            self.logger.debug('__deviceWriteAccessFoc: {0} is writable'.format(self.foc.name))

        return focOk

    @timeout(seconds=30, error_message=os.strerror(errno.ETIMEDOUT))
    def __deviceWriteAccessFtw(self):
        ftwOk=False
        if self.verbose: self.logger.debug('__deviceWriteAccessFtw: asking   from Ftw: {0}: filter'.format(self.ftws[0].name))
        ftnr=  self.proxy.getSingleValue(self.ftws[0].name, 'filter')
        if self.verbose: self.logger.debug('__deviceWriteAccessFtw: response from Ftw: {0}: filter: {1}'.format(self.ftws[0].name, ftnr))
        ftnrp1= str(ftnr +1)
        try:
            if self.verbose: self.logger.debug('__deviceWriteAccessFtw: setting  Ftw: {0}: to filter: {1}'.format(self.ftws[0].name, ftnrp1))
            self.proxy.setValue(self.ftws[0].name, 'filter',  ftnrp1)
            if self.verbose: self.logger.debug('__deviceWriteAccessFtw: setting  Ftw: {0}: to filter: {1}'.format(self.ftws[0].name, ftnr))
            self.proxy.setValue(self.ftws[0].name, 'filter',  str(ftnr))
            ftwOk= True
        except Exception, e:
            if e:
                self.logger.error('__deviceWriteAccessFtw: {0} is not writable: {1}'.format(self.ftws[0].name, repr(e)))
            else:
                self.logger.error('__deviceWriteAccessFtw: {0} is not writable (not timed out)'.format(self.ftws[0].name))
        if ftwOk:
            self.logger.debug('__deviceWriteAccessFtw: {0} is writable'.format(self.ftws[0].name))

        return ftwOk

    def deviceWriteAccess(self):
        """Check if RTS2 devices are writable through RTS2 JSON.
        """

        self.logger.info('deviceWriteAccess: this may take approx. a minute')

        ccdOk=ftwOk=focOk=False

        ccdOk=self.__deviceWriteAccessCCD()
        focOk=self.__deviceWriteAccessFoc()
        if self.ftws[0].name in 'FAKE_FTW':
            ftwOk=True
        else:
            ftwOk=self.__deviceWriteAccessFtw()

        if  ccdOk and ftwOk and focOk:
            self.logger.info('deviceWriteAccess: all devices are writable')

        return ccdOk and ftwOk and focOk
        

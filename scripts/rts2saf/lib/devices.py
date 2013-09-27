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

__author__ = 'wildi.markus@bluewin.ch'

import sys
import time
import re
import os
import errno
from rts2.json import JSONProxy
try:
    from lib.timeout import timeout
except:
    from timeout import timeout

class Filter():
    """Class for filter properties"""
    def __init__(self, name=None, OffsetToEmptySlot=None, lowerLimit=None, upperLimit=None, stepSize=None, exposureFactor=1., focFoff=None):
        self.name= name
        self.OffsetToEmptySlot= OffsetToEmptySlot# [tick]
        self.relativeLowerLimit= lowerLimit# [tick]
        self.relativeUpperLimit= upperLimit# [tick]
        self.exposureFactor   = exposureFactor 
        self.stepSize  = stepSize # [tick]
        self.focFoff=focFoff # range

class FilterWheel():
    """Class for filter wheel properties"""
    def __init__(self, name=None, filterOffsets=list(), filters=list()):
        self.name= name
        self.filterOffsets=filterOffsets # from CCD
        self.filters=filters # list of Filter
        self.emptySlots=None # set at run time ToDo go away??

class Focuser():
    """Class for focuser properties"""
    def __init__(self, name=None, resolution=None, absLowerLimitFb=None, absUpperLimitFb=None, speed=None, stepSize=None, temperatureCompensation=None, steps=list()):
        self.name= name
        self.resolution=resolution 
        self.absLowerLimitFb=absLowerLimitFb
        self.absUpperLimitFb=absUpperLimitFb
        self.speed=speed
        self.stepSize=stepSize
        self.temperatureCompensation=temperatureCompensation
        self.focFoff=None # will be set at run time
        self.focDef=None # will be set at run time
        self.focMn=None # will be set at run time
        self.focMx=None # will be set at run time
        self.focSt=None # focstep, will be set at run time

class CCD():
    """Class for CCD properties"""
    def __init__(self, name=None, binning=None, windowOffsetX=None, windowOffsetY=None, windowHeight=None, windowWidth=None, pixelSize=None, baseExposure=None):
        
        self.name= name
        self.binning=binning
        self.windowOffsetX=windowOffsetX
        self.windowOffsetY=windowOffsetY
        self.windowHeight=windowHeight
        self.windowWidth=windowWidth
        self.pixelSize= pixelSize
        self.baseExposure= baseExposure
        self.setFocDef=False

class CheckDevices(object):
    """Check the presence of the devices and filter slots"""    
    def __init__(self, debug=False, args=None, rt=None, logger=None):
        self.debug=debug
        self.args=args
        self.rt=rt
        self.logger=logger
        self.filterWheesFromCCD=list()
        self.proxy= JSONProxy(url=self.rt.cfg['URL'],username=self.rt.cfg['USERNAME'],password=self.rt.cfg['PASSWORD'])
        self.connected=False
        self.errorMessage=None
        # no return value here
        try:
            self.proxy.refresh()
            self.connected=True
        except Exception, e:
            self.errorMessage=e

    def __camera(self):
        if not self.connected:
            self.logger.error('checkDevices: no connection to XMLRPCd'.format(self.rt.ccd.name))        
            self.logger.info('acquire: set in rts2.ini, section [xmlrpcd]::auth_localhost = false')
            self.logger.info('acquire: user name, password correct?')
            return False

        self.proxy.refresh()
        try:
            self.proxy.getDevice(self.rt.ccd.name)
        except:
            self.logger.error('checkDevices: camera device: {0} not present'.format(self.rt.ccd.name))        
            return False
        if self.debug: self.logger.debug('checkDevices: camera: {0} present'.format(self.rt.ccd.name))

        if len(self.rt.filterWheelsInUse)==0:
            self.logger.info('checkDevices: no filter wheel in use')        
            return True

        # ToDo
        # interestingly on can set filter offsets for
        # filter_offsets_B
        # filter_offsets_C
        # but there is no way to specify it via cmd line
        # or --offsets-file
        #
        # filter_offsets_A, etc. see Bootes-2 device andor3
        #ftos=self.proxy.getValue(self.rt.ccd.name, 'filter_offsets_B')
        #print ftos
        #self.proxy.setValue(self.rt.ccd.name,'filter_offsets_B', '6 7 8')
        #self.proxy.refresh()
        #ftos=self.proxy.getValue(self.rt.ccd.name, 'filter_offsets_B')
        #print ftos
        #ftos=self.proxy.getValue(self.rt.ccd.name, 'filter_offsets_C')
        #print ftos
        #self.proxy.setValue(self.rt.ccd.name,'filter_offsets_C', '16 17 18')
        #self.proxy.refresh()
        #ftos=self.proxy.getValue(self.rt.ccd.name, 'filter_offsets_C')
        #print ftos
        #sys.exit(1)
        

        # Workaround for the above
        # Device camd provides the following variables
        # in case multiple filter wheels are present.
        # This example shows the configuration of Bootes-2:
        #
        # CCD andor3
        # FLI FTWs: COLWFLT, COLWSLT, COLWGRS
        # Variables appearing in tab andor3
        #  variable   ftw name   filters (read out from device, e.g. COLWGRS)
        #  wheel      COLWFLT  
        #  wheelA     COLWFLT    open, R, g, r, i, z, Y, empty8
        #  wheelB     COLWSLT    closed, slit1, slit2, slit3, slit4, h.., p.., open
        #  wheelC     COLWGRS    open, grism1, empty3, empty4, empty5, empty6, empty7, empty8 
        # 
        # Filters on wheels  wheelB, wheelC etc. which are 
        # named open or empty are treated as empty slots.
        # Only the filter wheel appearing in camd::wheel 
        # is used.
        # During a focus run filter wheels wheelB, wheelC etc. are
        # set to a slot named open or empty*
        #

        # first the wheel with the "real" filters
        fts=dict()
        ftos=dict()
        for i in range( 0, len(self.rt.filterWheelsInUse)):
            if i:
                ext= chr( 65 + i) # 'A' + i, as it is done in camd.cpp
                ftwn=self.proxy.getValue(self.rt.ccd.name, 'wheel{0}'.format(ext))
                # FILTA, FILTB, ...
                fts[ftwn] =self.proxy.getSelection(self.rt.ccd.name, 'FILT{0}'.format(ext))
                ftos[ftwn]=self.proxy.getValue(self.rt.ccd.name, 'filter_offsets_{0}'.format(ext))
            else:
                ftwn=self.proxy.getValue(self.rt.ccd.name, 'wheel')
                fts[ftwn] =self.proxy.getSelection(self.rt.ccd.name, 'filter')
                ftos[ftwn]=self.proxy.getValue(self.rt.ccd.name, 'filter_offsets')

            self.filterWheesFromCCD.append(ftwn)
        
        for ftwIU in self.rt.filterWheelsInUse: # ToDo this is/could be a glitch
            for ftwn in fts.keys():
                if ftwn in ftwIU.name:
                    if self.debug: self.logger.debug('checkDevices: found filter wheel: {0}'.format(ftwIU.name))
                    offsets=list()
                    if len(ftos[ftwn])==0:
                        self.logger.warn('checkDevices: for camera: {0} filter wheel: {1}, no filter offsets are defined, but filter wheels/filters are present'.format(self.rt.ccd.name, ftwn))        
                    else:
                        offsets= map(lambda x: x, ftos[ftwn]) 
                    # check filter wheel's filters in config and add offsets           
                    ftIUns= map( lambda x: x.name, ftwIU.filters)

                    for k, ftn in enumerate(fts[ftwn]): # names only
                        if not ftn in ftIUns:
                            self.logger.info('checkDevices: filter wheel: {0}, filter: {1} not used ignoring'.format(ftwIU.name, ftn))        
                            continue

                        for ft in self.rt.filters:
                            if ftn in ft.name:
                                
                                # ToDO that's not what I want
                                # e.g. filter name empty8 and in list: empty
                                match=False
                                for nm in self.rt.cfg['EMPTY_SLOT_NAMES']:
                                    # e.g. filter name R must exactly match!
                                    p = re.compile(nm)
                                    m = p.match(ft.name)
                                    if m:
                                        match=True
                                        break

                                if match:
                                    ft.OffsetToEmptySlot= 0.
                                    if self.debug: self.logger.debug('checkDevices: filter wheel: {0}, filter: {1} offset set to ZERO'.format(ftwIU.name, ft.name))
                                else:
                                    try:
                                        ft.OffsetToEmptySlot= offsets[k] 
                                        if self.debug: self.logger.debug('checkDevices: filter wheel: {0}, filter: {1} offset {2} from ccd: {3}'.format(ftwIU.name, ft.name, ft.OffsetToEmptySlot,self.rt.ccd.name))
                                    except:
                                        ft.OffsetToEmptySlot= 0.
                                        self.logger.warn('checkDevices: filter wheel: {0}, filter: {1} NO offset from ccd: {2}, setting it to ZERO'.format(ftwIU.name, ft.name,self.rt.ccd.name))
                                break
                        else:
                            self.logger.warn('checkDevices: filter wheel: {0}, filter: {1} not found in configuration, ignoring it'.format(ftwn, ftn))        
                            return False
                    break
            else:
                self.logger.error('checkDevices: filter wheel: {0} not found in configuration'.format(ftwIU.name))        
        return True

    def __focuser(self):
        if not self.connected:
            self.logger.error('checkDevices: no connection to XMLRPCd'.format(self.rt.ccd.name))        
            self.logger.info('acquire: set in rts2.ini, section [xmlrpcd]::auth_localhost = false')
            return False

        self.proxy.refresh()
        try:
            self.proxy.getDevice(self.rt.foc.name)
        except:
            self.logger.error('checkDevices: focuser device: {0} not present'.format(self.rt.foc.name))        
            return False
        if self.debug: self.logger.debug('checkDevices: focuser: {0} present'.format(self.rt.foc.name))

        try:
            self.rt.foc.focMn=self.proxy.getDevice(self.rt.foc.name)['foc_min'][1]
            self.rt.foc.focMx=self.proxy.getDevice(self.rt.foc.name)['foc_max'][1]
            #self.rt.foc.focSt=self.proxy.getDevice(self.rt.foc.name)['focstep'][1]
        except Exception, e:
            self.logger.warn('FocusFilterWheels: focuser: {0} has no foc_min or foc_max properties '.format(self.rt.foc.name))

        if self.rt.foc.focMn and self.rt.foc.focMx:
            self.rt.foc.focSt=self.rt.foc.stepSize
        else:
            self.rt.foc.focMn=self.rt.foc.absLowerLimitFb
            self.rt.foc.focMx=self.rt.foc.absUpperLimitFb
            self.rt.foc.focSt=self.rt.foc.stepSize
            self.logger.warn('FocusFilterWheels: setting foc_min: {0}, foc_max: {1}, focstep: {2}'.format(self.rt.foc.absLowerLimitFb, self.rt.foc.absUpperLimitFb, self.rt.foc.resolution))

        if self.args.blind:
            # ToDo: upper limit -abs(self.args.focStep) OK?
            if self.args.focRange:
                # ToDo check that
                withinLlUl=False
                if self.rt.foc.absLowerLimitFb < self.args.focRange[0] < self.rt.foc.absUpperLimitFb-(self.args.focRange[1]-self.args.focRange[0]):
                    if self.rt.foc.absLowerLimitFb + (self.args.focRange[1]-self.args.focRange[0]) < self.args.focRange[1] <  self.rt.foc.absUpperLimitFb:
                        withinLlUl=True
                if withinLlUl:
                    self.rt.foc.focFoff= range(self.args.focRange[0], self.args.focRange[1], abs(self.args.focRange[2]))
                else:
                    self.logger.error('FocusFilterWheels: out of bounds, exiting')
                    sys.exit(1)
            else:
                print self.rt.foc.focMn, self.rt.foc.focMx, self.rt.foc.focSt
                self.rt.foc.focFoff= range(int(self.rt.foc.focMn), int(self.rt.foc.focMx-abs(int(self.rt.foc.focSt))), abs(int(self.rt.foc.focSt)))

                if len(self.rt.foc.focFoff) > 10:
                    self.logger.info('FocusFilterWheels: focuser range has: {0} steps, you might consider to increase RTS2::focstep, or set decent value for --focrange'.format(len(self.rt.foc.focFoff)))

        return True
    # ToDo might go away
    def __filterWheels(self):
        if not self.connected:
            self.logger.error('checkDevices: no connection to XMLRPCd'.format(self.rt.ccd.name))        
            self.logger.info('acquire: set in rts2.ini, section [xmlrpcd]::auth_localhost = false')
            return False
        self.proxy.refresh()
        for ftw in self.rt.filterWheelsInUse:
            try:
                self.proxy.getDevice(ftw.name)
            except:
                self.logger.error('checkDevices: filter wheel device {0} not present'.format(ftw.name))        
                return False

        return True

    def statusDevices(self):
        # order matters:
        # read the filter offsets from the CCD and store them in Filter
        if not self.__focuser():
            return False
        if not self.__camera():
            return False
        if not self.__filterWheels():
            return False

        # find empty slots on all filter wheels
        # assumption: no combination of filters of the different filter wheels
        for ftw in self.rt.filterWheelsInUse:
            # first find in ftw.filters an empty slot 
            # use this slot first, followed by all others
            ftw.filters.sort(key=lambda x: x.OffsetToEmptySlot)
            for ft in ftw.filters:
                if self.args.debug: self.logger.debug('checkDevices: filter wheel: {0:5s}, filter:{1:5s} in use'.format(ftw.name, ft.name))
                if ft.OffsetToEmptySlot==0:
                    # ft.emptySlot=Null at instanciation
                    try:
                        ftw.emptySlots.append(ft)
                    except:
                        ftw.emptySlots=list()
                        ftw.emptySlots.append(ft)

                    if self.args.debug: self.logger.debug('checkDevices: filter wheel: {0:5s}, filter:{1:5s} is a candidate empty slot'.format(ftw.name, ft.name))

            if ftw.emptySlots:
                # drop multiple empty slots
                self.logger.info('checkDevices: filter wheel: {0}, empty slot:{1}'.format(ftw.name, ftw.emptySlots[0].name))

                for ft in ftw.emptySlots[1:]:
                    match=False
                    for nm in self.rt.cfg['EMPTY_SLOT_NAMES']:
                        # e.g. filter name R must exactly match!
                        p = re.compile(nm)
                        m = p.match(ft.name)
                        if m:
                            match= True
                            break
                    if m:
                        self.logger.info('checkDevices: filter wheel: {0}, dropping empty slot:{1}'.format(ftw.name, ft.name))
                        ftw.filters.remove(ft)
                    else:
                        if self.debug: self.logger.debug('checkDevices: filter wheel: {0}, NOT dropping slot:{1} with no offset=0 to empty slot'.format(ftw.name, ft.name))
                            
            else:
                # warn only if two or more ftws are used
                if len(self.rt.filterWheelsInUse) > 0:
                    self.logger.warn('checkDevices: filter wheel: {0}, no empty slot found'.format(ftw.name))

        # log INFO a summary, after dropping multiple empty slots
        self.logger.info('checkDevices: focus run summary, without multiple empty slots:')
        img=0
        ftwns= map( lambda x: x.name, self.rt.filterWheelsInUse) # need the first filter wheel
        for k, ftwn in enumerate(self.filterWheesFromCCD): # names
            ind= ftwns.index(ftwn)
            ftw= self.rt.filterWheelsInUse[ind]               
            # count only wheels with more than one filters (one slot must be empty)
            # the first filter wheel in the list 
            info = '\n'
            if len(ftw.filters)>1 or k==0:
                info = '{0:8s}:\n'.format(ftw.name)
                for ft in ftw.filters:
                    info += 'checkDevices: {0:8s}: '.format(ft.name)
                    if self.args.blind:
                        info += '{0:2d} steps, bewteen: {1:5d} and {2:5d}\n'.format(len(self.rt.foc.focFoff), min(self.rt.foc.focFoff), max(self.rt.foc.focFoff))
                        img += len(self.rt.foc.focFoff)
                    else:
                        info += '{0:2d} steps, bewteen: {1:5d} and {2:5d} (FOC_FOFF)\n'.format(len(ft.focFoff), min(ft.focFoff), max(ft.focFoff))
                        img += len(ft.focFoff)
                else:
                    self.logger.info('checkDevices: {0}'.format(info))
                
        self.logger.info('checkDevices: taking {0} images in total'.format(img))
        return True

    def printProperties(self):
        # Focuser
        self.logger.debug('checkDevices:Focuser: {} name'.format(self.rt.foc.name))
        self.logger.debug('checkDevices:Focuser: {} resolution'.format(self.rt.foc.resolution))
        self.logger.debug('checkDevices:Focuser: {} absLowerLimitFb'.format(self.rt.foc.absLowerLimitFb))
        self.logger.debug('checkDevices:Focuser: {} absUpperLimitFb'.format(self.rt.foc.absUpperLimitFb))
        self.logger.debug('checkDevices:Focuser: {} speed'.format(self.rt.foc.speed))
        self.logger.debug('checkDevices:Focuser: {} stepSize'.format(self.rt.foc.stepSize))
        self.logger.debug('checkDevices:Focuser: {} temperatureCompensation'.format(self.rt.foc.temperatureCompensation))
        if self.args.blind:
            self.logger.debug('checkDevices:Focuser: focFoff, steps: {0}, between: {1} and {2}'.format(len(self.rt.foc.focFoff), min(self.rt.foc.focFoff), max(self.rt.foc.focFoff)))
        else:
            self.logger.debug('checkDevices:Focuser: focFoff, set by filters if --blind is specified')
            
        self.logger.debug('checkDevices:Focuser: {} focDef'.format(self.rt.foc.focDef))
        self.logger.debug('checkDevices:Focuser: {} focMn'.format(self.rt.foc.focMn))
        self.logger.debug('checkDevices:Focuser: {} focMx'.format(self.rt.foc.focMx))
        self.logger.debug('checkDevices:Focuser: {} focSt'.format(self.rt.foc.focSt))
        # CCD
        self.logger.debug('')
        self.logger.debug('checkDevices:CCD: {} name'.format(self.rt.ccd.name))
        self.logger.debug('checkDevices:CCD: {} binning'.format(self.rt.ccd.binning))
        self.logger.debug('checkDevices:CCD: {} windowOffsetX'.format(self.rt.ccd.windowOffsetX))
        self.logger.debug('checkDevices:CCD: {} windowOffsetY'.format(self.rt.ccd.windowOffsetY))
        self.logger.debug('checkDevices:CCD: {} windowHeight'.format(self.rt.ccd.windowHeight))
        self.logger.debug('checkDevices:CCD: {} windowWidth'.format(self.rt.ccd.windowWidth))
        self.logger.debug('checkDevices:CCD: {} pixelSize'.format(self.rt.ccd.pixelSize))
        self.logger.debug('checkDevices:CCD: {} baseExposure'.format(self.rt.ccd.baseExposure))
        self.logger.debug('checkDevices:CCD: {} setFocDef'.format(self.rt.ccd.setFocDef))
        #Filter
        self.logger.debug('')
        
        self.logger.debug('checkDevices:FTWs: {} number'.format(len(self.rt.filterWheelsInUse)))
        for ftw in self.rt.filterWheelsInUse:
            for ft in ftw.filters:
                self.logger.debug('checkDevices:{0}: {1}'.format(ftw.name, ft.name))
                self.logger.debug('checkDevices:{0}: focFoff, steps: {1}, between: {2} and {3}'.format(ft.name, len(ft.focFoff), min(ft.focFoff), max(ft.focFoff)))

    @timeout(seconds=1, error_message=os.strerror(errno.ETIMEDOUT))
    def __deviceWriteAccessCCD(self):
        ccdOk=False
        if self.args.verbose: self.logger.debug('checkDevices: asking   from CCD: {0}: calculate_stat'.format(self.rt.ccd.name))
        cs=self.proxy.getDevice(self.rt.ccd.name)['calculate_stat'][1]
        if self.args.verbose: self.logger.debug('checkDevices: response from CCD: {0}: calculate_stat: {1}'.format(self.rt.ccd.name, cs))
        
        try:
            self.proxy.setValue(self.rt.ccd.name,'calculate_stat', 3) # no statisctics
            self .proxy.setValue(self.rt.ccd.name,'temp_max', str(cs))
            ccdOk= True
        except Exception, e:
            self.logger.error('checkDevices: CCD: {0} is not writable: {1}'.format(self.rt.ccd.name, repr(e)))

        if ccdOk:
            self.logger.debug('checkDevices: CCD: {} is writable'.format(self.rt.ccd.name))
            
        return ccdOk
    @timeout(seconds=2, error_message=os.strerror(errno.ETIMEDOUT))
    def __deviceWriteAccessFoc(self):
        focOk=False
        focDef=self.proxy.getDevice(self.rt.foc.name)['FOC_DEF'][1]
        try:
            self.proxy.setValue(self.rt.foc.name,'FOC_DEF', focDef+1)
            self.proxy.setValue(self.rt.foc.name,'FOC_DEF', focDef)
            focOk= True
        except Exception, e:
            self.logger.error('checkDevices: focuser: {0} is not writable: {1}'.format(self.rt.foc.name, repr(e)))
        if focOk:
            self.logger.debug('checkDevices: focuser: {} is writable'.format(self.rt.foc.name))

        return focOk

    @timeout(seconds=30, error_message=os.strerror(errno.ETIMEDOUT))
    def __deviceWriteAccessFtw(self):
        ftwOk=False
        if self.args.verbose: self.logger.debug('checkDevices: asking   from Ftw: {0}: filter'.format(self.rt.filterWheelsInUse[0].name))
        ftnr=  self.proxy.getSingleValue(self.rt.filterWheelsInUse[0].name, 'filter')
        if self.args.verbose: self.logger.debug('checkDevices: response from Ftw: {0}: filter: {1}'.format(self.rt.filterWheelsInUse[0].name, ftnr))
        ftnrp1= str(ftnr +1)
        try:
            if self.args.verbose: self.logger.debug('checkDevices: setting  Ftw: {0}: to filter: {1}'.format(self.rt.filterWheelsInUse[0].name, ftnrp1))
            self.proxy.setValue(self.rt.filterWheelsInUse[0].name, 'filter',  ftnrp1)
            if self.args.verbose: self.logger.debug('checkDevices: setting  Ftw: {0}: to filter: {1}'.format(self.rt.filterWheelsInUse[0].name, ftnr))
            self.proxy.setValue(self.rt.filterWheelsInUse[0].name, 'filter',  ftnr)
            ftwOk= True
        except Exception, e:
            if e:
                self.logger.error('checkDevices: filter wheel: {0} is not writable: {1}'.format(self.rt.filterWheelsInUse[0].name, repr(e)))
            else:
                self.logger.error('checkDevices: filter wheel: {0} is not writable (not timed out)'.format(self.rt.filterWheelsInUse[0].name))
        if ftwOk:
            self.logger.debug('checkDevices: filter wheel: {} is writable'.format(self.rt.filterWheelsInUse[0].name))

        return ftwOk

    def deviceWriteAccess(self):
        self.logger.info('checkDevices:deviceWriteAccess: this may take approx. a minute')

        ccdOk=ftwOk=focOk=False

        ccdOk=self.__deviceWriteAccessCCD()
        focOk=self.__deviceWriteAccessFoc()
        ftwOk=self.__deviceWriteAccessFtw()

        return ccdOk and ftwOk and focOk

        
if __name__ == '__main__':

    import argparse
    try:
        import lib.devices as dev
    except:
        import devices as dev
    try:
        import lib.log as  lg
    except:
        import log as lg
    try:
        import lib.config as cfgd
    except:
        import config as cfgd

    parser= argparse.ArgumentParser(prog=sys.argv[0], description='rts2asaf check devices')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--logfile',dest='logfile', default='/tmp/{0}.log'.format(sys.argv[0]), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--config', dest='config', action='store', default='/etc/rts2/rts2saf/rts2saf.cfg', help=': %(default)s, configuration file path')
    parser.add_argument('--verbose', dest='verbose', action='store_true', default=False, help=': %(default)s, print device properties and add more messages')
    parser.add_argument('--checkwrite', dest='checkWrite', action='store_true', default=False, help=': %(default)s, check if devices are writable')
    parser.add_argument('--focrange', dest='focRange', action='store', default=None,type=int, nargs='+', help=': %(default)s, focuser range given as "ll ul st" used only during blind run')
    parser.add_argument('--blind', dest='blind', action='store_true', default=False, help=': %(default)s, focus range and step size are defined in configuration, if --focrange is defined it is used to set the range')

    args=parser.parse_args()

    if args.verbose:
        args.level='DEBUG'
        args.debug=True
        args.toconsole=True

    lgd= lg.Logger(debug=args.debug, args=args) # if you need to chage the log format do it here
    logger= lgd.logger 

    if args.blind and not args.focRange:
        logger.info('checkDevices: --blind is set, recommendation: set --focrange to decent value')        

    if args.focRange:
        if (args.focRange[0] >= args.focRange[1]) or args.focRange[2] <= 0: 
            logger.error('checkDevices: bad range values: {}, exiting'.format(args.focRange))
            sys.exit(1)

    rt=cfgd.Configuration(logger=logger)
    rt.readConfiguration(fileName=args.config)

    cdv= CheckDevices(debug=args.debug, args=args, rt=rt, logger=logger)
    if not cdv.statusDevices():
        logger.error('checkDevices:  exiting')
        sys.exit(1)

    if args.verbose:
        cdv.printProperties()

    if args.checkWrite:
        if not cdv.deviceWriteAccess():
            logger.error('checkDevices:  exiting')
            sys.exit(1)
    else:
        logger.info('checkDevices: skiped check if devices are writable, enable with --checkwrite')        

    logger.info('checkDevices: DONE')        
    

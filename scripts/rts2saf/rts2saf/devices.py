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
from rts2saf.timeout import timeout

class Filter(object):
    """Class for filter properties"""
    def __init__(self, name=None, OffsetToEmptySlot=None, lowerLimit=None, upperLimit=None, stepSize=None, exposureFactor=1., focFoff=None):
        self.name= name
        self.OffsetToEmptySlot= OffsetToEmptySlot# [tick]
        self.relativeLowerLimit= lowerLimit# [tick]
        self.relativeUpperLimit= upperLimit# [tick]
        self.exposureFactor   = exposureFactor 
        self.stepSize  = stepSize # [tick]
        self.focFoff=focFoff # range

class FilterWheel(object):
    """Class for filter wheel properties"""
    def __init__(self, name=None, filterOffsets=list(), filters=list()):
        self.name= name
        self.filterOffsets=filterOffsets # from CCD
        self.filters=filters # list of Filter
        self.emptySlots=None # set at run time ToDo go away??

class Focuser(object):
    """Class for focuser properties"""
    def __init__(self, name=None, resolution=None, absLowerLimit=None, absUpperLimit=None, lowerLimit=None, upperLimit=None, stepSize=None, speed=None, focNoFtwrRange=None, temperatureCompensation=None, focFoff=None):
        self.name= name
        self.resolution=resolution 
        self.absLowerLimit=absLowerLimit
        self.absUpperLimit=absUpperLimit
        self.lowerLimit=lowerLimit
        self.upperLimit=upperLimit
        self.stepSize=stepSize
        self.speed=speed
        self.focNoFtwrRange=focNoFtwrRange
        self.temperatureCompensation=temperatureCompensation
        self.focFoff=focFoff # will be set at run time
        self.focDef=None # will be set at run time
        self.focMn=None # will be set at run time
        self.focMx=None # will be set at run time

class CCD(object):
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

class SetCheckDevices(object):
    """Check the presence of the devices and filter slots"""    
    def __init__(self, debug=False, rangeFocToff=None, blind=None, verbose=None, rt=None, logger=None):
        self.debug=debug
        self.rangeFocToff=rangeFocToff
        self.blind=blind
        self.verbose=verbose
        self.rt=rt
        self.logger=logger
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
            self.logger.error('setCheckDevices: no connection to XMLRPCd'.format(self.rt.ccd.name))        
            self.logger.info('devices: check if XMLRPCd is running, if yes set in rts2.ini, section [xmlrpcd]::auth_localhost = false')
            self.logger.info('devices: user name, password correct?')
            return False

        self.proxy.refresh()
        try:
            self.proxy.getDevice(self.rt.ccd.name)
        except:
            self.logger.error('setCheckDevices: camera device: {0} not present'.format(self.rt.ccd.name))        
            return False
        if self.debug: self.logger.debug('setCheckDevices: camera: {0} present'.format(self.rt.ccd.name))
        # ToDo no meaningless go away
        if len(self.rt.filterWheelsInUse)==0:
            self.logger.info('setCheckDevices: no filter wheel in use')        
            return True

        # There is no real filter wheel
        for ftwIU in self.rt.filterWheelsInUse: 
            if ftwIU.name in 'FAKE_FTW':
                if self.debug: self.logger.debug('setCheckDevices: using FAKE_FTW')        
                #  OffsetToEmptySlot set in config.py
                return True
        # check presence of a filter wheel
        try:
            ftwn=self.proxy.getValue(self.rt.ccd.name, 'wheel')
        except Exception, e:
            self.logger.error('setCheckDevices: no filter wheel present')
            return False


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

        for ftwIU in self.rt.filterWheelsInUse: # ToDo this is/could be a glitch
            for ftwn in fts.keys():
                if ftwn in ftwIU.name:
                    if self.debug: self.logger.debug('setCheckDevices: found filter wheel: {0}'.format(ftwIU.name))
                    offsets=list()
                    if len(ftos[ftwn])==0:
                        self.logger.warn('setCheckDevices: for camera: {0} filter wheel: {1}, no filter offsets are defined, but filter wheels/filters are present'.format(self.rt.ccd.name, ftwn))        
                    else:
                        offsets= map(lambda x: x, ftos[ftwn]) 
                    # check filter wheel's filters in config and add offsets           
                    ftIUns= map( lambda x: x.name, ftwIU.filters)

                    for k, ftn in enumerate(fts[ftwn]): # names only
                        if not ftn in ftIUns:
                            if self.debug: self.logger.debug('setCheckDevices: filter wheel: {0}, filter: {1} not used ignoring'.format(ftwIU.name, ftn))        
                            continue

                        for ft in self.rt.filters:
                            if ftn in ft.name:
                                
                                # ToDO that's not what I want
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
                                    if self.debug: self.logger.debug('setCheckDevices: filter wheel: {0}, filter: {1} offset set to ZERO'.format(ftwIU.name, ft.name))
                                else:
                                    try:
                                        ft.OffsetToEmptySlot= offsets[k] 
                                        if self.debug: self.logger.debug('setCheckDevices: filter wheel: {0}, filter: {1} offset {2} from ccd: {3}'.format(ftwIU.name, ft.name, ft.OffsetToEmptySlot,self.rt.ccd.name))
                                    except:
                                        ft.OffsetToEmptySlot= 0.
                                        self.logger.warn('setCheckDevices: filter wheel: {0}, filter: {1} NO offset from ccd: {2}, setting it to ZERO'.format(ftwIU.name, ft.name,self.rt.ccd.name))
                                break
                        else:
                            if self.debug: self.logger.debug('setCheckDevices: filter wheel: {0}, filter: {1} not found in configuration, ignoring it'.format(ftwn, ftn))        
                            return False
                    break
            else:
                self.logger.error('setCheckDevices: filter wheel: {0} not found in configuration'.format(ftwIU.name))        
        return True

    def __focuser(self):
        if not self.connected:
            self.logger.error('setCheckDevices: no connection to XMLRPCd'.format(self.rt.ccd.name))        
            self.logger.info('setCheckDevices: set in rts2.ini, section [xmlrpcd]::auth_localhost = false')
            return False

        self.proxy.refresh()
        try:
            self.proxy.getDevice(self.rt.foc.name)
        except:
            self.logger.error('setCheckDevices: focuser device: {0} not present'.format(self.rt.foc.name))        
            return False
        if self.debug: self.logger.debug('setCheckDevices: focuser: {0} present'.format(self.rt.foc.name))

        try:
            self.rt.foc.focDef=int(self.proxy.getDevice(self.rt.foc.name)['FOC_DEF'][1])
        except Exception, e:
            self.logger.warn('__focuser: focuser: {0} has no FOC_DEF set '.format(self.rt.foc.name))

        focMin=focMax=None
        try:
            focMin= self.proxy.getDevice(self.rt.foc.name)['foc_min'][1]
            focMax= self.proxy.getDevice(self.rt.foc.name)['foc_max'][1]
        except Exception, e:
            self.logger.warn('__focuser: focuser: {0} has no foc_min or foc_max properties '.format(self.rt.foc.name))

        rangeMin=rangeMax=rangeStep=None
        # focRange (from args) has priority
        if self.rangeFocToff:
            rangeMin= self.rangeFocToff[0] + self.rt.foc.focDef
            rangeMax= self.rangeFocToff[1] + self.rt.foc.focDef
            rangeStep= abs(self.rangeFocToff[2])
            self.logger.info('__focuser: focuser: {0} setting internal limits from arguments'.format(self.rt.foc.name))

        else:
            rangeMin=self.rt.foc.lowerLimit
            rangeMax=self.rt.foc.upperLimit
            rangeStep=self.rt.foc.stepSize
            self.logger.info('__focuser: focuser: {0} setting internal limits from configuration file and ev. default values!'.format(self.rt.foc.name))

        self.logger.info('__focuser: focuser: {0} setting internal limits to:[{1}, {2}], step size: {3}'.format(self.rt.foc.name, rangeMin, rangeMax, rangeStep))

        withinLlUl=False
        # ToDo check that against HW limits
        if self.rt.foc.absLowerLimit <= rangeMin <= self.rt.foc.absUpperLimit-(rangeMax-rangeMin):
            if self.rt.foc.absLowerLimit + (rangeMax-rangeMin) <= rangeMax <=  self.rt.foc.absUpperLimit:
                withinLlUl=True

        if withinLlUl:
            self.rt.foc.focFoff= range(rangeMin, rangeMax +rangeStep, rangeStep)
            if len(self.rt.foc.focFoff) <= self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
                self.logger.warn('__focuser: to few focuser positions: {0}<={1} (see MINIMUM_FOCUSER_POSITIONS)'.format(len(self.rt.foc.focFoff), self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']))

        else:
            self.logger.error('__focuser: focuser: {} abs. lowerLimit: {}, abs. upperLimit: {}; range rangeMin: {} rangeMax: {}, configuartion: **step size**: {}, min: {}, max: {}'.format(self.rt.foc.name, self.rt.foc.absLowerLimit, self.rt.foc.absUpperLimit, rangeMin, rangeMax, self.rt.foc.stepSize, self.rt.foc.lowerLimit, self.rt.foc.upperLimit))
            self.logger.info('__focuser: focuser: {0} FOC_DEF: {1}'.format(self.rt.foc.name, self.rt.foc.focDef))
            self.logger.error('__focuser: out of bounds, returning')
            return False

        # set the internal limits
        self.rt.foc.focMn= rangeMin
        self.rt.foc.focMx= rangeMax

        # ToDO check with no filter wheel configuration
        if len(self.rt.foc.focFoff) > 10 and  self.blind:
            self.logger.info('__focuser: focuser range has: {0} steps, you might consider set decent value for --focrange'.format(len(self.rt.foc.focFoff)))

        return True

    def __filterWheels(self):
        if not self.connected:
            self.logger.error('__filterWheels: no connection to XMLRPCd'.format(self.rt.ccd.name))        
            self.logger.info('__filterWheels: set in rts2.ini, section [xmlrpcd]::auth_localhost = false')
            return False

        # There is no real filter wheel
        for ftwIU in self.rt.filterWheelsInUse: 
            if ftwIU.name in 'FAKE_FTW':
                if self.debug: self.logger.debug('__filterWheels: using FAKE_FTW')        
                return True


        self.proxy.refresh()
        # find empty slots on all filter wheels
        # assumption: no combination of filters of the different filter wheels
        eSs=0
        for ftw in self.rt.filterWheelsInUse:
            try:
                self.proxy.getDevice(ftw.name)
            except:
                self.logger.error('__filterWheels: filter wheel device {0} not present'.format(ftw.name))        
                return False

            # first find in ftw.filters an empty slot 
            # use this slot first, followed by all others
            ftw.filters.sort(key=lambda x: x.OffsetToEmptySlot)
            for ft in ftw.filters:
                if self.debug: self.logger.debug('__filterWheels: filter wheel: {0:5s}, filter:{1:5s} in use'.format(ftw.name, ft.name))
                if ft.OffsetToEmptySlot==0:
                    # ft.emptySlot=Null at instanciation
                    try:
                        ftw.emptySlots.append(ft)
                    except:
                        ftw.emptySlots=list()
                        ftw.emptySlots.append(ft)

                    eSs += 1
                                            
                    if self.debug: self.logger.debug('__filterWheels: filter wheel: {0:5s}, filter:{1:5s} is a candidate empty slot'.format(ftw.name, ft.name))

            if ftw.emptySlots:
                # drop multiple empty slots
                self.logger.info('__filterWheels: filter wheel: {0}, empty slot:{1}'.format(ftw.name, ftw.emptySlots[0].name))

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
                        self.logger.info('__filterWheels: filter wheel: {0}, dropping empty slot:{1}'.format(ftw.name, ft.name))
                        ftw.filters.remove(ft)
                    else:
                        if self.debug: self.logger.debug('__filterWheels: filter wheel: {0}, NOT dropping slot:{1} with no offset=0 to empty slot'.format(ftw.name, ft.name))
                            
            else:
                # warn only if two or more ftws are used
                if len(self.rt.filterWheelsInUse) > 0:
                    self.logger.warn('__filterWheels: filter wheel: {0}, no empty slot found'.format(ftw.name))

        if eSs >= len(self.rt.filterWheelsInUse):
            if self.debug: self.logger.debug('__filterWheels: all filter wheels have an empty slot')
        else:
            self.logger.warn('__filterWheels: not all filter wheels have an empty slot, {}, {}'.format(eSs,  len(self.rt.filterWheelsInUse)))

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

        anyBelow=False
        # check MINIMUM_FOCUSER_POSITIONS
        for k, ftw in enumerate(self.rt.filterWheelsInUse):
            for ft in ftw.filters:
                if len(ft.focFoff) <= self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
                    self.logger.error( 'checkDevices: {0:8s}, filter: {1:8s} to few focuser positions: {2}<={3} (see MINIMUM_FOCUSER_POSITIONS)'.format(ftw.name, ft.name, len(ft.focFoff), self.rt.cfg['MINIMUM_FOCUSER_POSITIONS'])) 
                    anyBelow=True

        if anyBelow:
            self.logger.error('statusDevices: too few focuser positions, returning')
            return False

        # check bounds of filter lower and upper limit settings            
        anyOutOfLlUl=False
        for k, ftw in enumerate(self.rt.filterWheelsInUse):
            if len(ftw.filters)>1 or k==0:
                for ft in ftw.filters:
                    if self.blind:
                        # focuser limits are done in method __focuser()
                        pass
                    else:
                        rangeMin=self.rt.foc.focDef + min(ft.focFoff)
                        rangeMax=self.rt.foc.focDef + max(ft.focFoff)
                        outOfLlUl=True 
                        if self.rt.foc.absLowerLimit <= rangeMin <= self.rt.foc.absUpperLimit-(rangeMax-rangeMin):
                            if self.rt.foc.absLowerLimit + (rangeMax-rangeMin) <= rangeMax <=  self.rt.foc.absUpperLimit:
                                outOfLlUl=False

                        if outOfLlUl:
                            self.logger.error( 'checkDevices: {0:8s}, filter: {1:8s}, focuser: {2}, settings: abs. lowerLimit: {3}, abs. upperLimit: {4}; range rangeMin: {5} rangeMax: {6}, configuartion: **step size**: {7}, rel.min: {8}, rel.max: {9}'.format(ftw.name, ft.name, self.rt.foc.name, self.rt.foc.absLowerLimit, self.rt.foc.absUpperLimit, rangeMin, rangeMax, ft.stepSize, ft.relativeLowerLimit, ft.relativeUpperLimit))  
                            anyOutOfLlUl=True 

        if anyOutOfLlUl:
            self.logger.error('statusDevices: out of bounds, returning')
            self.logger.info('statusDevices: focuser: {0} FOC_DEF: {1}'.format(self.rt.foc.name, self.rt.foc.focDef))
            self.logger.info('statusDevices: check setting of FOC_DEF, FOC_FOFF, FOC_TOFF and limits in configuration')
            return False

        return True

    def summaryDevices(self):
        # log INFO a summary, after dropping multiple empty slots
        img=0
        # ToDo first part will go away
        if len(self.rt.filterWheelsInUse)==0:
            self.logger.info('summaryDevices: focus run summary, no filter wheel in use:')
            self.logger.info('summaryDevices: focuser: {0}, steps: {1}, min: {2}, max: {3}'.format(self.rt.foc.name, len(self.rt.foc.focFoff), min(self.rt.foc.focFoff), max(self.rt.foc.focFoff)))
            img=len(self.rt.foc.focFoff)

        else:
            self.logger.info('summaryDevices: focus run summary, without multiple empty slots:')
            for k, ftw in enumerate(self.rt.filterWheelsInUse):
                # count only wheels with more than one filters (one slot must be empty)
                # the first filter wheel in the list 
                if len(ftw.filters)>1 or k==0:
                    info = str()
                    for ft in ftw.filters:
                        info += 'checkDevices: {0:8s}: {1:8s}'.format(ftw.name, ft.name)
                        if self.blind:
                            info += '{0:2d} steps, between: {1:5d} and {2:5d}\n'.format(len(self.rt.foc.focFoff), min(self.rt.foc.focFoff), max(self.rt.foc.focFoff))
                            img += len(self.rt.foc.focFoff)
                        else:
                            info += '{0:2d} steps, FOC_FOFF between: {1:5d} and {2:5d}, '.format(len(ft.focFoff), min(ft.focFoff), max(ft.focFoff))
                            info += 'FOC_POS between: {1:5d} and {2:5d}, FOC_DEF: {3:5d}\n'.format(len(ft.focFoff), self.rt.foc.focDef + min(ft.focFoff), self.rt.foc.focDef + max(ft.focFoff),self.rt.foc.focDef)
                            img += len(ft.focFoff)
                    else:
                        self.logger.info('\n{0}'.format(info))

        self.logger.info('summaryDevices: taking {0} images in total'.format(img))
        

    def printProperties(self):
        # Focuser
        self.logger.debug('printProperties:Focuser: {} name'.format(self.rt.foc.name))
        self.logger.debug('printProperties:Focuser: {} resolution'.format(self.rt.foc.resolution))
        self.logger.debug('printProperties:Focuser: {} absLowerLimit'.format(self.rt.foc.absLowerLimit))
        self.logger.debug('printProperties:Focuser: {} absUpperLimit'.format(self.rt.foc.absUpperLimit))
        self.logger.debug('printProperties:Focuser: {} speed'.format(self.rt.foc.speed))
        self.logger.debug('printProperties:Focuser: {} stepSize'.format(self.rt.foc.stepSize))
        self.logger.debug('printProperties:Focuser: {} temperatureCompensation'.format(self.rt.foc.temperatureCompensation))
        if self.blind:
            self.logger.debug('printProperties:Focuser: focFoff, steps: {0}, between: {1} and {2}'.format(len(self.rt.foc.focFoff), min(self.rt.foc.focFoff), max(self.rt.foc.focFoff)))
        else:
            self.logger.debug('printProperties:Focuser: focFoff, set by filters if not --blind is specified')
            
        self.logger.debug('printProperties:Focuser: {} focDef'.format(self.rt.foc.focDef))
        self.logger.debug('printProperties:Focuser: {} focMn'.format(self.rt.foc.focMn))
        self.logger.debug('printProperties:Focuser: {} focMx'.format(self.rt.foc.focMx))
        # CCD
        self.logger.debug('')
        self.logger.debug('printProperties:CCD: {} name'.format(self.rt.ccd.name))
        self.logger.debug('printProperties:CCD: {} binning'.format(self.rt.ccd.binning))
        self.logger.debug('printProperties:CCD: {} windowOffsetX'.format(self.rt.ccd.windowOffsetX))
        self.logger.debug('printProperties:CCD: {} windowOffsetY'.format(self.rt.ccd.windowOffsetY))
        self.logger.debug('printProperties:CCD: {} windowHeight'.format(self.rt.ccd.windowHeight))
        self.logger.debug('printProperties:CCD: {} windowWidth'.format(self.rt.ccd.windowWidth))
        self.logger.debug('printProperties:CCD: {} pixelSize'.format(self.rt.ccd.pixelSize))
        self.logger.debug('printProperties:CCD: {} baseExposure'.format(self.rt.ccd.baseExposure))
        #Filter

    @timeout(seconds=10, error_message=os.strerror(errno.ETIMEDOUT))
    def __deviceWriteAccessCCD(self):
        ccdOk=False
        if self.verbose: self.logger.debug('__deviceWriteAccessCCD: asking   from CCD: {0}: calculate_stat'.format(self.rt.ccd.name))
        cs=self.proxy.getDevice(self.rt.ccd.name)['calculate_stat'][1]
        if self.verbose: self.logger.debug('__deviceWriteAccessCCD: response from CCD: {0}: calculate_stat: {1}'.format(self.rt.ccd.name, cs))
        
        try:
            self.proxy.setValue(self.rt.ccd.name,'calculate_stat', 3) # no statisctics
            self .proxy.setValue(self.rt.ccd.name,'calculate_stat', str(cs))
            ccdOk= True
        except Exception, e:
            self.logger.error('__deviceWriteAccessCCD: CCD: {0} is not writable: {1}'.format(self.rt.ccd.name, repr(e)))

        if ccdOk:
            self.logger.debug('__deviceWriteAccessCCD: CCD: {} is writable'.format(self.rt.ccd.name))
            
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
            self.logger.error('__deviceWriteAccessFoc: focuser: {0} is not writable: {1}'.format(self.rt.foc.name, repr(e)))
        if focOk:
            self.logger.debug('__deviceWriteAccessFoc: focuser: {} is writable'.format(self.rt.foc.name))

        return focOk

    @timeout(seconds=30, error_message=os.strerror(errno.ETIMEDOUT))
    def __deviceWriteAccessFtw(self):
        ftwOk=False
        if self.verbose: self.logger.debug('__deviceWriteAccessFtw: asking   from Ftw: {0}: filter'.format(self.rt.filterWheelsInUse[0].name))
        ftnr=  self.proxy.getSingleValue(self.rt.filterWheelsInUse[0].name, 'filter')
        if self.verbose: self.logger.debug('__deviceWriteAccessFtw: response from Ftw: {0}: filter: {1}'.format(self.rt.filterWheelsInUse[0].name, ftnr))
        ftnrp1= str(ftnr +1)
        try:
            if self.verbose: self.logger.debug('__deviceWriteAccessFtw: setting  Ftw: {0}: to filter: {1}'.format(self.rt.filterWheelsInUse[0].name, ftnrp1))
            self.proxy.setValue(self.rt.filterWheelsInUse[0].name, 'filter',  ftnrp1)
            if self.verbose: self.logger.debug('__deviceWriteAccessFtw: setting  Ftw: {0}: to filter: {1}'.format(self.rt.filterWheelsInUse[0].name, ftnr))
            self.proxy.setValue(self.rt.filterWheelsInUse[0].name, 'filter',  str(ftnr))
            ftwOk= True
        except Exception, e:
            if e:
                self.logger.error('__deviceWriteAccessFtw: filter wheel: {0} is not writable: {1}'.format(self.rt.filterWheelsInUse[0].name, repr(e)))
            else:
                self.logger.error('__deviceWriteAccessFtw: filter wheel: {0} is not writable (not timed out)'.format(self.rt.filterWheelsInUse[0].name))
        if ftwOk:
            self.logger.debug('__deviceWriteAccessFtw: filter wheel: {} is writable'.format(self.rt.filterWheelsInUse[0].name))

        return ftwOk

    def deviceWriteAccess(self):
        self.logger.info('deviceWriteAccess: deviceWriteAccess: this may take approx. a minute')

        ccdOk=ftwOk=focOk=False

        ccdOk=self.__deviceWriteAccessCCD()
        focOk=self.__deviceWriteAccessFoc()
        ftwOk=self.__deviceWriteAccessFtw()

        if  ccdOk and ftwOk and focOk:
            self.logger.info('deviceWriteAccess: deviceWriteAccess: all devices are writable')


        return ccdOk and ftwOk and focOk

        
if __name__ == '__main__':

    import argparse
    import rts2saf.devices as dev
    import rts2saf.log as  lg
    import rts2saf.config as cfgd

    prg= re.split('/', sys.argv[0])[-1]
    parser= argparse.ArgumentParser(prog=prg, description='rts2asaf check devices')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--logfile',dest='logfile', default='{0}.log'.format(prg), help=': %(default)s, logfile name')
    parser.add_argument('--topath', dest='toPath', metavar='PATH', action='store', default='.', help=': %(default)s, write log file to path')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--config', dest='config', action='store', default='/etc/rts2/rts2saf/rts2saf.cfg', help=': %(default)s, configuration file path')
    parser.add_argument('--verbose', dest='verbose', action='store_true', default=False, help=': %(default)s, print device properties and add more messages')
    parser.add_argument('--checkwrite', dest='checkWrite', action='store_true', default=False, help=': %(default)s, check if devices are writable')
    parser.add_argument('--focrange', dest='focRange', action='store', default=None,type=int, nargs='+', help=': %(default)s, focuser range given as "ll ul st" used only during blind run')
    parser.add_argument('--exposure', dest='exposure', action='store', default=None, type=float, help=': %(default)s, exposure time for CCD')
    parser.add_argument('--focdef', dest='focDef', action='store', default=None, type=float, help=': %(default)s, set FOC_DEF to value')
    parser.add_argument('--blind', dest='blind', action='store_true', default=False, help=': %(default)s, focus range and step size are defined in configuration, if --focrange is defined it is used to set the range')

    args=parser.parse_args()

    if args.verbose:
        args.level='DEBUG'
        args.debug=True
        args.toconsole=True

    lgd= lg.Logger(debug=args.debug, args=args) # if you need to chage the log format do it here
    logger= lgd.logger 

    if not args.blind and args.focRange:
        logger.error('setCheckDevices: --focrange has no effect without --blind'.format(args.focRange))
        sys.exit(1)

    if args.focRange:
        if (args.focRange[0] >= args.focRange[1]) or args.focRange[2] <= 0: 
            logger.error('setCheckDevices: bad range values: {}, exiting'.format(args.focRange))
            sys.exit(1)

    rt=cfgd.Configuration(logger=logger)
    rt.readConfiguration(fileName=args.config)

    cdv= SetCheckDevices(debug=args.debug, rangeFocToff=args.focRange, blind=args.blind, verbose=args.verbose, rt=rt, logger=logger)

    if not cdv.statusDevices():
        print 'DONE'
        logger.error('setCheckDevices: check not finished, exiting')
        sys.exit(1)

    cdv.statusDevices()

    if args.verbose:
        cdv.printProperties()

    if args.checkWrite:
        if not cdv.deviceWriteAccess():
            logger.error('setCheckDevices: check not finished, exiting')
            sys.exit(1)
    else:
        logger.info('setCheckDevices: skiped check if devices are writable, enable with --checkwrite')        

    logger.info('setCheckDevices: DONE')        

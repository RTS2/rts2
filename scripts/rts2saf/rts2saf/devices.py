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
import string
from rts2saf.timeout import timeout

# ToDo read, write to real devices
class Filter(object):
    """Class for filter properties"""
    def __init__(self, debug=None, name=None, OffsetToEmptySlot=None, lowerLimit=None, upperLimit=None, stepSize=None, exposureFactor=1., focFoff=None):
        self.debug=debug
        self.name= name
        self.OffsetToEmptySlot= OffsetToEmptySlot# [tick]
        self.relativeLowerLimit= lowerLimit# [tick]
        self.relativeUpperLimit= upperLimit# [tick]
        self.exposureFactor   = exposureFactor 
        self.stepSize  = stepSize # [tick]
        self.focFoff=focFoff # range


# ToDo read, write to real devices
class FilterWheel(object):
    """Class for filter wheel properties"""
    def __init__(self, debug=None, name=None, filterOffsets=list(), filters=list()):
        self.debug=debug
        self.name= name
        self.filterOffsets=filterOffsets # from CCD
        self.filters=filters # list of Filter
        self.emptySlots=None # set at run time ToDo go away??

    def check(self, proxy=None):
        proxy.refresh()
        # ToDo goon

# ToDo read, write to real devices
class Focuser(object):
    """Class for focuser properties"""
    def __init__(self, debug=None, name=None, resolution=None, absLowerLimit=None, absUpperLimit=None, lowerLimit=None, upperLimit=None, stepSize=None, speed=None, temperatureCompensation=None, focFoff=None, logger=None):
        self.debug=debug
        self.name= name
        self.resolution=resolution 
        self.absLowerLimit=absLowerLimit
        self.absUpperLimit=absUpperLimit
        self.lowerLimit=lowerLimit
        self.upperLimit=upperLimit
        self.stepSize=stepSize
        self.speed=speed
        self.temperatureCompensation=temperatureCompensation
        self.focFoff=focFoff # will be set at run time
        self.focDef=None # will be set at run time
        self.focMn=None # will be set at run time
        self.focMx=None # will be set at run time
        self.logger=logger

    def check(self, proxy=None):

        proxy.refresh()
        try:
            proxy.getDevice(self.name)
        except:
            self.logger.error('setCheckDevices: focuser device: {0} not present'.format(self.name))        
            return False
        if self.debug: self.logger.debug('setCheckDevices: focuser: {0} present'.format(self.name))

        try:
            self.focDef=int(proxy.getDevice(self.name)['FOC_DEF'][1])
        except Exception, e:
            self.logger.warn('check: focuser: {0} has no FOC_DEF set '.format(self.name))

        focMin=focMax=None
        try:
            focMin= proxy.getDevice(self.name)['foc_min'][1]
            focMax= proxy.getDevice(self.name)['foc_max'][1]
        except Exception, e:
            self.logger.warn('check: focuser: {0} has no foc_min or foc_max properties '.format(self.name))


        return True

# ToDo read, write to real devices
class CCD(object):
    """Class for CCD properties"""
    def __init__(self, debug=None, name=None, binning=None, windowOffsetX=None, windowOffsetY=None, windowHeight=None, windowWidth=None, pixelSize=None, baseExposure=None, logger=None):
        
        self.debug=debug
        self.name= name
        self.binning=binning
        self.windowOffsetX=windowOffsetX
        self.windowOffsetY=windowOffsetY
        self.windowHeight=windowHeight
        self.windowWidth=windowWidth
        self.pixelSize= pixelSize
        self.baseExposure= baseExposure
        self.logger=logger

    def check(self, proxy=None):
        proxy.refresh()
        try:
            proxy.getDevice(self.rt.ccd.name)
        except:
            self.logger.error('setCheckDevices: camera device: {0} not present'.format(self.name))        
            return False
        if self.debug: self.logger.debug('setCheckDevices: camera: {0} present'.format(self.name))
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
            ftwn=proxy.getValue(self.rt.ccd.name, 'wheel')
        except Exception, e:
            self.logger.error('setCheckDevices: no filter wheel present')
            return False

#
class CreateDevice(object):
    """Create the device """    
    def __init__(self, debug=False, proxy=None, blind=None, verbose=None, rt=None, logger=None):
        self.debug=debug
        self.proxy=proxy
        self.blind=blind
        self.verbose=verbose
        self.rt=rt
        self.logger=logger
        self.filters=list()
        self.filterWheelsDefs=dict()


class CreateCCD(CreateDevice):
    """Create the device CCD"""    
    def __init__( self, myStuff=None, *args, **kw ):
        super( CreateCCD, self ).__init__( *args, **kw )

    def create(self):
        # configuration has been read in, now create objects
        # create object CCD
        # ToDo, what does RTS2::ccd driver expect: int or str list?
        # for now: int
        wItems= re.split('[,]+', self.rt.cfg['WINDOW'][1:-1])
        if len(wItems) < 4:
            self.logger.warn( 'createDevices: too few ccd window items {0} {1}, using the whole CCD area'.format(len(items), value))
            wItems= [ -1, -1, -1, -1]
        else:
            wItems[:]= map(lambda x: int(x), wItems)

            ccd= CCD( 
                debug        =self.debug,
                name         =self.rt.cfg['CCD_NAME'],
                binning      =self.rt.cfg['CCD_BINNING'],
                windowOffsetX=wItems[0],
                windowOffsetY=wItems[1],
                windowHeight =wItems[2],
                windowWidth  =wItems[3],
                pixelSize    =float(self.rt.cfg['PIXELSIZE']),
                baseExposure =float(self.rt.cfg['BASE_EXPOSURE']),
                logger=self.logger
                )
            
        return ccd

class CreateFocuser(CreateDevice):
    """Create the device Focuser"""    
    def __init__( self, rangeFocToff=None, *args, **kw ):
        super( CreateFocuser, self ).__init__( *args, **kw )
        self.rangeFocToff=rangeFocToff

    def create(self):
        rangeMin=rangeMax=rangeStep=None
        # focRange (from args) has priority
        if self.rangeFocToff:
            rangeMin= self.rangeFocToff[0] + self.focDef
            rangeMax= self.rangeFocToff[1] + self.focDef
            rangeStep= abs(self.rangeFocToff[2])
            self.logger.info('create: focuser: {0} setting internal limits from arguments'.format(self.rt.cfg['FOCUSER_NAME']))

        else:
            rangeMin=int(self.rt.cfg['FOCUSER_LOWER_LIMIT'])
            rangeMax=int(self.rt.cfg['FOCUSER_UPPER_LIMIT'])
            rangeStep=int(self.rt.cfg['FOCUSER_STEP_SIZE'])
            self.logger.info('create: focuser: {0} setting internal limits from configuration file and ev. default values!'.format(self.rt.cfg['FOCUSER_NAME']))

        self.logger.info('create: focuser: {0} setting internal limits to:[{1}, {2}], step size: {3}'.format(self.rt.cfg['FOCUSER_NAME'], rangeMin, rangeMax, rangeStep))

        withinLlUl=False
        # ToDo check that against HW limits
        if int(self.rt.cfg['FOCUSER_ABSOLUTE_LOWER_LIMIT']) <= rangeMin <= int(self.rt.cfg['FOCUSER_ABSOLUTE_UPPER_LIMIT'])-(rangeMax-rangeMin):
            if int(self.rt.cfg['FOCUSER_ABSOLUTE_LOWER_LIMIT']) + (rangeMax-rangeMin) <= rangeMax <=  int(self.rt.cfg['FOCUSER_ABSOLUTE_UPPER_LIMIT']):
                withinLlUl=True

        if withinLlUl:
            self.focFoff= range(rangeMin, rangeMax +rangeStep, rangeStep)
            # ToDo must goelswhere
            #if len(self.focFoff) <= self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
            #    self.logger.warn('create: to few focuser positions: {0}<={1} (see MINIMUM_FOCUSER_POSITIONS)'.format(len(self.focFoff), self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']))

        else:
            self.logger.error('create: focuser: {0:8s} abs. lowerLimit: {1}, abs. upperLimit: {2}'.format(self.rt.cfg['FOCUSER_NAME'], self.rt.cfg['FOCUSER_ABSOLUTE_LOWER_LIMIT'], self.rt.cfg['FOCUSER_ABSOLUTE_UPPER_LIMIT'])) 
            self.logger.error('create: focuser:                 rangeMin: {}         rangeMax: {}, '.format( rangeMin, rangeMax))
            self.logger.error('create: out of bounds, returning')
            return None

        # ToDO check with no filter wheel configuration
        if len(self.focFoff) > 10 and  self.blind:
            self.logger.info('create: focuser range has: {0} steps, you might consider set decent value for --focrange'.format(len(self.focFoff)))


        # create object focuser
        foc= Focuser(
            debug         =self.debug,
            name          =self.rt.cfg['FOCUSER_NAME'],
            resolution    =float(self.rt.cfg['FOCUSER_RESOLUTION']),
            absLowerLimit =int(self.rt.cfg['FOCUSER_ABSOLUTE_LOWER_LIMIT']),
            absUpperLimit =int(self.rt.cfg['FOCUSER_ABSOLUTE_UPPER_LIMIT']),
            lowerLimit    =rangeMin,
            upperLimit    =rangeMax,
            stepSize      =rangeStep,
            speed         =float(self.rt.cfg['FOCUSER_SPEED']),
            temperatureCompensation=bool(self.rt.cfg['FOCUSER_TEMPERATURE_COMPENSATION']),
            logger = self.logger
            # set at run time:
            # focFoff=None,
            # focDef=None,
            # focMn=None,
            # focMx=None,
            # focSt=None
            )
        return foc

class CreateFilters(CreateDevice):
    """Create the filters"""    
    def __init__( self, myStuff=None, *args, **kw ):
        super( CreateFilters, self ).__init__( *args, **kw )

    
    def create(self):
        if self.rt.cfg['FAKE']:
            # create one FAKE_FT filter
            lowerLimit    = int(self.rt.cfg['FOCUSER_NO_FTW_RANGE'][0])
            upperLimit    = int(self.rt.cfg['FOCUSER_NO_FTW_RANGE'][1])
            stepSize      = int(self.rt.cfg['FOCUSER_NO_FTW_RANGE'][2])
            focFoff=range(lowerLimit, (upperLimit + stepSize), stepSize)

            ft=Filter( 
                debug        =self.debug,
                name          = 'FAKE_FT',
                OffsetToEmptySlot= 0,
                lowerLimit    =lowerLimit,
                upperLimit    =upperLimit,
                stepSize      =stepSize,
                exposureFactor= 1.,
                focFoff=focFoff
                )
                
            self.filters.append(ft)
        else:
            # create objects filter
            for ftd in self.rt.cfg['FILTER DEFINITIONS']:
                ftItems= ftd[1:-1].split(',')
                lowerLimit    = int(ftItems[1])
                upperLimit    = int(ftItems[2])
                stepSize      = int(ftItems[3])
                focFoff=range(lowerLimit, (upperLimit + stepSize), stepSize)
                name   = ftItems[0]
                if name in self.rt.cfg['EMPTY_SLOT_NAMES'] :
                    OffsetToEmptySlot= 0
                else:
                    OffsetToEmptySlot= None # must be set through CCD driver

                ft=Filter( 
                    name              = name,
                    OffsetToEmptySlot = OffsetToEmptySlot,
                    lowerLimit    = lowerLimit,
                    upperLimit    = upperLimit,
                    stepSize      = stepSize,
                    exposureFactor= string.atof(ftItems[4]),
                    focFoff       = focFoff
                    )
                self.filters.append(ft)

        return self.filters

class CreateFilterWheels(CreateDevice):
    """Create the devices filter wheel"""    
    def __init__( self, filters=None, focuser=None, *args, **kw ):
        super( CreateFilterWheels, self ).__init__( *args, **kw )

        self.filters=filters
        self.focuser=focuser

    def create(self):

        filterWheels=list()
        filterWheelsInUse=list()
        # create objects FilterWheel whith filter wheels names and with filter objects
        #  ftwn: W2
        #  ftds: ['nof1', 'U', 'Y', 'O2']
        for ftwn,ftds in self.rt.cfg['FILTER WHEEL DEFINITIONS'].iteritems():
            print ftds
            # ToDo (Python) if filters=list() is not present, then all filters appear in all filter wheels
            ftw=FilterWheel(debug=self.debug, name=ftwn,filters=list())
            for ftd in ftds:
                for ft in self.filters:
                    if ftd in ft.name: 
                        ftw.filters.append(ft)
                        break
                else:
                    self.logger.error('createDevices: no filter named: {0} found in config: {1}'.format(ftd, self.rt.cfg['CFGFN']))
                    return None

            filterWheels.append(ftw)

        # only used filter wheel are returned
        # ToDo fetch the offsets from CCD driver
        # and override the values from configuration
        # Think only FILTA will have that
        for ftw in filterWheels:
            if ftw.name in self.rt.cfg['FILTER WHEELS INUSE']:
                # ToDo ftw.ft.OffsetToEmptySlot==0
                filterWheelsInUse.append(ftw)

        # There is no real filter wheel
        for ftwIU in filterWheelsInUse: 
            if ftwIU.name in 'FAKE_FTW':
                if self.debug: self.logger.debug('create: using FAKE_FTW')        
                return filterWheelsInUse

        self.proxy.refresh()
        # find empty slots on all filter wheels
        # assumption: no combination of filters of the different filter wheels
        eSs=0
        for ftw in filterWheelsInUse:
            try:
                self.proxy.getDevice(ftw.name)
            except:
                self.logger.error('create: filter wheel device {0} not present'.format(ftw.name))        
                return None

            # first find in ftw.filters and empty slot 
            # use this slot first, followed by all others
            ftw.filters.sort(key=lambda x: x.OffsetToEmptySlot)
            for ft in ftw.filters:
                if self.debug: self.logger.debug('create: filter wheel: {0:5s}, filter:{1:5s} in use'.format(ftw.name, ft.name))
                if ft.OffsetToEmptySlot==0:
                    # ft.emptySlot=Null at instanciation
                    try:
                        ftw.emptySlots.append(ft)
                    except:
                        ftw.emptySlots=list()
                        ftw.emptySlots.append(ft)

                    eSs += 1
                                            
                    if self.debug: self.logger.debug('create: filter wheel: {0:5s}, filter:{1:5s} is a candidate empty slot'.format(ftw.name, ft.name))

            if ftw.emptySlots:
                # drop multiple empty slots
                self.logger.info('create: filter wheel: {0}, empty slot:{1}'.format(ftw.name, ftw.emptySlots[0].name))

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
                        self.logger.info('create: filter wheel: {0}, dropping empty slot:{1}'.format(ftw.name, ft.name))
                        ftw.filters.remove(ft)
                    else:
                        if self.debug: self.logger.debug('create: filter wheel: {0}, NOT dropping slot:{1} with no offset=0 to empty slot'.format(ftw.name, ft.name))
                            
            else:
                # warn only if two or more ftws are used
                if len(filterWheelsInUse) > 0:
                    self.logger.warn('create: filter wheel: {0}, no empty slot found'.format(ftw.name))

        if eSs >= len(filterWheelsInUse):
            if self.debug: self.logger.debug('create: all filter wheels have an empty slot')
        else:
            self.logger.warn('create: not all filter wheels have an empty slot, {}, {}'.format(eSs,  len(filterWheelsInUse)))
            return None


        # check bounds of filter lower and upper limit settings            
        anyOutOfLlUl=False
        name = self.focuser.name
        focDef = self.focuser.focDef
        absLowerLimit = self.focuser.absLowerLimit
        absUpperLimit = self.focuser.absUpperLimit

        for k, ftw in enumerate(filterWheelsInUse):
            if len(ftw.filters)>1 or k==0:
                for ft in ftw.filters:
                    if self.blind:
                        # focuser limits are done in method create()
                        pass
                    else:
                        rangeMin=focDef + min(ft.focFoff)
                        rangeMax=focDef + max(ft.focFoff)
                        outOfLlUl=True 
                        if absLowerLimit <= rangeMin <= absUpperLimit-(rangeMax-rangeMin):
                            if absLowerLimit + (rangeMax-rangeMin) <= rangeMax <=  absUpperLimit:
                                outOfLlUl=False

                        if outOfLlUl:
                            self.logger.error( 'checkDevices: {0:8s}, filter: {1:8s}, focuser: {2}, settings: abs. lowerLimit: {3}, abs. upperLimit: {4}; range rangeMin: {5} rangeMax: {6}, configuartion: **step size**: {7}, rel.min: {8}, rel.max: {9}'.format(ftw.name, ft.name, name, absLowerLimit, absUpperLimit, rangeMin, rangeMax, ft.stepSize, ft.relativeLowerLimit, ft.relativeUpperLimit))  
                            anyOutOfLlUl=True 

        if anyOutOfLlUl:
            self.logger.error('create: out of bounds, returning')
            self.logger.info('create: focuser: {0} FOC_DEF: {1}'.format(name, focDef))
            self.logger.info('create: check setting of FOC_DEF, FOC_FOFF, FOC_TOFF and limits in configuration')
            return False

        anyBelow=False
        # check MINIMUM_FOCUSER_POSITIONS
        for ftw in filterWheelsInUse:
            for ft in ftw.filters:
                if len(ft.focFoff) <= self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
                    self.logger.error( 'checkDevices: {0:8s}, filter: {1:8s} to few focuser positions: {2}<={3} (see MINIMUM_FOCUSER_POSITIONS)'.format(ftw.name, ft.name, len(ft.focFoff), self.rt.cfg['MINIMUM_FOCUSER_POSITIONS'])) 
                    anyBelow=True

        if anyBelow:
            self.logger.error('create: too few focuser positions, returning')
            return None




        return filterWheelsInUse

# ToDo remove read, write to real devices
class CheckDevices(object):
    """Check the presence of the devices and filter slots"""    
    def __init__(self, debug=False, proxy=None, blind=None, verbose=None, filterWheels=None, focuser=None, rt=None, logger=None):
        self.debug=debug
        self.blind=blind
        self.verbose=verbose
        self.filterWheels=filterWheels
        self.focuser=focuser
        self.rt=rt
        self.logger=logger
        self.proxy= proxy 

    def summaryDevices(self):
        # log INFO a summary, after dropping multiple empty slots
        img=0
        # ToDo first part will go away
        if len(self.filterWheels)==0:
            self.logger.info('summaryDevices: focus run summary, no filter wheel in use:')
            self.logger.info('summaryDevices: focuser: {0}, steps: {1}, min: {2}, max: {3}'.format(focuser.name, len(focuser.focFoff), min(focuser.focFoff), max(focuser.focFoff)))
            img=len(self.rt.foc.focFoff)

        else:
            self.logger.info('summaryDevices: focus run summary, without multiple empty slots:')
            for k, ftw in enumerate(self.filterWheels):
                # count only wheels with more than one filters (one slot must be empty)
                # the first filter wheel in the list 
                if len(ftw.filters)>1 or k==0:
                    info = str()
                    for ft in ftw.filters:
                        info += 'checkDevices: {0:8s}: {1:8s}'.format(ftw.name, ft.name)
                        if self.blind:
                            info += '{0:2d} steps, between: {1:5d} and {2:5d}\n'.format(len(focuser.focFoff), min(focuser.focFoff), max(focuser.focFoff))
                            img += len(self.rt.foc.focFoff)
                        else:
                            info += '{0:2d} steps, FOC_FOFF between: {1:5d} and {2:5d}, '.format(len(ft.focFoff), min(ft.focFoff), max(ft.focFoff))
                            info += 'FOC_POS between: {1:5d} and {2:5d}, FOC_DEF: {3:5d}\n'.format(len(ft.focFoff), self.focuser.focDef + min(ft.focFoff), self.focuser.focDef + max(ft.focFoff), self.focuser.focDef)
                            img += len(ft.focFoff)
                    else:
                        self.logger.info('\n{0}'.format(info))

        self.logger.info('summaryDevices: taking {0} images in total'.format(img))
        

    def printProperties(self):
        # Focuser
        self.logger.debug('printProperties:Focuser: {} name'.format(self.focuser.name))
        self.logger.debug('printProperties:Focuser: {} resolution'.format(self.focuser.resolution))
        self.logger.debug('printProperties:Focuser: {} absLowerLimit'.format(self.focuser.absLowerLimit))
        self.logger.debug('printProperties:Focuser: {} absUpperLimit'.format(self.focuser.absUpperLimit))
        self.logger.debug('printProperties:Focuser: {} speed'.format(self.focuser.speed))
        self.logger.debug('printProperties:Focuser: {} stepSize'.format(self.focuser.stepSize))
        self.logger.debug('printProperties:Focuser: {} temperatureCompensation'.format(self.focuser.temperatureCompensation))
        if self.blind:
            self.logger.debug('printProperties:Focuser: focFoff, steps: {0}, between: {1} and {2}'.format(len(self.focuser.focFoff), min(self.focuser.focFoff), max(self.focuser.focFoff)))
        else:
            self.logger.debug('printProperties:Focuser: focFoff, set by filters if not --blind is specified')
            
        self.logger.debug('printProperties:Focuser: {} focDef'.format(self.focuser.focDef))
        self.logger.debug('printProperties:Focuser: {} focMn'.format(self.focuser.focMn))
        self.logger.debug('printProperties:Focuser: {} focMx'.format(self.focuser.focMx))
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

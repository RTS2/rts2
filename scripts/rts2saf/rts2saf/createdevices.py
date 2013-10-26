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
import re
import os
import string

from rts2saf.devices import CCD,Focuser,Filter,FilterWheel
#
class CreateDevice(object):
    """Create the device """    
    def __init__(self, debug=False, proxy=None, check=None, blind=None, verbose=None, rt=None, logger=None):
        self.debug=debug
        self.proxy=proxy
        self.check=check
        self.blind=blind
        self.verbose=verbose
        self.rt=rt
        self.logger=logger
        self.filters=list()
        self.filterWheelsDefs=dict()

class CreateCCD(CreateDevice):
    """Create the device CCD"""    
    def __init__( self, ftws=None, *args, **kw ):
        super( CreateCCD, self ).__init__( *args, **kw )
        self.ftws=ftws

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
                ftws         =self.ftws,
                binning      =self.rt.cfg['CCD_BINNING'],
                windowOffsetX=wItems[0],
                windowOffsetY=wItems[1],
                windowHeight =wItems[2],
                windowWidth  =wItems[3],
                pixelSize    =float(self.rt.cfg['PIXELSIZE']),
                baseExposure =float(self.rt.cfg['BASE_EXPOSURE']),
                logger=self.logger
                )
            
        if self.check:
            if not ccd.check(proxy=self.proxy):
                return None
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
            self.logger.info('create:  {0} setting internal limits from arguments'.format(self.rt.cfg['FOCUSER_NAME']))

        else:
            rangeMin=int(self.rt.cfg['FOCUSER_LOWER_LIMIT'])
            rangeMax=int(self.rt.cfg['FOCUSER_UPPER_LIMIT'])
            rangeStep=int(self.rt.cfg['FOCUSER_STEP_SIZE'])
            self.logger.info('create:  {0} setting internal limits from configuration file and ev. default values!'.format(self.rt.cfg['FOCUSER_NAME']))

        self.logger.info('create:  {0} setting internal limits for --blind :[{1}, {2}], step size: {3}'.format(self.rt.cfg['FOCUSER_NAME'], rangeMin, rangeMax, rangeStep))

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
            self.logger.error('create:  {0:8s} abs. lowerLimit: {1}, abs. upperLimit: {2}'.format(self.rt.cfg['FOCUSER_NAME'], self.rt.cfg['FOCUSER_ABSOLUTE_LOWER_LIMIT'], self.rt.cfg['FOCUSER_ABSOLUTE_UPPER_LIMIT'])) 
            self.logger.error('create:                  rangeMin: {}         rangeMax: {}, '.format( rangeMin, rangeMax))
            self.logger.error('create: out of bounds, returning')
            return None

        # ToDO check with no filter wheel configuration
        if len(self.focFoff) > 10 and  self.blind:
            self.logger.info('create: focuser range has: {0} steps, you might consider set decent value for --focrange'.format(len(self.focFoff)))

        try:
            focDef=int(self.proxy.getDevice(self.rt.cfg['FOCUSER_NAME'])['FOC_DEF'][1])
        except Exception, e:
            self.logger.warn('create:  {0} has no FOC_DEF set '.format(self.rt.cfg['FOCUSER_NAME']))
            focDef=None

        if self.debug: self.logger.debug('create:  {0} FOC_DEF: {1}'.format(self.rt.cfg['FOCUSER_NAME'], focDef))

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
            logger = self.logger,
            focDef=focDef,
            # set at run time:
            # focFoff=None,
            # focMn=None,
            # focMx=None,
            # focSt=None
            )
        if self.check:
            if not foc.check(proxy=self.proxy):
                return None

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
    def __init__( self, filters=None, foc=None, *args, **kw ):
        super( CreateFilterWheels, self ).__init__( *args, **kw )

        self.filters=filters
        self.foc=foc

    def create(self):

        # all in config defined filters have no relation to a filter wheel
        filterDict= { x.name: x for x in self.filters }
        filterWheels=list()
        # create objects FilterWheel whith filter wheels names and with filter objects
        #  ftwn: W2
        #  ftds: ['nof1', 'U', 'Y', 'O2']
        for ftwn,ftds in self.rt.cfg['FILTER WHEEL DEFINITIONS'].iteritems():
            # ToDo (Python) if filters=list() is not present, then all filters appear in all filter wheels
            filters=list()
            for ftd in ftds:
                ft=None
                try:
                    ft=filterDict[ftd]
                except:
                    self.logger.error('createDevices: no filter named: {0} found in config: {1}'.format(ftd, self.rt.cfg['CFGFN']))
                    return None

                if ft:
                    filters.append(filterDict[ftd])
            # empty slots first
            filters.sort(key=lambda x: x.OffsetToEmptySlot, reverse=True)
            ftw=FilterWheel(debug=self.debug, name=ftwn,filters=filters)
            filterWheels.append(ftw)

        # for ftw in filterWheels:
        #    for ft in ftw.filters:
        #        print ftw.name, ft.name

        # sys.exit(1)
        # only used filter wheel are returned
        # ToDo fetch the offsets from CCD driver
        # and override the values from configuration
        # Think only FILTA will have that
        filterWheelsInUse=list()
        for ftw in filterWheels:
            if ftw.name in self.rt.cfg['FILTER WHEELS INUSE']:
                # ToDo ftw.ft.OffsetToEmptySlot==0
                filterWheelsInUse.append(ftw)
                
        # empty slots first
        filterWheelsInUse.sort(key = lambda x: len(x.filters), reverse=True)

        # find empty slots on all filter wheels
        # assumption: no combination of filters of the different filter wheels
        eSs=0
        for ftw in filterWheelsInUse:
            # first find in ftw.filters an empty slot 
            # use this slot first, followed by all others
            for ft in ftw.filters:
                if self.debug: self.logger.debug('create:  {0:5s}, filter:{1:5s} in use'.format(ftw.name, ft.name))
                if ft.OffsetToEmptySlot==0:
                    # ft.emptySlot=Null at instanciation
                    try:
                        ftw.emptySlots.append(ft)
                    except:
                        ftw.emptySlots=list()
                        ftw.emptySlots.append(ft)

                    eSs += 1
                    if self.debug: self.logger.debug('create:  {0:5s}, filter:{1:5s} is a candidate empty slot'.format(ftw.name, ft.name))

            if ftw.emptySlots:
                # drop multiple empty slots
                self.logger.info('create:  {0}, empty slot:{1}'.format(ftw.name, ftw.emptySlots[0].name))

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
                        self.logger.info('create:  {0}, dropping empty slot:{1}'.format(ftw.name, ft.name))
                        ftw.filters.remove(ft)
                    else:
                        if self.debug: self.logger.debug('create:  {0}, NOT dropping slot:{1} with no offset=0 to empty slot'.format(ftw.name, ft.name))
                            
            else:
                # warn only if two or more ftws are used
                if len(filterWheelsInUse) > 0:
                    self.logger.warn('create:  {0}, no empty slot found'.format(ftw.name))

        if eSs >= len(filterWheelsInUse):
            if self.debug: self.logger.debug('create: all filter wheels have an empty slot')
        else:
            self.logger.warn('create: not all filter wheels have an empty slot, {}, {}'.format(eSs,  len(filterWheelsInUse)))
            return None

        # check bounds of filter lower and upper limit settings            
        anyOutOfLlUl=False
        name = self.foc.name
        focDef = self.foc.focDef
        absLowerLimit = self.foc.absLowerLimit
        absUpperLimit = self.foc.absUpperLimit

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
                            self.logger.error( 'create:  {0:8s}, filter: {1:8s},  {2}: abs.limit: [{3},  {4}]; range: [{5}  {6}], step size: {7}, offset: [{8} {9}]'.format(ftw.name, ft.name, name, absLowerLimit, absUpperLimit, rangeMin, rangeMax, ft.stepSize, ft.relativeLowerLimit, ft.relativeUpperLimit))  
                            anyOutOfLlUl=True 

        if anyOutOfLlUl:
            self.logger.error('create: out of bounds, returning')
            self.logger.info('create:  {0} FOC_DEF: {1}'.format(name, focDef))
            self.logger.info('create: check setting of FOC_DEF, FOC_FOFF, FOC_TOFF and limits in configuration')
            return None

        anyBelow=False
        # check MINIMUM_FOCUSER_POSITIONS
        for ftw in filterWheelsInUse:
            for ft in ftw.filters:
                if len(ft.focFoff) <= self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
                    self.logger.error( 'create: {0:8s}, filter: {1:8s} to few focuser positions: {2}<={3} (see MINIMUM_FOCUSER_POSITIONS)'.format(ftw.name, ft.name, len(ft.focFoff), self.rt.cfg['MINIMUM_FOCUSER_POSITIONS'])) 
                    anyBelow=True

        if anyBelow:
            self.logger.error('create: too few focuser positions, returning')
            return None

        
        if self.check:
            for ftw in filterWheelsInUse:
                if not ftw.check(proxy=self.proxy):
                    return None
        return filterWheelsInUse

        

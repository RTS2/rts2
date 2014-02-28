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
"""Create CCD, filter wheel and focuser objects
"""

__author__ = 'wildi.markus@bluewin.ch'

import sys
import re
import os
import string
import collections
import time

from rts2saf.devices import CCD,Focuser,Filter,FilterWheel
#
class CreateDevice(object):
    """Base class for devices

    :var debug: enable more debug output with --debug and --level
    :var check: if True check presence of device
    :var proxy: :py:mod:`rts2.proxy`
    :var verbose: True, more debug output
    :var rt: run time configuration,  :py:mod:`rts2saf.config.Configuration`, usually read from /usr/local/etc/rts2/rts2saf/rts2saf.cfg
    :var logger:  :py:mod:`rts2saf.log`

    """    
    def __init__(self, debug=False, proxy=None, check=None, verbose=None, rt=None, logger=None):
        self.debug=debug
        self.proxy=proxy
        self.check=check
        self.verbose=verbose
        self.rt=rt
        self.logger=logger
        self.filters=list()
        self.filterWheelsDefs=dict()

class CreateCCD(CreateDevice):
    """Create device :py:mod:`rts2saf.devices.CCD`

    :var debug: enable more debug output with --debug and --level
    :var check: if True check presence of device
    :var proxy: :py:mod:`rts2.proxy`
    :var verbose: True, more debug output
    :var rt: run time configuration,  :py:mod:`rts2saf.config.Configuration`, usually read from /usr/local/etc/rts2/rts2saf/rts2saf.cfg
    :var logger:  :py:mod:`rts2saf.log`
    :var ftws: list of  :py:mod:`rts2saf.devices.FilterWheel`
    :var fetchOffsets: if True call `filterOffsets`
    """    
    def __init__( self, ftws=None, fetchOffsets=False, *args, **kw ):
        super( CreateCCD, self ).__init__( *args, **kw )
        self.ftws=ftws
        self.fetchOffsets=fetchOffsets
        self.ccd=None

    def create(self):
        """Create :py:mod:`rts2saf.devices.CCD` based on properties stored in configuration. Optionally fetch filter offsets from CCD and/or  check if device is present. 

        :return:  :py:mod:`rts2saf.devices.CCD` if success else None

        """
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

            self.ccd= CCD( 
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
            if not self.ccd.check(proxy=self.proxy):
                return None
        if self.fetchOffsets:
            # fetch the filter offsets
            if not self.filterOffsets():
                return None

        return self.ccd

    # ToDo
    # interestingly on can set filter offsets for
    # filter_offsets_B
    # filter_offsets_C
    # but there is no way to specify it via cmd line
    # or --offsets-file
    #
    # filter_offsets_A, etc. see Bootes-2 device andor3
    # ftos=self.proxy.getValue(self.ccd.name, 'filter_offsets_B')
    # print ftos
    # self.proxy.setValue(self.ccd.name,'filter_offsets_B', '6 7 8')
    # self.proxy.refresh()
    # ftos=self.proxy.getValue(self.ccd.name, 'filter_offsets_B')
    # print ftos
    # ftos=self.proxy.getValue(self.ccd.name, 'filter_offsets_C')
    # print ftos
    # self.proxy.setValue(self.ccd.name,'filter_offsets_C', '16 17 18')
    # self.proxy.refresh()
    # ftos=self.proxy.getValue(self.ccd.name, 'filter_offsets_C')
    # print ftos
    # sys.exit(1)
        

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


    def filterOffsets(self, proxy=None):
        """Fetch for all filter wheels their filter offsets and check if all configured items are present in RTS2 run time environment.

        :return: True if success else False

        """
        fts=dict()  # filters
        ftos=dict() # filter offsets
        ccdFtwn=collections.defaultdict(str)
        # as of 2013-11017 CCD::filter_offsets_X are not filled
        for i, ftw in enumerate(self.ftws):
            if i:
                # FILTB, FILTC, ...
                ext= chr( 65 + i) # 'A' + i, as it is done in camd.cpp
                ccdN ='wheel{0}'.format(ext)
                ftwn=self.proxy.getValue(self.ccd.name, ccdN)
                ccdFtwn[ftwn] = ccdN 
                fts[ftwn] =self.proxy.getSelection(self.ccd.name, 'FILT{0}'.format(ext))
                ftos[ftwn]=self.proxy.getValue(self.ccd.name, 'filter_offsets_{0}'.format(ext))
            else:
                ccdN ='wheel'
                ftwn=self.proxy.getValue(self.ccd.name, ccdN)
                ccdFtwn[ftwn] =  ccdN
                fts[ftwn] =self.proxy.getSelection(self.ccd.name, 'filter')
                ftos[ftwn]=self.proxy.getValue(self.ccd.name, 'filter_offsets')

        for ftw in self.ftws: # defined in configuration
            ftIUns= [x.name for x in  ftw.filters] # filters from configuration
            try:
                ftw.ccdName=ccdFtwn[ftw.name] # these filter wheels names  are read back from CCD
                if self.debug: self.logger.debug('filterOffsets: found filter wheel: {0}, named on CCD: {1}'.format(ftw.name, ftw.ccdName))
            except:
                self.logger.error('filterOffsets: {0} {1} configured filter wheel not found'.format(self.ccd.name,ftw.name))
                return False

            if len(ftos[ftw.name])==0:
                self.logger.warn('filterOffsets: {0}, {1} no filter offsets could be read from CCD, but filter wheel/filters are present'.format(self.ccd.name, ftw.name))

            if len(fts[ftw.name])==0:
                self.logger.warn('filterOffsets: {0}, {1} no filters could be read from CCD, but filter wheel/filters are present'.format(self.ccd.name, ftw.name))
            else:
                self.logger.info('filterOffsets: {0}, filter wheel {1} defined filters {2}'.format(self.ccd.name, ftw.name, [x for x in fts[ftw.name]]))
                self.logger.info('filterOffsets: {0}, filter wheel {1} used    filters {2}'.format(self.ccd.name, ftw.name, ftIUns))

            ccdOffsets = [ x for x in ftos[ftw.name]] # offsets from CCD
            ccdFilters = [ x for x in fts[ftw.name]]  # filter names from CCD

            for k, ft in enumerate(ftw.filters): # defined in configuration 
                if ft.name in ccdFilters:            
                    for nm in self.rt.cfg['EMPTY_SLOT_NAMES']:
                        # e.g. filter name R must exactly match!
                        p = re.compile(nm)
                        m = p.match(ft.name)
                        if m:
                            ft.OffsetToEmptySlot= 0
                            if self.debug: self.logger.debug('filterOffsets: {0}, {1} offset set to ZERO'.format(ftw.name, ft.name))
                            break
                    else:
                        try:
                            ft.OffsetToEmptySlot= ccdOffsets[k]
                            if self.debug: self.logger.debug('filterOffsets: {0}, {1} offset {2} from ccd: {3}'.format(ftw.name, ft.name, ft.OffsetToEmptySlot,self.ccd.name))
                        except:
                            ft.OffsetToEmptySlot= 0
                            self.logger.warn('filterOffsets: {0}, {1} NO offset from ccd: {2}, setting it to ZERO'.format(ftw.name, ft.name,self.ccd.name))
                else:
                    if self.debug: self.logger.debug('filterOffsets: {0} filter {1} not found on CCD {2}, ignoring it'.format( ftw.name, ftn, self.ccd.name))        
                    return False
                    
        return True

class CreateFocuser(CreateDevice):
    """Create  device :py:mod:`rts2saf.devices.Focuser`

    :var debug: enable more debug output with --debug and --level
    :var check: if True check presence of device
    :var proxy: :py:mod:`rts2.proxy`
    :var blind: blind - flavor of focus run
    :var verbose: True, more debug output
    :var rt: run time configuration,  :py:mod:`rts2saf.config.Configuration`, usually read from /usr/local/etc/rts2/rts2saf/rts2saf.cfg
    :var logger:  :py:mod:`rts2saf.log`
    :var rangeFocToff: list containing minimum, maximum and step size 

    """    
    def __init__( self, rangeFocToff=None, blind=False, *args, **kw ):
        super( CreateFocuser, self ).__init__( *args, **kw )
        self.rangeFocToff=rangeFocToff
        self.blind=blind

    def create(self, focDef=None): # in production mode focDef must be None, unittest sets it, because there are no real devices there to query
        """Create :py:mod:`rts2saf.devices.Focuser` based on properties stored in configuration. 
        Optionally check if device is present. Focuser range is set if values  
        are within limits [ FOCUSER_ABSOLUTE_LOWER_LIMIT,FOCUSER_ABSOLUTE_UPPER_LIMIT ]

        :return:  :py:mod:`rts2saf.devices.Focuser` if success else None

        """
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
            if self.blind:
                self.logger.info('create:  {0} setting internal limits for --blind :[{1}, {2}], step size: {3}'.format(self.rt.cfg['FOCUSER_NAME'], rangeMin, rangeMax, rangeStep))
            else:
                self.logger.info('create:  {0} setting internal limits from configuration file and ev. default values!'.format(self.rt.cfg['FOCUSER_NAME']))


        withinLlUl=False

        if int(self.rt.cfg['FOCUSER_ABSOLUTE_LOWER_LIMIT']) <= rangeMin <= int(self.rt.cfg['FOCUSER_ABSOLUTE_UPPER_LIMIT'])-(rangeMax-rangeMin):
            if int(self.rt.cfg['FOCUSER_ABSOLUTE_LOWER_LIMIT']) + (rangeMax-rangeMin) <= rangeMax <=  int(self.rt.cfg['FOCUSER_ABSOLUTE_UPPER_LIMIT']):
                withinLlUl=True

        if withinLlUl:
            self.focFoff= range(rangeMin, rangeMax +rangeStep, rangeStep)
            
        else:
            self.logger.error('create:  {0:8s} abs. lowerLimit: {1}, abs. upperLimit: {2}'.format(self.rt.cfg['FOCUSER_NAME'], self.rt.cfg['FOCUSER_ABSOLUTE_LOWER_LIMIT'], self.rt.cfg['FOCUSER_ABSOLUTE_UPPER_LIMIT'])) 
            self.logger.error('create:                  rangeMin: {}         rangeMax: {}, '.format( rangeMin, rangeMax))
            self.logger.error('create: out of bounds, returning')
            return None

        # ToDo check with no filter wheel configuration
        if len(self.focFoff) > 10 and  self.blind:
            self.logger.info('create: focuser range has: {0} steps, you might consider set decent value for --focrange'.format(len(self.focFoff)))

        if focDef is None: # this part used during production mode
            cnt=0
            while True:
                # ToDo the break hard part goes away
                cnt +=1
                if cnt > 10:
                    self.logger.error('create: breaking hard')
                    break

                try:
                    self.proxy.refresh()
                except Exception as e:
                    self.logger.error('create:  can not refresh for device: {0} : {1}'.format(self.rt.cfg['FOCUSER_NAME'], e))
                    time.sleep(1)
                    continue

                try:
                    focDef=int(self.proxy.getDevice(self.rt.cfg['FOCUSER_NAME'])['FOC_DEF'][1])
                    self.logger.info('create:  {0} has    FOC_DEF set, breaking'.format(self.rt.cfg['FOCUSER_NAME']))
                    break
                except Exception, e:
                    self.logger.error('create:  {0} has no FOC_DEF set, error: {1}'.format(self.rt.cfg['FOCUSER_NAME'], e))
                    #focDef=None
                    time.sleep(1)

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
            # will be overridden in FilterWheels.create:
            focFoff=self.focFoff,
            # set at run time:
            # focMn=None,
            # focMx=None,
            # focSt=None
            )
        if self.check:
            if not foc.check(proxy=self.proxy):
                return None

        return foc

class CreateFilters(CreateDevice):
    """Create list of :py:mod:`rts2saf.devices.Filter`

    :var debug: enable more debug output with --debug and --level
    :var check: if True check presence of device
    :var proxy: :py:mod:`rts2.proxy`
    :var verbose: True, more debug output
    :var rt: run time configuration,  :py:mod:`rts2saf.config.Configuration`, usually read from /usr/local/etc/rts2/rts2saf/rts2saf.cfg
    :var logger:  :py:mod:`rts2saf.log`

    """    
    def __init__( self, *args, **kw ):
        super( CreateFilters, self ).__init__( *args, **kw )

    
    def create(self):
        """Create a list :py:mod:`rts2saf.devices.Filters` based on properties stored in configuration. 
        Optionally check if device is present. 

        :return: list of :py:mod:`rts2saf.devices.Filter` if success else None

        """

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
    """Create list of :py:mod:`rts2saf.devices.FilterWheel`

    :var debug: enable more debug output with --debug and --level
    :var check: if True check presence of device
    :var proxy: :py:mod:`rts2.proxy`
    :var blind: blind - flavor of focus run
    :var verbose: True, more debug output
    :var rt: run time configuration,  :py:mod:`rts2saf.config.Configuration`, usually read from /usr/local/etc/rts2/rts2saf/rts2saf.cfg
    :var logger:  :py:mod:`rts2saf.log`
    :var filters:  list of :py:mod:`rts2saf.devices.Filter`
    :var  foc: :py:mod:`rts2saf.devices.Focuser`
    :var blind: blind - flavor of focus run

    """ 
   
    def __init__( self, filters=None, foc=None, blind=False, *args, **kw ):
        super( CreateFilterWheels, self ).__init__( *args, **kw )

        self.filters=filters
        self.foc=foc
        self.blind=blind
        self.filterWheelsInUse=list()

    def create(self):
        """Create a list :py:mod:`rts2saf.devices.FilterWheel` based on properties stored in configuration. 
        Optionally check if device is present. 

        :return:  list of  :py:mod:`rts2saf.devices.FilterWheel` if success else None

        """

        # all in config defined filters have no relation to a filter wheel
        filterDict= { x.name: x for x in self.filters }
        filterWheels=list()
        # create objects FilterWheel with filter wheels names and with filter objects
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
            ftw=FilterWheel(debug=self.debug, name=ftwn,filters=filters, logger=self.logger)
            filterWheels.append(ftw)

        # only used filter wheel are returned
        for ftw in filterWheels:
            if ftw.name in self.rt.cfg['FILTER WHEELS INUSE']:
                # ToDo ftw.ft.OffsetToEmptySlot==0
                self.filterWheelsInUse.append(ftw)
                
        # filter wheel with the highest filter count first
        self.filterWheelsInUse.sort(key = lambda x: len(x.filters), reverse=True)

        # find empty slots on all filter wheels
        # assumption: no combination of filters of the different filter wheels
        eSs=0
        for ftw in self.filterWheelsInUse:
            # first find in ftw.filters an empty slot 
            # use this slot first, followed by all others
            for ft in ftw.filters:
                if self.debug: self.logger.debug('create:  {0:5s}, filter:{1:5s} in use'.format(ftw.name, ft.name))
                if ft.OffsetToEmptySlot==0:
                    # ft.emptySlot=Null at instantiation
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
                if len(self.filterWheelsInUse) > 0:
                    self.logger.warn('create:  {0}, no empty slot found'.format(ftw.name))



        if eSs >= len(self.filterWheelsInUse):
            if self.debug: self.logger.debug('create: all filter wheels have an empty slot')
        else:
            self.logger.warn('create: not all filter wheels have an empty slot, {}, {}'.format(eSs,  len(self.filterWheelsInUse)))
            return None

        if self.check:
            for ftw in self.filterWheelsInUse:
                if not ftw.check(proxy=self.proxy):
                    return None
        return self.filterWheelsInUse

    def checkBounds(self):
        """Check if FOC_DEF + max/min(FOC_FOFF) are within focuser range and if the number of steps is above MINIMUM_FOCUSER_POSITIONS.

        :return: True if both conditions are met else False

        """

        # check bounds of filter lower and upper limit settings            
        anyOutOfLlUl=False
        name = self.foc.name
        focDef = self.foc.focDef
        absLowerLimit = self.foc.absLowerLimit
        absUpperLimit = self.foc.absUpperLimit

        for k, ftw in enumerate(self.filterWheelsInUse):
            for ft in ftw.filters:
                if self.blind:
                    # focuser limits are done in method create()
                    pass
                else:
                    if ft.OffsetToEmptySlot:
                        fto= ft.OffsetToEmptySlot #  it may be still None
                    else:
                        fto= 0
                        ft.OffsetToEmptySlot=0
                        self.logger.warn('checkBounds: {} has no defined filter offset, setting it to ZERO'.format(ft.name))

                    self.logger.warn('checkBounds: {} {} {}'.format(ftw.name, ft.name, ft.OffsetToEmptySlot))
                    rangeMin=focDef + min(ft.focFoff) + fto
                    rangeMax=focDef + max(ft.focFoff) + fto
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
            return False

        anyBelow=False
        # check MINIMUM_FOCUSER_POSITIONS
        for ftw in self.filterWheelsInUse:
            for ft in ftw.filters:
                if len(ft.focFoff) <= self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
                    self.logger.error( 'create: {0:8s}, filter: {1:8s} to few focuser positions: {2}<={3} (see MINIMUM_FOCUSER_POSITIONS)'.format(ftw.name, ft.name, len(ft.focFoff), self.rt.cfg['MINIMUM_FOCUSER_POSITIONS'])) 
                    anyBelow=True

        if anyBelow:
            self.logger.error('create: too few focuser positions, returning')
            return False


        return True

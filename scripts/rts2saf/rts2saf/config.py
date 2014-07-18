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
"""Config provides all required constants with default values. ToDo: This module must be rewritten in future.

"""

__author__ = 'wildi.markus@bluewin.ch'

import ConfigParser
import os
import string

# thanks http://stackoverflow.com/questions/635483/what-is-the-best-way-to-implement-nested-dictionaries-in-python
class AutoVivification(dict):
    """Implementation of perl's autovivification feature."""
    def __getitem__(self, item):
        try:
            return dict.__getitem__(self, item)
        except KeyError:
            value = self[item] = type(self)()
            return value

class DefaultConfiguration(object):
    """Default configuration for rts2saf"""    
    def __init__(self, debug=False, logger=None):

        self.debug=debug
        self.logger=logger

        self.ccd=None 
        self.foc=None
        self.sexFields=list()
        self.config = ConfigParser.RawConfigParser()
        self.config.optionxform = str

        self.dcf=dict()
        self.dcf[('basic', 'BASE_DIRECTORY')]= '/tmp/rts2saf_focus'
        self.dcf[('basic', 'TEMP_DIRECTORY')]= '/tmp/'
        self.dcf[('basic', 'FILE_GLOB')]= '*fits'

        self.dcf[('filter wheels', 'inuse')]= '[ FILTA ]'
        self.dcf[('filter wheels', 'EMPTY_SLOT_NAMES')]= [ 'empty8', 'open' ]
        # this is really ugly
        # but ConfigParser does not allow something else
        # ToDo define more!
        self.dcf[('filter wheel', 'fltw1')]= '[ FILTA, U, nof]'
        self.dcf[('filter wheel', 'fltw2')]= '[ FILTB, Y ]'
        self.dcf[('filter wheel', 'fltw3')]= '[ FILTC, nof ]'
        self.dcf[('filter wheel', 'fltw4')]= '[ FILTD, nof ]'
        #
        #                                                 relative lower acquisition limit [tick]
        #                                                       relative upper acquisition limit [tick]
        #                                                             stepsize [tick]
        #                                                                  exposure factor
        self.dcf[('filter properties', 'flt1')]= '[ U,   -1000, 1100, 100, 11.1]'
        self.dcf[('filter properties', 'flt2')]= '[ nof1,-1200, 1300, 200, 1.]'
        self.dcf[('filter properties', 'flt3')]= '[ nof2,-1200, 1300, 200, 1.]'
        self.dcf[('filter properties', 'flt4')]= '[ C,   -1400, 1500, 300, 1.]'
        self.dcf[('filter properties', 'flt5')]= '[ D,   -1400, 1500, 300, 1.]'
        self.dcf[('filter properties', 'flt6')]= '[ D,   -1400, 1500, 300, 1.]'
        self.dcf[('filter properties', 'flt7')]= '[ D,   -1400, 1500, 300, 1.]'
        self.dcf[('filter properties', 'flt8')]= '[ D,   -1400, 1500, 300, 1.]'
        self.dcf[('filter properties', 'flt9')]= '[ D,   -1400, 1500, 300, 1.]'

        self.dcf[('focuser properties', 'FOCUSER_NAME')]= 'F0'
        self.dcf[('focuser properties', 'FOCUSER_RESOLUTION')]= 20 
        self.dcf[('focuser properties', 'FOCUSER_ABSOLUTE_LOWER_LIMIT')]= 0 
        self.dcf[('focuser properties', 'FOCUSER_ABSOLUTE_UPPER_LIMIT')]= 20
        self.dcf[('focuser properties', 'FOCUSER_LOWER_LIMIT')]= 0 
        self.dcf[('focuser properties', 'FOCUSER_UPPER_LIMIT')]= 20
        self.dcf[('focuser properties', 'FOCUSER_STEP_SIZE')]= 2
        self.dcf[('focuser properties', 'FOCUSER_SPEED')]= 100.
        self.dcf[('focuser properties', 'FOCUSER_NO_FTW_RANGE')]= '[ -100, 100, 20 ]'
        self.dcf[('focuser properties', 'FOCUSER_TEMPERATURE_COMPENSATION')]= False
        # not yet in use:
        self.dcf[('acceptance circle', 'CENTER_OFFSET_X')]= 0.
        self.dcf[('acceptance circle', 'CENTER_OFFSET_Y')]= 0.
        #
        self.dcf[('acceptance circle', 'RADIUS')]= 2000.
        #
        #
        self.dcf[('analysis', 'MINIMUM_OBJECTS')]= 5
        self.dcf[('analysis', 'MINIMUM_FOCUSER_POSITIONS')]= 5
        # if non empty only FOC_POS within this interval will be analyzed
        self.dcf[('analysis', 'FOCUSER_INTERVAL')]= list()
        
        self.dcf[('SExtractor', 'SEXPATH')]= 'sextractor'
        self.dcf[('SExtractor', 'SEXCFG')]= '/usr/local/etc/rts2/rts2saf/sex/rts2saf-sex.cfg'
        self.dcf[('SExtractor', 'FIELDS')]= ['NUMBER', 'EXT_NUMBER','X_IMAGE','Y_IMAGE','MAG_BEST','FLAGS','CLASS_STAR','FWHM_IMAGE','A_IMAGE','B_IMAGE']
        # ToDo, currently put into default sex.fg
        # from sextractor config file
        # ASSOC_PARAMS     3,4          # columns of xpos,ypos[,mag] # rts2af do not use mag
        # ASSOC_RADIUS     10.0         # cross-matching radius (pixels)
        # ASSOC_TYPE       NEAREST        # ASSOCiation method: FIRST, NEAREST, MEAN,
        self.dcf[('SExtractor', 'OBJECT_SEPARATION')]= 10.
        self.dcf[('SExtractor', 'ELLIPTICITY')]= .1
        self.dcf[('SExtractor', 'ELLIPTICITY_REFERENCE')]= .3
        self.dcf[('SExtractor', 'DETECT_THRESH')]=1.7 
        self.dcf[('SExtractor', 'ANALYSIS_THRESH')]=1.7 
        self.dcf[('SExtractor', 'DEBLEND_MINCONT')]= 0.1 
        self.dcf[('SExtractor', 'SATUR_LEVEL')]= 65535
        self.dcf[('SExtractor', 'STARNNW_NAME')]= '/usr/local/etc/rts2/rts2saf/rts2saf-sex.nnw'
        # mapping as found in dummy CCD, used for set 
        self.dcf[('ccd binning mapping', '1x1')] = 0
        self.dcf[('ccd binning mapping', '2x2')] = 1
        self.dcf[('ccd binning mapping', '3x3')] = 2
        self.dcf[('ccd binning mapping', '4x4')] = 3

        self.dcf[('ccd', 'CCD_NAME')]= 'CD'
        self.dcf[('ccd', 'CCD_BINNING')]= '1x1'
        self.dcf[('ccd', 'WINDOW')]= '[ -1, -1, -1, -1 ]'
        self.dcf[('ccd', 'PIXELSIZE')]= 9.e-6 # unit meter
        self.dcf[('ccd', 'PIXELSCALE')]= 1.1 # unit arcsec/pixel
        self.dcf[('ccd', 'BASE_EXPOSURE')]= .01

        self.dcf[('mode', 'SET_FOC_DEF')]= False
        self.dcf[('mode', 'WRITE_FILTER_OFFSETS')]= True
        # ToDo, make a real alternative
        # self.dcf[('mode', 'ANALYZE_FWHM')]= True
        self.dcf[('mode', 'ANALYZE_FLUX')]= False
        self.dcf[('mode', 'ANALYZE_ASSOC')]= False
        self.dcf[('mode', 'ANALYZE_ASSOC_FRACTION')]= 0.65
        self.dcf[('mode', 'WITH_MATHPLOTLIB')]= False 
        self.dcf[('mode', 'WEIGHTED_MEANS')]= False 
        # mapping of fits header elements to canonical
        self.dcf[('fits header mapping', 'AMBIENTTEMPERATURE')]= 'HIERARCH DAVIS.DOME_TMP'
        self.dcf[('fits header mapping', 'DATETIME')]= 'JD'
        self.dcf[('fits header mapping', 'EXPOSURE')]= 'EXPOSURE'
        self.dcf[('fits header mapping', 'CCD_TEMP')]= 'CCD_TEMP'
        self.dcf[('fits header mapping', 'FOC_POS')] = 'FOC_POS'
        self.dcf[('fits header mapping', 'DATE-OBS')]= 'DATE-OBS'
        self.dcf[('fits header mapping', 'BINNING')]= 'BINNING' 
        self.dcf[('fits header mapping', 'BINNING_X')]= 'BIN_V' # seen BIN_X
        self.dcf[('fits header mapping', 'BINNING_Y')]= 'BIN_H' # seen BIN_Y
        # These factors are used for fitting
        self.dcf[('fits binning mapping', '1x1')]= 1
        self.dcf[('fits binning mapping', '2x2')]= 2
        self.dcf[('fits binning mapping', '4x4')]= 4
        self.dcf[('fits binning mapping', '8x8')]= 8

        self.dcf[('telescope', 'TEL_RADIUS')] = 0.09 # [meter]
        self.dcf[('telescope', 'TEL_FOCALLENGTH')] = 1.26 # [meter]

        self.dcf[('connection', 'URL')] = 'http://127.0.0.1:8889' 
        self.dcf[('connection', 'USERNAME')] = 'rts2saf'
        self.dcf[('connection', 'PASSWORD')] = 'set password in your config file'

        self.dcf[('queue focus run', 'FWHM_LOWER_THRESH')] = 35.

        self.dcf[('analysis', 'FWHM_MIN')] = 1.5 
        self.dcf[('analysis', 'FWHM_MAX')] = 12. 

        self.dcf[('IMGP analysis', 'FILTERS_TO_EXCLUDE')] = '[ FILTC:grism1]' 
        self.dcf[('IMGP analysis', 'SCRIPT_FWHM')] = '/usr/local/bin/rts2saf_fwhm.py' 
        self.dcf[('IMGP analysis', 'SCRIPT_ASTROMETRY')] = '/usr/local/bin/rts2-astrometry.net' 
        # or rts2-astrometry.net


    def writeDefaultConfiguration(self, cfn='./rts2saf-default.cfg'):
        """Write the default configuration to file, serves as a starting point.

        :param cfn: file name
        :type string:
        :return cfn: file name if success else None
        """

        for (section, identifier), value in sorted(self.dcf.iteritems()):
            if self.config.has_section(section)== False:
                self.config.add_section(section)

            self.config.set(section, identifier, value)
        try:
            with open( cfn, 'w') as configfile:
                configfile.write('# 2013-09-10, Markus Wildi\n')
                configfile.write('# default configuration for rts2saf\n')
                configfile.write('#\n')
                configfile.write('#\n')
                self.config.write(configfile)
        except Exception, e:
            self.logger.error('Configuration.writeDefaultConfiguration: config file: {0} could not be written, error: {1}'.format(cfn,e))
            return None
        return cfn

class Configuration(DefaultConfiguration):
    """Helper class containing the runtime configuration.
    """
    # init from base class
    def readConfiguration(self, fileName=None):
        """Copy the default configuration and overwrite the values with those from configuration file.

        :return: True if success else False

        """
        # make the values accessible
        self.cfg=AutoVivification()
        # TODO
        filterWheelsInuse=list()
        filterWheelsDefs=dict()
        config = ConfigParser.ConfigParser()
        config.optionxform = str


        if os.path.exists(fileName):
            try:
                config.readfp(open(fileName))
            except Exception, e:                
                self.logger.error('Configuration.readConfiguration: config file: {0} has wrong syntax, error: {1}'.format(fileName,e))
                return False
            # ok, I misuse ConfigParser
            # check additional elements or typo
            for sct in config.sections():
                for k,v in config.items(sct): 
                    try:
                        self.dcf[(sct, k)]
                    except Exception, e:
                        self.logger.error('Configuration.readConfiguration: config file: {0} has wrong syntax, error: {1}'.format(fileName,e))
                        return False

        else:
            self.logger.error('Configuration.readConfiguration: config file: {0} not found'.format(fileName))
            return False

        self.cfg['CFGFN'] = fileName
        # read the defaults
        for (section, identifier), value in self.dcf.iteritems():
            #
            # ToDO ugly
            if section == 'ccd' :
                self.cfg[identifier]= value
                
            elif section in 'fits binning mapping' or section in 'ccd binning mapping':
                self.cfg[section][identifier]= value

            else:
                self.cfg[identifier]= value

        # over write the defaults
        ftds=list()
        # if there is no filter wheel defined, a FAKE wheel with one FAKE filter is created
        fakeFtw=True
        for (section, identifier), value in self.dcf.iteritems():
            try:
                value = config.get( section, identifier)
            except Exception, e:
                # exception if section, identifier value are not present in config file
                #self.logger.error('Configuration.readConfiguration: config file: {0} has an error at section:{1}, identifier:{2}, value:{3}'.format(fileName, section, identifier, value))
                continue

            value= string.replace( value, ' ', '')

            items=list()
            # decode the compound configuration expressions first, the rest is copied to  
            # after completion
            if section=='SExtractor':
                if identifier in 'FIELDS':
                    value=value.replace("'", '')
                    self.cfg['FIELDS']=value[1:-1].split(',')
                else:
                    self.cfg[identifier]= value

            elif section=='basic': 
                if isinstance(self.cfg[identifier], bool):
                    # ToDo, looking for a direct way
                    if value in 'True':
                        self.cfg[identifier]= True
                    else:
                        self.cfg[identifier]= False
                else:
                    self.cfg[identifier]= value

            elif section=='focuser properties':
                if identifier in 'FOCUSER_NO_FTW_RANGE':
                    self.cfg[identifier]=value[1:-1].split(',')
                else:
                    self.cfg[identifier]= value
            #
            elif section=='filter properties': 
                self.cfg[identifier]= value
                ftds.append(value)
            #
            elif section=='filter wheel':
                items= value[1:-1].split(',')
                filterWheelsDefs[items[0]]=[ x for x in items[1:] if x is not '']
            #
            elif( section=='filter wheels'):
                fakeFtw=False
                if identifier in 'inuse':
                    filterWheelsInuse=value[1:-1].split(',')
                    self.cfg[identifier]=filterWheelsInuse
                elif identifier in 'EMPTY_SLOT_NAMES':
                    self.cfg[identifier]=value[1:-1].split(',')
            #
            elif( section == 'ccd' and identifier == 'WINDOW'):
                items= value[1:-1].split(',')
                self.cfg[identifier] = [ int(x) for x in items ]
                if len(self.cfg[identifier]) != 4:
                    self.logger.warn( 'Configuration.readConfiguration: wrong ccd window specification {0} {1}, using the whole CCD area'.format(len(self.cfg[identifier]), self.cfg[identifier]))
                    self.cfg[identifier] = [ -1, -1, -1, -1]

            elif( section=='analysis') and identifier == 'FOCUSER_INTERVAL':
                items= value[1:-1].split(',')
                self.cfg[identifier] = [ int(x) for x in items ]
                if len(self.cfg[identifier]) != 2:
                    self.logger.warn( 'Configuration.readConfiguration: wrong focuser interval specification {0} {1}, using all images'.format(len(self.cfg[identifier]), self.cfg[identifier]))
                    self.cfg[identifier] = list()

            elif( section=='IMGP analysis'):
                items= value[1:-1].split(',')
                if identifier in 'FILTERS_TO_EXCLUDE':
                    tDict=dict()
                    for e in value[1:-1].split(','):
                        k,v=e.split(':')
                        tDict[v]=k # that's ok !!
                    self.cfg[identifier]=tDict
                else:
                    self.cfg[identifier]= value

            elif( section=='fits binning mapping'):
                # exception
                self.cfg[section][identifier]= value
            elif( section=='ccd binning mapping'):
                # exception
                self.cfg[section][identifier]= value
            # first bool, then int !
            elif isinstance(self.cfg[identifier], bool):
                # ToDo, looking for a direct way
                if value in 'True':
                    self.cfg[identifier]= True
                else:
                    self.cfg[identifier]= False
            elif( isinstance(self.cfg[identifier], int)):
                try:
                    self.cfg[identifier]= int(value)
                except Exception, e:
                    self.logger.error('Configuration.readConfiguration: no int '+ value+ ' in section ' +  section + ', identifier ' +  identifier + ' in file ' + fileName+ ', error: {0}'.format(e))
                    
            elif(isinstance(self.cfg[identifier], float)):
                try:
                    self.cfg[identifier]= float(value)
                except Exception, e:
                    self.logger.error('Configuration.readConfiguration: no float '+ value+ 'in section ' +  section + ', identifier ' +  identifier + ' in file ' + fileName + ', error: {0}'.format(e))

            else:
                self.cfg[identifier]= value

        # for convenience
        # ToDo look!
        self.cfg['FAKE'] = fakeFtw                

        if self.cfg['FAKE']:
            self.cfg['FILTER DEFINITIONS'] = ['FAKE_FT']
            self.cfg['FILTER WHEEL DEFINITIONS'] = {'FAKE_FTW': [ 'FAKE_FT'] }
            self.cfg['FILTER WHEELS INUSE'] = [ 'FAKE_FTW' ]
        else:
            self.cfg['FILTER DEFINITIONS'] = ftds
            self.cfg['FILTER WHEEL DEFINITIONS'] = filterWheelsDefs
            self.cfg['FILTER WHEELS INUSE'] = filterWheelsInuse


        self.cfg['FITS_BINNING_MAPPING'] = self.cfg['fits binning mapping'] 
        self.cfg['CCD_BINNING_MAPPING'] = self.cfg['ccd binning mapping'] 
        return True

    def writeConfiguration(self, cfn='./rts2saf-my-new.cfg'):
        for (section, identifier), value in sorted(self.dcf.iteritems()):
            print section, '=>', identifier, '=>', value
            if self.config.has_section(section)== False:
                self.config.add_section(section)

            self.config.set(section, identifier, value)
        
        with open( cfn, 'w') as configfile:
            configfile.write(' 2013-09-10, Markus Wildi\n')
            configfile.write(' default configuration for rts2saf\n')
            configfile.write('\n')
            configfile.write('\n')
            self.config.write(configfile)


    def checkConfiguration(self, args=None):
        """Check the runtime configuration e.g. if SExtractor is present or if the filter wheel definitions and filters are consistent.

        :return: True if success else False
        """
        # rts2.sextractor excepts the file not found error and uses internal defaults, we check that here
        if not os.path.exists(self.cfg['SEXPATH']):
            self.logger.warn( 'Configuration.checkConfiguration: sextractor path: {0} not valid, returning'.format(self.cfg['SEXPATH']))            
            return False
        if not os.path.exists(self.cfg['SEXCFG']):
            self.logger.warn( 'Configuration.checkConfiguration: SExtractor config file: {0} not found, returning'.format(self.cfg['SEXCFG']))            
            return False
        if not os.path.exists(self.cfg['STARNNW_NAME']):
            self.logger.warn( 'Configuration.checkConfiguration: SExtractor NNW config file: {0} not found, returning'.format(self.cfg['STARNNW_NAME']))            
            return False
        if not self.cfg['FIELDS']:
            self.logger.warn( 'Configuration.checkConfiguration: no sextractor fields defined, returning')
            return False

        ftws = self.cfg['FILTER WHEEL DEFINITIONS'].keys()
        fts=list()
        for x in self.cfg['FILTER DEFINITIONS']:
            ele= x.strip('[]').split(',')
            fts.append(ele[0])

        for ftw in self.cfg['FILTER WHEELS INUSE']:
            if ftw  not in ftws:
                self.logger.warn( 'Configuration.checkConfiguration: filter wheel: {} not defined in: {}'.format(ftw, ftws))
                return False
            for ftName in self.cfg['FILTER WHEEL DEFINITIONS'][ftw]:
                if ftName not in fts:
                    self.logger.warn( 'Configuration.checkConfiguration: filter: {} not defined in: {}'.format(ftName, self.cfg['FILTER DEFINITIONS']))
                    return False


        try:
            vars(args)['associate']
            if not 'NUMBER' in self.cfg['FIELDS']:
                self.logger.error( 'Configuration.checkConfiguration: with --associate specify SExtractor parameter NUMBER in FIELDS: {}'.format( self.cfg['FIELDS']))
                return False
        except:
            pass

        try:
            vars(args)['flux']
            for fld in ['FLUX_MAX' , 'FLUX_APER', 'FLUXERR_APER']:
                if  fld in self.cfg['FIELDS']:
                    self.logger.error( 'Configuration.checkConfiguration: with --flux do not specify SExtractor parameter: {}  in FIELDS: {}'.format( fld, self.cfg['FIELDS']))
                    return False
        except:
            pass
                    

        return True
        # more to come

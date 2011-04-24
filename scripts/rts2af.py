"""Class definitions for rts2_autofocus"""
# (C) 2010, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rts_autofocus.py --help
#   
#   see man 1 rts2_autofocus.py
#   see rts2_autofocus_unittest.py for unit tests
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
# required packages:
# wget 'http://argparse.googlecode.com/files/argparse-1.1.zip

__author__ = 'markus.wildi@one-arcsec.org'

import argparse
import logging
import stat
import os
from operator import itemgetter
# globals


class AFScript:
    """Class for any AF script"""
    def __init__(self, scriptName):
        self.scriptName= scriptName

    def arguments( self): 
        

        parser = argparse.ArgumentParser(description='RTS2 autofocus', epilog="See man 1 rts2-autofocus.py for mor information")
#        parser.add_argument(
#            '--write', action='store', metavar='FILE NAME', 
#            type=argparse.FileType('w'), 
#            default=sys.stdout,
#            help='the file name where the default configuration will be written default: write to stdout')

        parser.add_argument(
            '-w', '--write', action='store_true', 
            help='write defaults to configuration file ' + runTimeConfig.configFileName)

        parser.add_argument('--config', dest='fileName',
                            metavar='CONFIG FILE', nargs=1, type=str,
                            help='configuration file')

        parser.add_argument('-r', '--reference', dest='referenceFitsFileName',
                            metavar='REFERENCE', nargs=1, type=str,
                            help='reference file name')

        parser.add_argument('-l', '--loglevel', dest='logLevel', action='store', 
                            metavar='LOGLEVEL', nargs=1, type=str,
                            default='warning',
                            help=' log level: usual levels')

        parser.add_argument('-t', '--logto', dest='logTo', action='store', 
                            metavar='LOGTO', nargs=1, type=str,
                            default='-',
                            help=' log file: filename or - for stdout')

#        parser.add_argument('-t', '--test', dest='test', action='store', 
#                            metavar='TEST', nargs=1,
#    no default means None                        default='myTEST',
#                            help=' test case, default: myTEST')

        parser.add_argument('-v', dest='verbose', 
                            action='store_true',
                            help=' print (some) messages to stdout'
                            )

        self.args= parser.parse_args()
        self.logformat= '%(asctime)s %(levelname)s %(message)s'

        if(self.args.logTo[0] == '-'):
            logging.basicConfig(level=logging.WARN, format='%(asctime)s %(levelname)s %(message)s')
        else:
            logging.basicConfig(filename=self.args.logTo[0], level=logging.INFO, format= self.logformat)

        if(self.args.verbose):
            global verbose
            verbose= self.args.verbose
            if( verbose):
                runTimeConfig.dumpDefaults()

        if( self.args.write):
            runTimeConfig.writeDefaultConfiguration()
            print 'wrote default configuration to ' +  runTimeConfig.configurationFileName()
            sys.exit(0)

        return  self.args


import ConfigParser
import string

class Configuration:
    """Configuration for any AFScript"""    
    def __init__(self, fileName='rts2af-offline.cfg'):
        self.configFileName = fileName
        self.values={}
        self.filters=[]
        self.filtersInUse=[]

        self.cp={}

        self.config = ConfigParser.RawConfigParser()
        
        self.cp[('basic', 'CONFIGURATION_FILE')]= '/etc/rts2/rts2af/rts2af-acquire.cfg'
        
        self.cp[('basic', 'BASE_DIRECTORY')]= '/tmp'
        self.cp[('basic', 'TEMP_DIRECTORY')]= '/tmp'
        self.cp[('basic', 'FILE_GLOB')]= '*fits'
        self.cp[('basic', 'FITS_IN_BASE_DIRECTORY')]= False
        self.cp[('basic', 'CCD_CAMERA')]= 'CD'
        #                                        name
        #                                           offset to clear path
        #                                                 relative lower acquisition limit [tick]
        #                                                        relative upper acquisition limit [tick]
        #                                                              stepsize [tick]
        #                                                                   exposure time [sec]
        self.cp[('filter properties', 'f01')]= '[U, 5074, -1500, 1500, 100, 40]'
        self.cp[('filter properties', 'f02')]= '[B, 4712, -1500, 1500, 100, 30]'
        self.cp[('filter properties', 'f03')]= '[V, 4678, -1500, 1500, 100, 20]'
        self.cp[('filter properties', 'f04')]= '[R, 4700, -1500, 1500, 100, 20]'
        self.cp[('filter properties', 'f05')]= '[I, 4700, -1500, 1500, 100, 20]'
        self.cp[('filter properties', 'f06')]= '[X, 3275, -1500, 1500, 100, 10]'
        self.cp[('filter properties', 'f07')]= '[H, 3446, -1500, 1500, 100, 10]'
        self.cp[('filter properties', 'f08')]= '[b, 3446, -1500, 1500, 100, 10]'
        self.cp[('filter properties', 'f09')]= '[c, 3446, -1500, 1500, 100, 10]'
        self.cp[('filter properties', 'f0a')]= '[d, 3446, -1500, 1500, 100, 10]'
        self.cp[('filter properties', 'f0b')]= '[e, 3446, -1500, 1500, 100, 10]'
        self.cp[('filter properties', 'f0c')]= '[f, 3446, -1500, 1500, 100, 10]'
        self.cp[('filter properties', 'f0d')]= '[g, 3446, -1500, 1500, 100, 10]'
        self.cp[('filter properties', 'f0e')]= '[h, 3446, -1500, 1500, 100, 10]'
        self.cp[('filter properties', 'nofilter')]= '[NOFILTER, 3446, -1500, 1500, 109, 19]'
        
        self.cp[('focuser properties', 'FOCUSER_RESOLUTION')]= 20
        self.cp[('focuser properties', 'FOCUSER_ABSOLUTE_LOWER_LIMIT')]= 1501
        self.cp[('focuser properties', 'FOCUSER_ABSOLUTE_UPPER_LIMIT')]= 6002

        self.cp[('acceptance circle', 'CENTER_OFFSET_X')]= 0.
        self.cp[('acceptance circle', 'CENTER_OFFSET_Y')]= 0.
        self.cp[('acceptance circle', 'RADIUS')]= 2000.
        
        self.cp[('filters', 'filters')]= 'U:B:V:R:I:X:Y'
        
        self.cp[('DS9', 'DS9_REFERENCE')]= True
        self.cp[('DS9', 'DS9_MATCHED')]= True
        self.cp[('DS9', 'DS9_ALL')]= True
        self.cp[('DS9', 'DS9_DISPLAY_ACCEPTANCE_AREA')]= True
        self.cp[('DS9', 'DS9_REGION_FILE')]= 'ds9-autofocus.reg'
        
        self.cp[('analysis', 'ANALYSIS_FOCUS_UPPER_LIMIT')]= 10000
        self.cp[('analysis', 'ANALYSIS_FOCUS_LOWER_LIMIT')]= 0
        self.cp[('analysis', 'MINIMUM_OBJECTS')]= 20
        self.cp[('analysis', 'MINIMUM_FOCUSER_POSITIONS')]= 5
        self.cp[('analysis', 'INCLUDE_AUTO_FOCUS_RUN')]= False
        self.cp[('analysis', 'SET_LIMITS_ON_SANITY_CHECKS')]= True
        self.cp[('analysis', 'SET_LIMITS_ON_FILTER_FOCUS')]= True
        self.cp[('analysis', 'FIT_RESULT_FILE')]= 'fit-autofocus.dat'
        self.cp[('analysis', 'MATCHED_RATIO')]= 0.1
        
        self.cp[('fitting', 'FOCROOT')]= 'rts2-fit-focus'
        self.cp[('fitting', 'DISPLAYFIT')]= True
        
        self.cp[('SExtractor', 'SEXPRG')]= 'sex 2>/dev/null'
        self.cp[('SExtractor', 'SEXCFG')]= '/etc/rts2/autofocus/sex-autofocus.cfg'
        self.cp[('SExtractor', 'SEXPARAM')]= '/etc/rts2/autofocus/sex-autofocus.param'
        self.cp[('SExtractor', 'SEXREFERENCE_PARAM')]= '/etc/rts2/autofocus/sex-autofocus-reference.param'
        self.cp[('SExtractor', 'OBJECT_SEPARATION')]= 10.
        self.cp[('SExtractor', 'ELLIPTICITY')]= .1
        self.cp[('SExtractor', 'ELLIPTICITY_REFERENCE')]= .3
        self.cp[('SExtractor', 'SEXSKY_LIST')]= 'sex-autofocus-assoc-sky.list'
        self.cp[('SExtractor', 'SEXCATALOGUE')]= 'sex-autofocus.cat'
        self.cp[('SExtractor', 'SEX_TMP_CATALOGUE')]= 'sex-autofocus-tmp.cat'
        self.cp[('SExtractor', 'CLEANUP_REFERENCE_CATALOGUE')]= True
        
        self.cp[('mode', 'SET_FOCUS')]= True
        self.cp[('mode', 'CCD_BINNING')]= 0
        self.cp[('mode', 'AUTO_FOCUS')]= False
        self.cp[('mode', 'NUMBER_OF_AUTO_FOCUS_IMAGES')]= 10
        
        
        self.defaults={}
        for (section, identifier), value in sorted(self.cp.iteritems()):
            self.defaults[(identifier)]= value
        #            print 'value ', self.defaults[(identifier)]
 

    def configurationFileName(self):
        return  self.defaults['CONFIGURATION_FILE']

    
    def configIdentifiers(self):
        return sorted(self.cp.iteritems())

    def defaultsIdentifiers(self):
        return sorted(self.defaults.iteritems())
        
    def defaultsValue( self, identifier):
        return self.defaults[ identifier]

    def dumpDefaults(self):
        for (identifier), value in self.configIdentifiers():
            print "dump defaults :", ', ', identifier, '=>', value

    def valuesIdentifiers(self):
        return sorted(self.values.iteritems())
    # why not values?
    #  runTimeConfig.values('SEXCFG'), TypeError: 'dict' object is not callable
    def value( self, identifier):
        return self.values[ identifier]

    def dumpValues(self):
        for (identifier), value in self.valuesIdentifiers():
            print "dump values :", ', ', identifier, '=>', value

    def filterByName(self, name):
        for filter in  self.filters:
            print "NAME>" + name + "<>" + filter.name
            if( name == filter.name):
                print "NAME: {0} {1}".format(name, filter.name)
                return filter

        return False

    def filterInUseByName(self, name):
        for filter in  self.filtersInUse:
            if( name == filter.name):
                return filter
        return False

    def writeDefaultConfiguration(self):
        for (section, identifier), value in sorted(self.cp.iteritems()):
            print section, '=>', identifier, '=>', value
            if( self.config.has_section(section)== False):
                self.config.add_section(section)

            self.config.set(section, identifier, value)


        with open( self.configFileName, 'w') as configfile:
            configfile.write('# 2010-07-10, Markus Wildi\n')
            configfile.write('# default configuration for rts2af-autofocus.py\n')
            configfile.write('# generated with rts2-autofous.py -p\n')
            configfile.write('# see man rts2af-autofocus.py for details\n')
            configfile.write('#\n')
            configfile.write('#\n')
            self.config.write(configfile)

    def writeConfigurationForFilter(self, fileName=None, filter=None):

        self.cp[( 'filters',  'filters')]= filter

        for (section, identifier), value in sorted(self.cp.iteritems()):
            if( self.config.has_section(section)== False):
                self.config.add_section(section)

            self.config.set(section, identifier, value)


        with open( fileName, 'w') as configfile:
            configfile.write('# 2011-04-19, Markus Wildi\n')
            configfile.write('# default configuration for rts2af-autofocus.py\n')
            configfile.write('# generated with rts2-autofous.py -p\n')
            configfile.write('# see man rts2af-autofocus.py for details\n')
            configfile.write('#\n')
            configfile.write('#\n')
            self.config.write(configfile)


    def readConfiguration( self, configFileName):
        verbose=False
        config = ConfigParser.ConfigParser()
        try:
            config.readfp(open(configFileName))
        except:
            logging.error('Configuration.readConfiguration: config file >' + configFileName + '< not found or wrong syntax, exiting')
            sys.exit(1)

# read the defaults
        for (section, identifier), value in self.configIdentifiers():
            self.values[identifier]= value
# over write the defaults
        for (section, identifier), value in self.configIdentifiers():

            try:
                value = config.get( section, identifier)
            except:
                logging.info('Configuration.readConfiguration: no section ' +  section + ' or identifier ' +  identifier + ' in file ' + configFileName)
            # overwrite the default configuration (if needed)
            self.cp[( section,  identifier)]= value

            # first bool, then int !
            if(isinstance(self.values[identifier], bool)):
#ToDO, looking for a direct way
                if( value == 'True'):
                    self.values[identifier]= True
                else:
                    self.values[identifier]= False
            elif( isinstance(self.values[identifier], int)):
                try:
                    self.values[identifier]= int(value)
                except:
                    logging.error('Configuration.readConfiguration: no int '+ value+ 'in section ' +  section + ', identifier ' +  identifier + ' in file ' + configFileName)
                    
            elif(isinstance(self.values[identifier], float)):
                try:
                    self.values[identifier]= float(value)
                except:
                    logging.error('Configuration.readConfiguration: no float '+ value+ 'in section ' +  section + ', identifier ' +  identifier + ' in file ' + configFileName)

            else:
                self.values[identifier]= value
                items=[] ;
                if( section == 'filter properties'): 
                    items= value[1:-1].split(',')
#, ToDo, hm
                    for item in items: 
                        item= string.replace( item, ' ', '')

                    items[0]= string.replace( items[0], ' ', '')
                    if(verbose):
                        print '-----------filterInUse---------:>' + items[0] + '<'
                    self.filters.append( Filter( items[0], string.atoi(items[1]), string.atoi(items[2]), string.atoi(items[3]), string.atoi(items[4]), string.atoi(items[5])))
                elif( section == 'filters'):
                    items= value.split(':')
                    for item in items:
                        item.replace(' ', '')
                        if(verbose):
                            print 'filterInUse---------:>' + item + '<'
                        self.filtersInUse.append(item)

                    
        if(verbose):
            for (identifier), value in self.defaultsIdentifiers():
                print "over ", identifier, self.values[(identifier)]

        if(verbose):
            for filter in self.filters:
                print 'Filter --------' + filter.name
                print 'Filter --------%d'  % filter.exposure 
                print 'Filter --------%d'  % filter.OffsetToClearPath


        if(verbose):
            for filter in self.filtersInUse:
                print "used filters :", filter

        return self.values

class SExtractorParams():
    def __init__(self, paramsFileName=None):
# ToDo, clarify 
        if( paramsFileName== None):
            self.paramsFileName= runTimeConfig.value('SEXREFERENCE_PARAM')
        else:
            self.paramsFileName= paramsFileName
        self.reference= []
        self.assoc= []

    def elementIndex(self, element):
        for ele in  self.assoc:
            if( element== ele) : 
                return self.assoc.index(ele)
        else:
            return False

    def readSExtractorParams(self):
        params=open( self.paramsFileName, 'r')
        lines= params.readlines()
        pElement = re.compile( r'([\w]+)')
        for (i, line) in enumerate(lines):
            element = pElement.match(line)
            if( element):
                if(verbose):
                    print "element.group(1) : ", element.group(1)
                self.reference.append(element.group(1))
                self.assoc.append(element.group(1))

# current structure of the reference catalogue
#   1 NUMBER                 Running object number
#   2 X_IMAGE                Object position along x                                    [pixel]
#   3 Y_IMAGE                Object position along y                                    [pixel]
#   4 FLUX_ISO               Isophotal flux                                             [count]
#   5 FLUX_APER              Flux vector within fixed circular aperture(s)              [count]
#   6 FLUXERR_APER           RMS error vector for aperture flux(es)                     [count]
#   7 MAG_APER               Fixed aperture magnitude vector                            [mag]
#   8 MAGERR_APER            RMS error vector for fixed aperture mag.                   [mag]
#   9 FLUX_MAX               Peak flux above background                                 [count]
#  10 ISOAREA_IMAGE          Isophotal area above Analysis threshold                    [pixel**2]
#  11 FLAGS                  Extraction flags
#  12 FWHM_IMAGE             FWHM assuming a gaussian core                              [pixel]
#  13 FLUX_RADIUS            Fraction-of-light radii                                    [pixel]
#  14 ELLIPTICITY            1 - B_IMAGE/A_IMAGE

# current structure of the associated catalogue
#   1 NUMBER                 Running object number
#   2 X_IMAGE                Object position along x                                    [pixel]
#   3 Y_IMAGE                Object position along y                                    [pixel]
#   4 FLUX_ISO               Isophotal flux                                             [count]
#   5 FLUX_APER              Flux vector within fixed circular aperture(s)              [count]
#   6 FLUXERR_APER           RMS error vector for aperture flux(es)                     [count]
#   7 MAG_APER               Fixed aperture magnitude vector                            [mag]
#   8 MAGERR_APER            RMS error vector for fixed aperture mag.                   [mag]
#   9 FLUX_MAX               Peak flux above background                                 [count]
#  10 ISOAREA_IMAGE          Isophotal area above Analysis threshold                    [pixel**2]
#  11 FLAGS                  Extraction flags
#  12 FWHM_IMAGE             FWHM assuming a gaussian core                              [pixel]
#  13 FLUX_RADIUS            Fraction-of-light radii                                    [pixel]
#  14 ELLIPTICITY            1 - B_IMAGE/A_IMAGE
#  15 VECTOR_ASSOC           ASSOCiated parameter vector
#  29 NUMBER_ASSOC           Number of ASSOCiated IDs
            
        for element in self.reference:
            self.assoc.append('ASSOC_'+ element)

        self.assoc.append('NUMBER_ASSOC')

        if(verbose):
            for element in  self.reference:
                print "Reference element : >"+ element+"<"
            for element in  self.assoc:
                print "Association element : >" + element+"<"
class Setting:
    """Class holding settings for a given position, offset and exposure time"""
    def __init__(self, offset=0, exposure=0):
        self.offset= offset
        self.exposure = exposure
                
class Filter:
    """Class for filter properties"""

    def __init__(self, name, OffsetToClearPath, lowerLimit, upperLimit, resolution, exposure):
        self.name= name
        self.OffsetToClearPath    = OffsetToClearPath
        self.lowerLimit= lowerLimit
        self.upperLimit= upperLimit
        self.stepSize  = resolution # [tick]
        self.exposure  = exposure
        self.settings=[]
        for offset in range (self.lowerLimit, self.upperLimit +  self.stepSize,  self.stepSize):
            self.settings.append(Setting( offset, self.exposure))

        
class SXObject():
    """Class holding the used properties of SExtractor object"""
    def __init__(self, objectNumber=None, focusPosition=None, position=None, fwhm=None, flux=None, associatedSXobject=None, separationOK=True, propertiesOK=True, acceptanceOK=True):
        self.objectNumber= objectNumber
        self.focusPosition= focusPosition
        self.position= position # is a list (x,y)
        self.matched= False
        self.fwhm= fwhm
        self.flux= flux
        self.associatedSXobject= associatedSXobject
        self.separationOK= separationOK
        self.propertiesOK= propertiesOK
        self.acceptanceOK= acceptanceOK

    def printPosition(self):
        print "=== %f %f" %  (self.x, self.y)
    
    def isValid(self):
        if( self.objectNumber != None):
            if( self.x != None):
                if( self.y != None):
                    if( self.fwhm != None):
                        if( self.flux != None):
                            return True
        return False


class SXReferenceObject(SXObject):
    """Class holding the used properties of SExtractor object"""
    def __init__(self, objectNumber=None, focusPosition=None, position=None, fwhm=None, flux=None, separationOK=True, propertiesOK=True, acceptanceOK=True):
        self.objectNumber= objectNumber
        self.focusPosition= focusPosition
        self.position= position # is a list (x,y)
        self.matchedReference= False
        self.foundInAll= False
        self.fwhm= fwhm
        self.flux= flux
        self.separationOK= separationOK
        self.propertiesOK= propertiesOK
        self.acceptanceOK= acceptanceOK
        self.multiplicity=0
        self.numberOfMatches=0
        self.matchedsxObjects=[]


    def append(self, sxObject):
        self.matchedsxObjects.append(sxObject)
        

import shlex
import subprocess
import re
from math import sqrt
class Catalogue():
    """Class for a catalogue (SExtractor result)"""
    def __init__(self, fitsHDU=None, SExtractorParams=None, referenceCatalogue=None):
        self.fitsHDU  = fitsHDU
        self.catalogueFileName= serviceFileOp.expandToCat(self.fitsHDU)
        self.ds9RegionFileName= serviceFileOp.expandToDs9RegionFileName(self.fitsHDU)
        self.lines= []
        self.catalogue = {}
        self.sxObjects = {}
        self.multiplicity = {}
        self.isReference = False
        self.isValid     = False
        self.SExtractorParams = SExtractorParams
        self.referenceCatalogue= referenceCatalogue
        self.indexeFlag       = self.SExtractorParams.assoc.index('FLAGS')  
        self.indexellipticity = self.SExtractorParams.assoc.index('ELLIPTICITY')
        
        Catalogue.__lt__ = lambda self, other: self.fitsHDU.headerElements['FOC_POS'] < other.fitsHDU.headerElements['FOC_POS']

    def runSExtractor(self):
        if( verbose):
            print 'sextractor  ' + runTimeConfig.value('SEXPRG')
            print 'sextractor  ' + runTimeConfig.value('SEXCFG')
            print 'sextractor  ' + runTimeConfig.value('SEXREFERENCE_PARAM')

        # 2>/dev/null swallowed by PIPE
        (prg, arg)= runTimeConfig.value('SEXPRG').split(' ')

        cmd= [  prg,
                self.fitsHDU.fitsFileName, 
                '-c ',
                runTimeConfig.value('SEXCFG'),
                '-CATALOG_NAME',
                self.catalogueFileName,
                '-PARAMETERS_NAME',
                runTimeConfig.value('SEXPARAM'),
                '-ASSOC_NAME',
                serviceFileOp.expandToSkyList(self.fitsHDU)
                ]
        try:
            output = subprocess.Popen( cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]

        except OSError as (errno, strerror):
            logging.error( 'Catalogue.runSExtractor: I/O error({0}): {1}'.format(errno, strerror))
            sys.exit(1)

        except:
            logging.error('Catalogue.runSExtractor: '+ repr(cmd) + ' died')
            sys.exit(1)

# checking against reference items see http://www.wellho.net/resources/ex.php4?item=y115/re1.py
    def createCatalogue(self):
        if( self.SExtractorParams==None):
            logging.error( 'Catalogue.createCatalogue: no SExtractor parameter configuration given')
            return False
    
        #if( not self.fitsHDU.isReference):
        SXcat= open( self.catalogueFileName, 'r')
        #else:
        #    SXcat= open( serviceFileOp.expandToSkyList(self.fitsHDU), 'r')

        self.lines= SXcat.readlines()
        SXcat.close()

        # rely on the fact that first line is stored as first element in list
        # match, element, data
        ##   2 X_IMAGE                Object position along x                                    [pixel]
        #         1   2976.000      7.473     2773.781     219.8691     42.19405  -5.8554   0.2084     120.5605         6  26     4.05      5.330    0.823
 
        pElement = re.compile( r'#[ ]+([0-9]+)[ ]+([\w]+)')
        pData    = re.compile( r'')
        sxObjectNumber= -1
        itemNrX_IMAGE     = self.SExtractorParams.assoc.index('X_IMAGE')
        itemNrY_IMAGE     = self.SExtractorParams.assoc.index('Y_IMAGE')
        itemNrFWHM_IMAGE  = self.SExtractorParams.assoc.index('FWHM_IMAGE')
        itemNrFLUX_MAX    = self.SExtractorParams.assoc.index('FLUX_MAX')
        itemNrASSOC_NUMBER= self.SExtractorParams.assoc.index('ASSOC_NUMBER')

        for (lineNumber, line) in enumerate(self.lines):
            element = pElement.match(line)
            data    = pData.match(line)
            x= -1
            y= -1
            sxObjectNumberASSOC= '-1'
            if( element):
                if( not ( element.group(2)== 'VECTOR_ASSOC')):
                    try:
                        self.SExtractorParams.assoc.index(element.group(2))
                    except:
                        logging.error( 'Catalogue.createCatalogue: no matching element for ' + element.group(2) +' found')
                        break
            elif( data):
                items= line.split()
                sxObjectNumber= items[0] # NUMBER
                sxObjectNumberASSOC= items[itemNrASSOC_NUMBER]
                
                for (j, item) in enumerate(items):
                    self.catalogue[(sxObjectNumber, self.SExtractorParams.assoc[j])]=  float(item)


                self.sxObjects[sxObjectNumber]= SXObject(sxObjectNumber, self.fitsHDU.headerElements['FOC_POS'], (float(items[itemNrX_IMAGE]), float(items[itemNrY_IMAGE])), float(items[itemNrFWHM_IMAGE]), float(items[itemNrFLUX_MAX]), items[itemNrASSOC_NUMBER]) # position, bool is used for clean up (default True, True)
                if( sxObjectNumberASSOC in self.multiplicity): # interesting
                    self.multiplicity[sxObjectNumberASSOC] += 1
                else:
                    self.multiplicity[sxObjectNumberASSOC]= 1
            else:
                logging.error( 'Catalogue.readCatalogue: no match on line %d' % lineNumber)
                logging.error( 'Catalogue.readCatalogue: ' + line)
                break

        else: # exhausted 
            logging.info( 'Catalogue.readCatalogue: catalogue created ' + self.fitsHDU.fitsFileName)
            self.isValid= True

        return self.isValid

    def printCatalogue(self):
        for (sxObjectNumber,identifier), value in sorted( self.catalogue.iteritems()):
# ToDo, remove me:
            if( sxObjectNumber== "12"):
                print  "printCatalogue " + sxObjectNumber + ">"+ identifier + "< %f"% value 

    def printObject(self, sxObjectNumber):
        for identifier in self.SExtractorParams.identifiersAssoc:
            if(((sxObjectNumber, identifier)) in self.catalogue):
                if(verbose):
                    print "printObject: object number " + sxObjectNumber + " identifier " + identifier + "value %f" % self.catalogue[(sxObjectNumber, identifier)]
            else:
                logging.error( "Catalogue.printObject: object number " + sxObjectNumber + " for >" + identifier + "< not found, break")
                break
        else:
#                logging.error( "Catalogue.printObject: for object number " + sxObjectNumber + " all elements printed")
            return True

        return False
    
    def ds9DisplayCatalogue(self, color="green", load=True):
        import ds9
        onscreenDisplay = ds9.ds9()
        if( load):
            onscreenDisplay.set('file {0}'.format(self.fitsHDU.fitsFileName))
            # does not work (yet)       onscreenDisplay.set('global color=yellow')
        try:
            onscreenDisplay.set('regions', 'image; point ({0} {1}) '.format(self.referenceCatalogue.circle.transformedCenterX,self.referenceCatalogue.circle.transformedCenterY) + '# color=magenta font=\"helvetica 10 normal\" select=1 highlite=1 edit=1 move=1 delete=1 include=1 fixed=0 source text = {center acceptance}')
            
            onscreenDisplay.set('regions', 'image; circle ({0} {1} {2}) '.format(self.referenceCatalogue.circle.transformedCenterX,self.referenceCatalogue.circle.transformedCenterY,self.referenceCatalogue.circle.transformedRadius) + '# color=magenta font=\"helvetica 10 normal\" select=1 highlite=1 edit=1 move=1 delete=1 include=1 fixed=0 source text = {radius acceptance}')
        except:
            logging.error( "Catalogue.ds9DisplayCatalogue: No contact to ds9: writing header")
            
        for (sxObjectNumber,identifier), value in sorted( self.catalogue.iteritems()):
            try:
                onscreenDisplay.set('regions', 'image; circle ({0} {1} {2}) # font=\"helvetica 10 normal\" color={{{3}}} select=1 highlite=1 edit=1 move=1 delete=1 include=1 fixed=0 source text = {{{4}}}'.format( self.catalogue[(sxObjectNumber, 'X_IMAGE')], self.catalogue[(sxObjectNumber, 'Y_IMAGE')], self.catalogue[(sxObjectNumber, 'FWHM_IMAGE')]/2., color, sxObjectNumber))
            except:
                logging.error( "Catalogue.ds9DisplayCatalogue: No contact to ds9, object: {0}".format(sxObjectNumber))
                

    def ds9WriteRegionFile(self, writeSelected=False, writeMatched=False, writeAll=False, colorSelected="cyan", colorMatched="green", colorAll="red"):
        with open( self.ds9RegionFileName, 'w') as ds9RegionFile:
            ds9RegionFile.write("# Region file format: DS9 version 4.0\n")
            ds9RegionFile.write("# Filename: {0}\n".format(self.fitsHDU.fitsFileName))

            ds9RegionFile.write("image\n")
            ds9RegionFile.write("text (" + str(80) + "," + str(10) + ") # color=magenta font=\"helvetica 15 normal\" text = {FOC_POS="+ str(self.fitsHDU.headerElements['FOC_POS']) + "}\n")
            # ToDo: Solve that
            if(runTimeConfig.value('DS9_DISPLAY_ACCEPTANCE_AREA')):
                ds9RegionFile.write("point ({0},{1}) ".format( self.referenceCatalogue.circle.transformedCenterX,self.referenceCatalogue.circle.transformedCenterY) + "# color=magenta text = {center acceptance}\n")
                ds9RegionFile.write("circle ({0},{1},{2}) ".format(self.referenceCatalogue.circle.transformedCenterX,self.referenceCatalogue.circle.transformedCenterY, self.referenceCatalogue.circle.transformedRadius) + "# color=magenta text = {radius acceptance}\n")

            ds9RegionFile.write("global color={0} font=\"helvetica 10 normal\" select=1 highlite=1 edit=1 move=1 delete=1 include=1 fixed=0 source\n".format(colorSelected))

            # objects selected (found in all fits images)
            if(writeSelected):
                for sxObjectNumber, sxObject in self.sxObjects.items():
                    sxReferenceObject=  self.referenceCatalogue.sxObjectByNumber(sxObject.associatedSXobject)
                    if( sxReferenceObject.foundInAll):
                        radius= self.catalogue[(sxObjectNumber, 'FWHM_IMAGE')]/ 2. # not diameter
                        # ToDo: find out escape
                        ds9RegionFile.write("circle ({0},{1},{2})".format( self.catalogue[(sxObjectNumber, 'X_IMAGE')], self.catalogue[(sxObjectNumber, 'Y_IMAGE')], radius) + "# text = {R:" + sxReferenceObject.objectNumber + ",O: " + sxObjectNumber  + ",F:" + str(self.catalogue[(sxObjectNumber, 'FWHM_IMAGE')]) + " }\n")

                        ds9RegionFile.write("circle ({0},{1},{2})".format( self.catalogue[(sxObjectNumber, 'X_IMAGE')], self.catalogue[(sxObjectNumber, 'Y_IMAGE')], radius) + "\n")

                        # cricle to enhance visibility
                        ds9RegionFile.write("circle ({0},{1},{2})".format( self.catalogue[(sxObjectNumber, 'X_IMAGE')], self.catalogue[(sxObjectNumber, 'Y_IMAGE')], 40)  + '#  color= yellow\n') 
            # all objects which matched against the reference catalogue, but not found on all fits images
            if(writeMatched):
                ds9RegionFile.write("global color={0} font=\"helvetica 10 normal\" select=1 highlite=1 edit=1 move=1 delete=1 include=1 fixed=0 source\n".format(colorMatched))
                ds9RegionFile.write("image\n")
                for sxObjectNumber, sxObject in self.sxObjects.items():
                    sxReferenceObject=  self.referenceCatalogue.sxObjectByNumber(sxObject.associatedSXobject)
                    if( sxObject.matched):
                        if( writeSelected and sxReferenceObject.foundInAll):
                            continue
                        radius= self.catalogue[(sxObjectNumber, 'FWHM_IMAGE')]/ 2. # not diameter
                        # ToDo: find out escape
                        ds9RegionFile.write("circle ({0},{1},{2})".format( self.catalogue[(sxObjectNumber, 'X_IMAGE')], self.catalogue[(sxObjectNumber, 'Y_IMAGE')], radius) + "# text = {R:" + sxReferenceObject.objectNumber + ",O:" + sxObjectNumber  +  ",F:" + str(self.catalogue[(sxObjectNumber, 'FWHM_IMAGE')]) + " }\n")

            # all objects found by sextractor but not found in all fits images
            # but not falling in the selected or matched category
            if(writeAll):
                ds9RegionFile.write("global color={0} font=\"helvetica 10 normal\" select=1 highlite=1 edit=1 move=1 delete=1 include=1 fixed=0 source\n".format(colorAll))
                ds9RegionFile.write("image\n")

                for sxObjectNumber, sxObject in self.sxObjects.items():
                    sxReferenceObject=  self.referenceCatalogue.sxObjectByNumber(sxObject.associatedSXobject)
                    radius= self.catalogue[(sxObjectNumber, 'FWHM_IMAGE')]/ 2. # not diameter
                    if( not (sxReferenceObject.foundInAll or sxObject.matched)):
                        ds9RegionFile.write("circle ({0},{1},{2})".format( self.catalogue[(sxObjectNumber, 'X_IMAGE')], self.catalogue[(sxObjectNumber, 'Y_IMAGE')], radius) + "# text = {" + " O:"+ sxObjectNumber +  ",F:" + str(self.catalogue[(sxObjectNumber, 'FWHM_IMAGE')]) + "}\n") 

        ds9RegionFile.close()
        
    def removeSXObject(self, sxObjectNumber):
        if( not sxObjectNumber in self.sxObjects):
            logging.error( "Catalogue.removeSXObject: reference Object number " + sxObjectNumber + " for >" + identifier + "< not found in sxObjects")
            return False
        else:
            del self.sxObjects[sxObjectNumber]

        # remove its properties
        for identifier in self.SExtractorParams.reference:
            if(((sxObjectNumber, identifier)) in self.catalogue):
                del self.catalogue[(sxObjectNumber, identifier)]
            else:
                logging.error( "Catalogue.removeSXObject: object number " + sxObjectNumber + " for >" + identifier + "< not found, break")
                break
        else:
#                logging.error( "Catalogue.removeSXObject: for object number " + sxObjectNumber + " all elements deleted")
            return True
        return False

    # ToDo, define if that shoud go into SXObject (now, better not)
    def checkProperties(self, sxObjectNumber): 
        if( self.catalogue[( sxObjectNumber, 'FLAGS')] != 0): # ToDo, ATTENTION
            #print "checkProperties flags %d" % self.catalogue[( sxObjectNumber, 'FLAGS')]
            return False
        if( self.catalogue[( sxObjectNumber, 'ELLIPTICITY')] > runTimeConfig.value('ELLIPTICITY')): # ToDo, ATTENTION
            #print "checkProperties elli %f %f" %  (self.catalogue[( sxObjectNumber, 'ELLIPTICITY')],runTimeConfig.value('ELLIPTICITY'))
            return False
        # TRUE    
        return True

    def cleanUp(self):
        logging.debug( 'Catalogue.cleanUp: catalogue, I am : ' + self.fitsHDU.fitsFileName)

        discardedObjects= 0
        sxObjectNumbers= self.sxObjects.keys()
        for sxObjectNumber in sxObjectNumbers:
            if( not self.checkProperties(sxObjectNumber)):
#                self.removeSXObject( sxObjectNumber)
                discardedObjects += 1

        logging.info("Catalogue.cleanUp: Number of objects discarded %d " % discardedObjects) 

    def matching(self):
        matched= 0
        #verbose= True
        for sxObjectNumber, sxObject in self.sxObjects.items():
                sxReferenceObject= self.referenceCatalogue.sxObjectByNumber(sxObject.associatedSXobject)
                if( sxReferenceObject != None):
                    if( self.multiplicity[sxReferenceObject.objectNumber] == 1): 
                        matched += 1
                        sxObject.matched= True 
                        sxReferenceObject.numberOfMatches += 1
                        sxReferenceObject.matchedsxObjects.append(sxObject)
                    else:
                        if( verbose):
                            print "lost " + sxReferenceObject.objectNumber + " %d" % self.multiplicity[sxReferenceObject.objectNumber] # count

        if(verbose):
            print "number of sxObjects =%d" % len(self.sxObjects)
            print "number of matched sxObjects %d=" % matched + "of %d "% len(self.referenceCatalogue.sxObjects) + " required %f " % ( runTimeConfig.value('MATCHED_RATIO') * len(self.referenceCatalogue.sxObjects))

        if( self.referenceCatalogue.numberReferenceObjects() > 0):
            if( (float(matched)/float(self.referenceCatalogue.numberReferenceObjects())) > runTimeConfig.value('MATCHED_RATIO')):
                return True
            else:
                logging.error("matching: too few sxObjects matched %d" % matched + " of %d" % len(self.referenceCatalogue.sxObjects) + " required are %f" % ( runTimeConfig.value('MATCHED_RATIO') * len(self.referenceCatalogue.sxObjects)) + " sxobjects at FOC_POS %d= " % self.fitsHDU.headerElements['FOC_POS'] + "file "+ self.fitsHDU.fitsFileName)
                return False
        else:
            logging.error('matching: should not happen here, number reference objects is {0} should be greater than 0'.format( self.referenceCatalogue.numberReferenceObjects()))
            return False
                    

# example how to access the catalogue
    def average(self, variable):
        sum= 0
        i= 0
        for (sxObjectNumber,identifier), value in self.catalogue.iteritems():
            if( identifier == variable):
                sum += float(value)
                i += 1

        #if(verbose):
        if( i != 0):
            print 'average at FOC_POS: ' + str(self.fitsHDU.headerElements['FOC_POS']) + ' '+ variable + ' %f ' % (sum/ float(i)) 
            return (sum/ float(i))
        else:
            print 'Error in average i=0'
            return False
    def averageFWHM(self, selection="all"):
        sum= 0
        i= 0
        
        for sxObjectNumber, sxObject in self.sxObjects.items():
            sxReferenceObject=  self.referenceCatalogue.sxObjectByNumber(sxObject.associatedSXobject)
            if re.search("selected", selection):
                if( sxReferenceObject.foundInAll):
                    sum += sxObject.fwhm
                    i += 1

            elif re.search("matched", selection):
                if( sxObject.matched):
                    sum += sxObject.fwhm
                    i += 1
            else:
                sum += sxObject.fwhm
                i += 1
                    
        #if(verbose):
        if( i != 0):
            print 'average %8s' %(selection) + ' at FOC_POS: ' + str(self.fitsHDU.headerElements['FOC_POS']) + ' FWHM  %5.2f ' % (sum/ float(i))  + ' number of objects %5d' % (i)
            return (sum/ float(i))
        else:
            print 'Error in average i=0'
            return False

class ReferenceCatalogue(Catalogue):
    """Class for a catalogue (SExtractor result)"""
    def __init__(self, fitsHDU=None, SExtractorParams=None):
        self.fitsHDU  = fitsHDU
        self.catalogueFileName= serviceFileOp.expandToCat(self.fitsHDU)
        self.lines= []
        self.catalogue = {}
        self.sxObjects = {}
        self.isReference = False
        self.isValid     = False
        self.objectSeparation2= runTimeConfig.value('OBJECT_SEPARATION')**2
        self.SExtractorParams = SExtractorParams
        self.indexeFlag       = self.SExtractorParams.reference.index('FLAGS')  
        self.indexellipticity = self.SExtractorParams.reference.index('ELLIPTICITY')
        self.skyList= serviceFileOp.expandToSkyList(self.fitsHDU)
        self.circle= AcceptanceRegion( self.fitsHDU) 

        ReferenceCatalogue.__lt__ = lambda self, other: self.fitsHDU.headerElements['FOC_POS'] < other.fitsHDU.headerElements['FOC_POS']

    def sxObjectByNumber(self, sxObjectNumber):
        if(sxObjectNumber in self.sxObjects.keys()):
            return self.sxObjects[sxObjectNumber]
        return None

    def writeCatalogue(self):
        logging.info( 'ReferenceCatalogue.writeCatalogue: writing reference catalogue, for ' + self.fitsHDU.fitsFileName) 

        pElement = re.compile( r'#[ ]+([0-9]+)[ ]+([\w]+)')
        pData    = re.compile( r'^[ \t]+([0-9]+)[ \t]+')
        SXcat= open( self.skyList, 'w')

        for line in self.lines:
            element= pElement.search(line)
            data   = pData.search(line)
            if(element):
                SXcat.write(line)
            else:
                if((data.group(1)) in self.sxObjects):
                    try:
                        SXcat.write(line)
                    except:
                        logging.error( 'ReferenceCatalogue.writeCatalogue: could not write line for object ' + data.group(1))
                        sys.exit(1)
                        break
        SXcat.close()

    def runSExtractor(self):
        if( verbose):
            print 'sextractor  ' + runTimeConfig.value('SEXPRG')
            print 'sextractor  ' + runTimeConfig.value('SEXCFG')
            print 'sextractor  ' + runTimeConfig.value('SEXREFERENCE_PARAM')

        # 2>/dev/null swallowed by PIPE
        (prg, arg)= runTimeConfig.value('SEXPRG').split(' ')

        self.isReference = True 
        cmd= [  prg,
                self.fitsHDU.fitsFileName, 
                '-c ',
                runTimeConfig.value('SEXCFG'),
                '-CATALOG_NAME',
                self.skyList,
                '-PARAMETERS_NAME',
                runTimeConfig.value('SEXREFERENCE_PARAM'),]
        try:
            output = subprocess.Popen( cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]

        except OSError as (errno, strerror):
            logging.error( 'ReferenceCatalogue.runSExtractor: I/O error({0}): {1}'.format(errno, strerror))
            sys.exit(1)

        except:
            logging.error('ReferenceCatalogue.runSExtractor: '+ repr(cmd) + ' died')
            sys.exit(1)

# checking against reference items see http://www.wellho.net/resources/ex.php4?item=y115/re1.py
    def createCatalogue(self):
        if( self.SExtractorParams==None):
            logging.error( 'ReferenceCatalogue.createCatalogue: no SExtractor parameter configuration given')
            return False

        # if( not self.fitsHDU.isReference):
        #SXcat= open( self.catalogueFileName, 'r')
        #else:
        try:
            SXcat= open( self.skyList, 'r')
        except:
            logging.error("could not open file " + self.skyList + ", exiting")
            sys.exit(1) 

        self.lines= SXcat.readlines()
        SXcat.close()

        # rely on the fact that first line is stored as first element in list
        # match, element, data
        ##   2 X_IMAGE                Object position along x                                    [pixel]
        #         1   2976.000      7.473     2773.781     219.8691     42.19405  -5.8554   0.2084     120.5605         6  26     4.05      5.330    0.823
 
        pElement = re.compile( r'#[ ]+([0-9]+)[ ]+([\w]+)')
        pData    = re.compile( r'')
        sxObjectNumber= -1
        itemNrX_IMAGE     = self.SExtractorParams.reference.index('X_IMAGE')
        itemNrY_IMAGE     = self.SExtractorParams.reference.index('Y_IMAGE')
        itemNrFWHM_IMAGE  = self.SExtractorParams.reference.index('FWHM_IMAGE')
        itemNrFLUX_MAX    = self.SExtractorParams.reference.index('FLUX_MAX')

        for (lineNumber, line) in enumerate(self.lines):
            element = pElement.match(line)
            data    = pData.match(line)
            x= -1
            y= -1
            sxObjectNumberASSOC= '-1'
            if( element):
                pass
            elif( data):
                items= line.split()
                sxObjectNumber= items[0] # NUMBER
                
                for (j, item) in enumerate(items):
                    self.catalogue[(sxObjectNumber, self.SExtractorParams.reference[j])]=  float(item)


                self.sxObjects[sxObjectNumber]= SXReferenceObject(sxObjectNumber, self.fitsHDU.headerElements['FOC_POS'], (float(items[itemNrX_IMAGE]), float(items[itemNrY_IMAGE])), float(items[itemNrFWHM_IMAGE]), float(items[itemNrFLUX_MAX])) # position, bool is used for clean up (default True, True)
            else:
                logging.error( 'ReferenceCatalogue.readCatalogue: no match on line %d' % lineNumber)
                logging.error( 'ReferenceCatalogue.readCatalogue: ' + line)
                break

        else: # exhausted 
            if( self.numberReferenceObjects() > 0):
                logging.info( 'ReferenceCatalogue.readCatalogue: catalogue created {0} number of objects {1}'.format( self.fitsHDU.fitsFileName, self.numberReferenceObjects()))
                self.isValid= True
            else:
                logging.error( 'ReferenceCatalogue.readCatalogue: catalogue created {0} no objects found'.format( self.fitsHDU.fitsFileName))
                self.isValid= False

        return self.isValid

    def numberReferenceObjects(self):
        return len(self.sxObjects)

    def printObject(self, sxObjectNumber):
            
        for identifier in self.SExtractorParams.reference():
            
            if(((sxObjectNumber, identifier)) in self.catalogue):
                print "printObject: reference object number " + sxObjectNumber + " identifier " + identifier + "value %f" % self.catalogue[(sxObjectNumber, identifier)]
            else:
                logging.error( "ReferenceCatalogue.printObject: reference Object number " + sxObjectNumber + " for >" + identifier + "< not found, break")
                break
        else:
#                logging.error( "ReferenceCatalogue.printObject: for object number " + sxObjectNumber + " all elements printed")
            return True

        return False

    def removeSXObject(self, sxObjectNumber):
        if( not sxObjectNumber in self.sxObjects):
            logging.error( "ReferenceCatalogue.removeSXObject: reference Object number " + sxObjectNumber + " for >" + identifier + "< not found in sxObjects")
        else:
            if( sxObjectNumber in self.sxObjects):
                del self.sxObjects[sxObjectNumber]
            else:
                logging.error( "ReferenceCatalogue.removeSXObject: object number " + sxObjectNumber + " not found")

        for identifier in self.SExtractorParams.reference:
            if(((sxObjectNumber, identifier)) in self.catalogue):
                del self.catalogue[(sxObjectNumber, identifier)]
            else:
                logging.error( "ReferenceCatalogue.removeSXObject: reference Object number " + sxObjectNumber + " for >" + identifier + "< not found, break")
                break
        else:
#                logging.error( "ReferenceCatalogue.removeSXObject: for object number " + sxObjectNumber + " all elements deleted")
            return True

        return False

    def checkProperties(self, sxObjectNumber): 
        if( self.catalogue[( sxObjectNumber, 'FLAGS')] != 0): # ToDo, ATTENTION
            return False
        elif( self.catalogue[( sxObjectNumber, 'ELLIPTICITY')] > runTimeConfig.value('ELLIPTICITY_REFERENCE')): # ToDo, ATTENTION
            return False
        # TRUE    
        return True

    def checkSeparation( self, position1, position2):
        dx= abs(position2[0]- position1[0])
        dy= abs(position2[1]- position1[1])
        if(( dx < runTimeConfig.value('OBJECT_SEPARATION')) and ( dy< runTimeConfig.value('OBJECT_SEPARATION'))):
            if( (dx**2 + dy**2) < self.objectSeparation2):
                return False
        # TRUE    
        return True

    def checkAcceptance(self, sxObject=None, circle=None):
        distance= sqrt(( float(sxObject.position[0])- circle.transformedCenterX)**2 +(float(sxObject.position[1])- circle.transformedCenterX)**2)
 
        if( circle.transformedRadius >= 0):
            if( distance < abs( circle.transformedRadius)):
                return True
            else:
                return False
        else:
            if( distance < abs( circle.transformedRadius)):
                return False
            else:
                return True

    def cleanUpReference(self):
        if( not  self.isReference):
            logging.debug( 'ReferenceCatalogue.cleanUpReference: clean up only for a reference catalogue, I am : ' + self.fitsHDU.fitsFileName) 
            return False
        else:
            logging.debug( 'ReferenceCatalogue.cleanUpReference: reference catalogue, I am : ' + self.fitsHDU.fitsFileName)
        flaggedSeparation= 0
        flaggedProperties= 0
        flaggedAcceptance= 0
        for sxObjectNumber1, sxObject1 in self.sxObjects.iteritems():
            for sxObjectNumber2, sxObject2 in self.sxObjects.iteritems():
                if( sxObjectNumber1 != sxObjectNumber2):
                    if( not self.checkSeparation( sxObject1.position, sxObject2.position)):
                        sxObject1.separationOK=False
                        sxObject2.separationOK=False
                        flaggedSeparation += 1

            else:
                if( not self.checkProperties(sxObjectNumber1)):
                    sxObject1.propertiesOK=False
                    flaggedProperties += 1
                if(not self.checkAcceptance(sxObject1, self.circle)):
                    sxObject1.acceptanceOK=False
                    flaggedAcceptance += 1

        discardedObjects= 0
        sxObjectNumbers= self.sxObjects.keys()
        for sxObjectNumber in sxObjectNumbers:
            if(( not self.sxObjects[sxObjectNumber].separationOK) or ( not self.sxObjects[sxObjectNumber].propertiesOK) or ( not self.sxObjects[sxObjectNumber].acceptanceOK)):
                self.removeSXObject( sxObjectNumber)
                discardedObjects += 1

        logging.info("ReferenceCatalogue.cleanUpReference: Number of objects discarded %d  (%d, %d, %d)" % (discardedObjects, flaggedSeparation, flaggedProperties, flaggedAcceptance)) 



class AcceptanceRegion():
    """Class holding the properties of the acceptance circle, units are (binned) pixel"""
    def __init__(self, fitsHDU=None, centerOffsetX=None, centerOffsetY=None, radius=None):
        self.fitsHDU= fitsHDU
        self.naxis1= 0
        self.naxis2= 0

        try:
            # if binning is used these values represent the rebinned pixels, e.g. 1x1 4096 and 2x2 2048
            self.naxis1 = float(fitsHDU.headerElements['NAXIS1'])
            self.naxis2 = float(fitsHDU.headerElements['NAXIS2'])
        except:
            logging.error("AcceptanceRegion.__init__: something went wrong here")

        if( centerOffsetX==None):
            self.centerOffsetX= float(runTimeConfig.value('CENTER_OFFSET_X'))
        if( centerOffsetY==None):
            self.centerOffsetY= float(runTimeConfig.value('CENTER_OFFSET_Y'))
        if( radius==None):
            self.radius = float(runTimeConfig.value('RADIUS'))
        l_x= 0. # window (ev.)
        l_y= 0.
        self.transformedCenterX= (self.naxis1- l_x)/2 + self.centerOffsetX 
        self.transformedCenterY= (self.naxis2- l_y)/2 + self.centerOffsetY
        self.transformedRadius= self.radius
        if( verbose):
            print "AcceptanceRegion %f %f %f %f %f  %f %f %f" % (self.naxis1, self.naxis1, self.centerOffsetX, self.centerOffsetY, self.radius, self.transformedCenterX, self.transformedCenterY, self.transformedRadius)


import numpy
from collections import defaultdict
class Catalogues():
    """Class holding Catalogues"""
    def __init__(self,referenceCatalogue=None):
        self.CataloguesList= []
        self.isValid= False
        self.averageFwhm= {}
        self.averageFlux={}
        self.referenceCatalogue= referenceCatalogue
        if( self.referenceCatalogue != None):
            self.dataFileNameFwhm= serviceFileOp.expandToFitInput( self.referenceCatalogue.fitsHDU, 'FWHM_IMAGE')
            self.dataFileNameFlux= serviceFileOp.expandToFitInput( self.referenceCatalogue.fitsHDU, 'FLUX_MAX')
            self.imageFilename= serviceFileOp.expandToFitImage( self.referenceCatalogue.fitsHDU)
            self.ds9CommandFileName= serviceFileOp.expandToDs9CommandFileName(self.referenceCatalogue.fitsHDU)
        self.maxFwhm= 0.
        self.minFwhm= 0.
        self.maxFlux= 0.
        self.minFlux= 0.
        self.numberOfObjectsFoundInAllFiles= 0.
        self.ds9Command=""
        self.averagesCalculated=False
        
    def validate(self):
        if( len(self.CataloguesList) > 0):
            # default is False
            for cat in self.CataloguesList:
                if( cat.isValid== True):
                    if( verbose):
                        print 'Catalogues.validate: valid cat for: ' + cat.fitsHDU.fitsFileName
                    continue
                else:
                    if( verbose):
                        print 'Catalogues.validate: removed cat for: ' + cat.fitsHDU.fitsFileName

                    try:
                        del self.CataloguesList[cat]
                    except:
                        logging.error('Catalogues.validate: could not remove cat for ' + cat.fitsHDU.fitsFileName)
                        if(verbose):
                            print "Catalogues.validate: failed to removed cat: " + cat.fitsHDU.fitsFileName
                    break
            else:
                # still catalogues here
                if( len(self.CataloguesList) > 0):
                    self.isValid= True
                    return self.isValid
                else:
                    logging.error('Catalogues.validate: no catalogues found after validation')
        else:
            logging.error('Catalogues.validate: no catalogues found')
        return False

    def countObjectsFoundInAllFiles(self):
        self.numberOfObjectsFoundInAllFiles= 0
        for sxReferenceObjectNumber, sxReferenceObject in self.referenceCatalogue.sxObjects.items():
            if(sxReferenceObject.numberOfMatches== len(self.CataloguesList)):
                self.numberOfObjectsFoundInAllFiles += 1

        if( self.numberOfObjectsFoundInAllFiles < runTimeConfig.value('MINIMUM_OBJECTS')):
            logging.error('Catalogues.writeFitInputValues: too few sxObjects %d < %d' % (self.numberOfObjectsFoundInAllFiles, runTimeConfig.value('MINIMUM_OBJECTS')))
            return False
        else:
            return True

    def writeFitInputValues(self):
        
        fitInput= open( self.dataFileNameFwhm, 'w')
        for focPos in sorted(self.averageFwhm):
            line= "%04d %f\n" % ( focPos, self.averageFwhm[focPos])
            fitInput.write(line)

        fitInput.close()
        fitInput= open( self.dataFileNameFlux, 'w')
        for focPos in sorted(self.averageFlux):
            line= "%04d %f\n" % ( focPos, self.averageFlux[focPos]/self.maxFlux * self.maxFwhm)
            fitInput.write(line)

        fitInput.close()

    def fitTheValues(self, configFileName=None):
# ToDo: think about a better solution for  configFileName

        if( self.countObjectsFoundInAllFiles()):
            self.__average__()
            self.writeFitInputValues()
# ROOT, "$fitprg $fltr $date $number_of_objects_found_in_all_files $fwhm_file $flux_file $tmp_fit_result_file
            if(runTimeConfig.value('DISPLAYFIT')):
                display= '1'
            else:
                display= '0'
            cmd= [ runTimeConfig.value('FOCROOT'),
                   display, 
                   self.referenceCatalogue.fitsHDU.headerElements['FILTER'],
                   serviceFileOp.now,
                   str(self.numberOfObjectsFoundInAllFiles),
                   self.dataFileNameFwhm,
                   self.dataFileNameFlux,
                   self.imageFilename]
        
            output = subprocess.Popen( cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()

            if( configFileName==None):
                # offline mode
                print 'FOCUS: {0}'.format(output[0].split()[1])
            else:
                # acquire mode
                fitResultFileName= serviceFileOp.reconstructFitResultPath(configFileName) 
                if(fitResultFileName==None):
                    # if something really wen wrong
                    print 'FOCUS: {0}'.format(output[0].split()[1])
                else:
                    # ToDo: discuss with Petr
                    print 'FOCUS: {0}'.format(output[0].split()[1])
                    with open( fitResultFileName, 'a') as frfn:
                        for item in output:
                            frfn.write(item)

                    frfn.close()

        else:
            logging.error('Catalogues.fitTheValues: too few objects, do not fit')

    def __average__(self):
        numberOfObjects = 0
        if( self.averagesCalculated):
            return
        else:
            self.averagesCalculated=True
            
        fwhm= defaultdict(list)
        flux= defaultdict(list)
        for sxReferenceObjectNumber, sxReferenceObject in self.referenceCatalogue.sxObjects.items():
            if(sxReferenceObject.numberOfMatches== len(self.CataloguesList)):
                numberOfObjects += 1
                for sxObject in sxReferenceObject.matchedsxObjects:
                    fwhm[sxObject.focusPosition].append(sxObject.fwhm)
                    flux[sxObject.focusPosition].append(sxObject.flux)
                    sxReferenceObject.foundInAll= True


        fwhmList=[]
        fluxList=[]
        for focPos in fwhm:
            self.averageFwhm[focPos] = numpy.mean(fwhm[focPos], axis=0)
            fwhmList.append(self.averageFwhm[focPos])
            
        for focPos in flux:
            self.averageFlux[focPos] = numpy.mean(flux[focPos], axis=0)
            fluxList.append(self.averageFlux[focPos])

        try:
            self.maxFwhm= numpy.amax(fwhmList)
        except:
            logging.error('__average__: something wrong with fwhmList maximum')
            return False
        try:
            self.minFwhm= numpy.amax(fwhmList)
        except:
            logging.error('__average__: something wrong with fwhmList minimum')
            return False
        try:
            self.maxFlux= numpy.amax(fluxList)
        except:
            logging.error('__average__: something wrong with fluxList maximum')
            return False
        try:
            self.minFlux= numpy.amax(fluxList)
        except:
            logging.error('__average__: something wrong with fluxList minimum')
            return False

        if(verbose):
            print "numberOfObjects %d " % (numberOfObjects)

        return True

    def ds9DisplayCatalogues(self):
# deprecated        self.__average__()

        for cat in sorted(self.CataloguesList, key=lambda cat: cat.fitsHDU.headerElements['FOC_POS']):
            cat.ds9DisplayCatalogue("cyan")
            # if reference catr.ds9DisplayCatalogue("yellow", False)

    def ds9WriteRegionFiles(self):
# deprecated         self.__average__()
        
        self.ds9Command= "ds9 -zoom to fit -scale mode zscale\\\n" 

        for cat in sorted(self.CataloguesList, key=lambda cat: cat.fitsHDU.headerElements['FOC_POS']):

            if( cat.catalogueFileName == cat.referenceCatalogue.catalogueFileName):
                cat.ds9WriteRegionFile(True, True, False, "yellow")
            else:
                cat.ds9WriteRegionFile(True, runTimeConfig.value('DS9_MATCHED'), runTimeConfig.value('DS9_ALL'), "cyan")

#            self.ds9CommandAdd(" " + cat.fitsHDU.fitsFileName + " -region " +  serviceFileOp.expandToDs9RegionFileName(cat.fitsHDU) + " -region " + serviceFileOp.expandToDs9RegionFileName(cat.referenceCatalogue.fitsHDU) + "\\\n")
            self.ds9CommandAdd(" " + cat.fitsHDU.fitsFileName + " -region " +  serviceFileOp.expandToDs9RegionFileName(cat.fitsHDU) + "\\\n")

        self.ds9CommandFileWrite()

        
    def ds9CommandFileWrite(self):
        self.ds9CommandAdd(" -single -blink\n")

        with open( self.ds9CommandFileName, 'w') as ds9CommandFile:
            ds9CommandFile.write("#!/bin/bash\n") 
            ds9CommandFile.write(self.ds9Command) 

        ds9CommandFile.close()
        serviceFileOp.setModeExecutable(self.ds9CommandFileName)
        
    def ds9CommandAdd(self, ds9CommandPart):
        self.ds9Command= self.ds9Command + ds9CommandPart

    def printSelectedSXobjects(self):
        for sxReferenceObjectNumber, sxReferenceObject in self.referenceCatalogue.sxObjects.items():
            if( sxReferenceObject.foundInAll): 
                print "======== %d %d %f %f" %  (int(sxReferenceObjectNumber), sxReferenceObject.focusPosition, sxReferenceObject.position[0],sxReferenceObject.position[1])


import sys
import pyfits

class FitsHDU():
    """Class holding fits file name and its properties"""
    def __init__(self, fitsFileName=None, referenceFitsHDU=None):
        if(referenceFitsHDU==None):
            self.fitsFileName= serviceFileOp.expandToRunTimePath(fitsFileName)
        else:
            self.fitsFileName= fitsFileName

        self.referenceFitsHDU= referenceFitsHDU
        self.isValid= False
        self.headerElements={}
        #ToDo: generalize
        self.binning=-1
        self.assumedBinning=False

        FitsHDU.__lt__ = lambda self, other: self.headerElements['FOC_POS'] < other.headerElements['FOC_POS']

    def headerProperties(self):

        if( verbose):
            print "fits file path: " + self.fitsFileName
        try:
            fitsHDU = pyfits.fitsopen(self.fitsFileName)
        except:
            if( self.fitsFileName):
                logging.error('FitsHDU: file not found : ' + self.fitsFileName)
            else:
                logging.error('FitsHDU: file name not given (None)')
            return False

        fitsHDU.close()

        try:
            self.headerElements['FILTER']= fitsHDU[0].header['FILTER']
        except:
            self.isValid= True
            self.headerElements['FILTER']= 'NOFILTER' 
            logging.info('headerProperties: fits file ' + self.fitsFileName + ' the filter header element not found, assuming filter NOFILTER')

        try:
            self.headerElements['BINNING']= fitsHDU[0].header['BINNING']
        except:
            self.isValid= True
            self.assumedBinning=True
            self.headerElements['BINNING']= '1x1'
            logging.info('headerProperties: fits file ' + self.fitsFileName + ' the binning header element not found, assuming 1x1 binning')

        try:
            self.headerElements['ORIRA']  = fitsHDU[0].header['ORIRA']
            self.headerElements['ORIDEC'] = fitsHDU[0].header['ORIDEC']
        except:
            self.isValid= True
            logging.info('headerProperties: fits file ' + self.fitsFileName + ' the coordinates header elements: ORIRA, ORIDEC not found ')

        try:
            self.headerElements['FOC_POS']= fitsHDU[0].header['FOC_POS']
            self.headerElements['NAXIS1'] = fitsHDU[0].header['NAXIS1']
            self.headerElements['NAXIS2'] = fitsHDU[0].header['NAXIS2']
        except:
            self.isValid= False
            logging.error('headerProperties: fits file ' + self.fitsFileName + ' the required header elements not found')
            return False
        
        if( not self.extractBinning()):
            self.isValid= False
            return False
        # match the header elements with reference HDU
        if( self.referenceFitsHDU != None):
            if( self.matchHeaderElements(fitsHDU)):
                self.isValid= True
                return True
            else:
                self.isValid= False
                return False
        else:
            self.isValid= True
            return True
            
    def matchHeaderElements(self, fitsHDU):
        keys= self.referenceFitsHDU.headerElements.keys()
        i= 0
        for key in keys:
            if(key == 'FOC_POS'):
                self.headerElements['FOC_POS']= fitsHDU[0].header['FOC_POS']
                continue
            if( self.assumedBinning):
                if(key == 'BINNING'):
                    continue
            if(self.referenceFitsHDU.headerElements[key]!= fitsHDU[0].header[key]):
##wildi                logging.error("headerProperties: fits file " + self.fitsFileName + " property " + key + " " + self.referenceFitsHDU.headerElements[key] + " " + fitsHDU[0].header[key])
                break
            else:
                self.headerElements[key] = fitsHDU[0].header[key]

        else: # exhausted
            self.isValid= True
            if( verbose):
                print 'headerProperties: header ok : ' + self.fitsFileName

            return True
        
        logging.error("headerProperties: fits file " + self.fitsFileName + " has different header properties")
        return False

    def extractBinning(self):
        #ToDo: clumsy
        pbinning1x1 = re.compile( r'1x1')
        binning1x1 = pbinning1x1.match(self.headerElements['BINNING'])
        pbinning2x2 = re.compile( r'2x2')
        binning2x2 = pbinning2x2.match(self.headerElements['BINNING'])

        if( binning1x1):
            self.binning= 1
        elif(binning2x2):
            self.binning= 2
        else:
            logging.error('headerProperties: fits file ' + self.fitsFileName +  ' binning: {0} not supported, exiting'.format(fitsHDU[0].header['BINNING']))
            return False
        return True
    
class FitsHDUs():
    """Class holding FitsHDU"""
    def __init__(self, referenceHDU=None):
        self.fitsHDUsList= []
        self.isValid= False
        self.numberOfFocusPositions= 0 
        if( referenceHDU !=None):
            self.referenceHDU= referenceHDU
        else:
            self.referenceHDU=None
            
    def findHDUByFitsFileName( self, fileName):
        for hdu in sorted(self.fitsHDUsList):
            if( fileName== hdu.fitsFileName):
                return hdu
        return False

    def findReference(self):
        if( self.referenceHDU !=None):
            return self.referenceHDU
        else:
            return False
    def findReferenceByFocPos(self, name):
        filter=  runTimeConfig.filterByName(name)
#        for hdu in sorted(self.fitsHDUsList):
        for hdu in sorted(self.fitsHDUsList):
            if( filter.OffsetToClearPath < hdu.headerElements['FOC_POS']):
                if(verbose):
                    print "FOUND reference by foc pos ==============" + hdu.fitsFileName + "======== %d" % hdu.headerElements['FOC_POS']
                return hdu
        return False

    def validate(self, name=None):
        hdur= self.findReference()
        if( hdur== False):
            hdur= self.findReferenceByFocPos(name)
            if( hdur== False):
                if( verbose):
                    print "Nothing found in findReference and findReferenceByFocPos"
                return self.isValid

        keys= hdur.headerElements.keys()
        i= 0
        differentFocuserPositions={}
        for hdu in self.fitsHDUsList:
            for key in keys:
                if(key == 'FOC_POS'):
                    differentFocuserPositions[str(hdu.headerElements['FOC_POS'])]= 1 # means nothing
                    continue
                if( hdur.headerElements[key]!= hdu.headerElements[key]):
                    try:
                        del self.fitsHDUsList[hdu]
                    except:
                        logging.error('FitsHDUs.validate: could not remove hdu for ' + hdu.fitsFileName)
                        if(verbose):
                            print "FitsHDUs.validate: removed hdu: " + hdu.fitsFileName + "========== %d" %  self.fitsHDUsList.index(hdu)
                    break # really break out the keys loop 
            else: # exhausted
                hdu.isValid= True
                if( verbose):
                    print 'FitsHDUs.validate: valid hdu: ' + hdu.fitsFileName

        self.numberOfFocusPositions= len(differentFocuserPositions.keys())

        if( self.numberOfFocusPositions >= runTimeConfig.value('MINIMUM_FOCUSER_POSITIONS')):
            if( len(self.fitsHDUsList) > 0):
                logging.info('FitsHDUs.validate: hdus are valid')
                self.isValid= True
        else:
            logging.error('FitsHDUs.validate: hdus are invalid due too few focuser positions %d required are %d' % (self.numberOfFocusPositions, runTimeConfig.value('MINIMUM_FOCUSER_POSITIONS')))

        return self.isValid
    def countFocuserPositions(self):
        differentFocuserPositions={}
        for hdu in self.fitsHDUsList:
            differentFocuserPositions(hdu.headerElements['FOC_POS'])
            
        
        return len(differentFocuserPositions)

import time
import datetime
import glob
import os

class ServiceFileOperations():
    """Class performing various task on files, e.g. expansion to (full) path"""
    def __init__(self, runTimePath=None):
        self.now= datetime.datetime.now().isoformat()
        if( runTimePath==None):
            self.runTimePath='/'
        else:
            self.runTimePath= runTimePath
    def prefix( self, fileName):
        return 'rts2af-' +fileName

    def notFits(self, fileName):
        items= fileName.split('/')[-1]
        return items.split('.fits')[0]

    def absolutePath(self, fileName=None):
        if( fileName==None):
            logging.error('ServiceFileOperations.absolutePath: no file name given')
            
        pLeadingSlash = re.compile( r'\/.*')
        leadingSlash = pLeadingSlash.match(fileName)
        if( leadingSlash):
            return True
        else:
            return False

    def fitsFilesInRunTimePath( self):
        if( verbose):
            print "searching in " + repr(glob.glob( self.runTimePath + '/' + runTimeConfig.value('FILE_GLOB')))
        return glob.glob( self.runTimePath + '/' + runTimeConfig.value('FILE_GLOB'))

    def expandToTmp( self, fileName=None):
        if( self.absolutePath(fileName)):
            return fileName

        fileName= runTimeConfig.value('TEMP_DIRECTORY') + '/'+ fileName
        return fileName

    def expandToSkyList( self, fitsHDU=None):
        if( fitsHDU==None):
            logging.error('ServiceFileOperations.expandToCat: no hdu given')
        
        items= runTimeConfig.value('SEXSKY_LIST').split('.')
        try:
            fileName= self.prefix( items[0] +  '-' + fitsHDU.headerElements['FILTER'] + '-' +   self.now + '.' + items[1])
        except:
            fileName= self.prefix( items[0] + '-' +   self.now + '.' + items[1])
            
        if(verbose):
            print 'ServiceFileOperations:expandToSkyList expanded to ' + fileName
        
        return  self.expandToTmp(fileName)

    def expandToCat( self, fitsHDU=None):
        if( fitsHDU==None):
            logging.error('ServiceFileOperations.expandToCat: no hdu given')
        try:
            fileName= self.prefix( self.notFits(fitsHDU.fitsFileName) + '-' + fitsHDU.headerElements['FILTER'] + '-' + self.now + '.cat')
        except:
            fileName= self.prefix( self.notFits(fitsHDU.fitsFileName) + '-' + self.now + '.cat')

        return self.expandToTmp(fileName)

    def expandToFitInput(self, fitsHDU=None, element=None):
        items= runTimeConfig.value('FIT_RESULT_FILE').split('.')
        if((fitsHDU==None) or ( element==None)):
            logging.error('ServiceFileOperations.expandToFitInput: no hdu or elementgiven')

        fileName= items[0] + "-" + fitsHDU.headerElements['FILTER'] + "-" + self.now +  "-" + element + "." + items[1]
        return self.expandToTmp(self.prefix(fileName))

    def expandToFitImage(self, fitsHDU=None):
        if( fitsHDU==None):
            logging.error('ServiceFileOperations.expandToFitImage: no hdu given')

        items= runTimeConfig.value('FIT_RESULT_FILE').split('.') 
# ToDo png
        fileName= items[0] + "-" + fitsHDU.headerElements['FILTER'] + "-" + self.now + ".png"
        return self.expandToTmp(self.prefix(fileName))

    def expandToRunTimePath(self, fileName=None):
        if( self.absolutePath(fileName)):
            return fileName
        else:
            return self.runTimePath + '/' + fileName
# ToDo: refactor with expandToSkyList
    def expandToDs9RegionFileName( self, fitsHDU=None):
        if( fitsHDU==None):
            logging.error('ServiceFileOperations.expandToDs9RegionFileName: no hdu given')
        
        
        items= runTimeConfig.value('DS9_REGION_FILE').split('.')
        # not nice
        names= fitsHDU.fitsFileName.split('/')
        try:
            fileName= self.prefix( items[0] +  '-' + fitsHDU.headerElements['FILTER'] + '-' + self.now + '-' + names[-1] + '-' + str(fitsHDU.headerElements['FOC_POS']) + '.' + items[1])
        except:
            fileName= self.prefix( items[0] + '-' +   self.now + '.' + items[1])
            
        if(verbose):
            print 'ServiceFileOperations:expandToDs9RegionFileName expanded to ' + fileName
        
        return  self.expandToTmp(fileName)

    def expandToDs9CommandFileName( self, fitsHDU=None):
        if( fitsHDU==None):
            logging.error('ServiceFileOperations.expandToDs9COmmandFileName: no hdu given')
        
        items= runTimeConfig.value('DS9_REGION_FILE').split('.')
        try:
            fileName= self.prefix( items[0] +  '-' + fitsHDU.headerElements['FILTER'] + '-' + self.now + '.' + items[1] + '.sh')
        except:
            fileName= self.prefix( items[0] + '-' +   self.now + '.' + items[1]+ '.sh')
            
        if(verbose):
            print 'ServiceFileOperations:expandToDs9CommandFileName expanded to ' + fileName
        
        return  self.expandToTmp(fileName)
        
    def expandToAcquisitionBasePath(self, filter=None):

        if( filter== None):
            return runTimeConfig.value('BASE_DIRECTORY') + '/' + self.now + '/'  
        else: 
            return runTimeConfig.value('BASE_DIRECTORY') + '/' + self.now + '/' + filter.name + '/'
        
    def expandToTmpConfigurationPath(self, fileName=None):
        if( fileName==None):
            logging.error('ServiceFileOperations.expandToTmpConfigurationPath: no filename given')

        fileName= fileName + self.now + '.cfg'
        return self.expandToTmp(fileName)


    def expandToFitResultPath(self, fileName=None):
        if( fileName==None):
            logging.error('ServiceFileOperations.expandToFitResultPath: no filename given')

        return self.expandToTmp(fileName + self.now + '.fit')


    def reconstructFitResultPath(self, configFileName):
        # If the configuration file name of the present process has a time stamp, like
        # /tmp/rts2af-acquire-V-2011-04-22T11:32:20.061022.cfg
        # then it was created by rts2af_acquire.py
        # This timestamp is used to identify an existing file which contains the results
        # of a given focus run, e.g. the filters U:B:V:...
        # this is appropriate but not real good idea.
        date= re.search( r'([0-9]{1,4}-[0-9]{1,2}-[0-9]{1,2}T[0-9]{1,2}:[0-9]{1,2}:[0-9]{1,2}\.[0-9]{1,6})\.cfg', configFileName)
        originator= re.search(r'rts2af-(\w+)-', configFileName)

        if(date==None or originator==None):
            return None
        else:
            return self.expandToTmp( '{0}{1}.fit'.format(originator.group(0), date.group(1)))

    def createAcquisitionBasePath(self, filter=None):
        os.makedirs( self.expandToAcquisitionBasePath( filter))

        
    def defineRunTimePath(self, fileName=None):
        if( self.absolutePath(fileName)):
            return fileName

        for root, dirs, names in os.walk(runTimeConfig.value('BASE_DIRECTORY')):
            if( fileName in names):
                self.runTimePath= root
                return True
        return False
        

    
    def setModeExecutable(self, path):
        #mode = os.stat(path)
        os.chmod(path, 0744)

    #
# stub, will be called in main script 
runTimeConfig= Configuration()
serviceFileOp= ServiceFileOperations()
verbose= False
class Service():
    """Temporary class for uncategorized methods"""
    def __init__(self):
        print



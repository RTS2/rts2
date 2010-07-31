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
from operator import itemgetter
# globals
LOG_FILENAME = '/var/log/rts2-autofocus'
logger= logging.getLogger('rts2_af_logger') ;


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
            help='write defaults to configuration file ' + runTimeConfig.configurationFileName())

        parser.add_argument('--config', dest='fileName',
                            metavar='CONFIG FILE', nargs=1, type=str,
                            help='configuration file')

        parser.add_argument('-r', '--reference', dest='reference',
                            metavar='REFERENCE', nargs=1, type=str,
                            help='reference file name')

        parser.add_argument('-l', '--logging', dest='logLevel', action='store', 
                            metavar='LOGLEVEL', nargs=1, type=str,
                            default='warning',
                            help=' log level: usual levels')

#        parser.add_argument('-t', '--test', dest='test', action='store', 
#                            metavar='TEST', nargs=1,
#    no default means None                        default='myTEST',
#                            help=' test case, default: myTEST')

        parser.add_argument('-v', dest='verbose', 
                            action='store_true',
                            help=' print (some) messages to stdout'
                            )

        self.args= parser.parse_args()

        if(self.args.verbose):
            global verbose
            verbose= self.args.verbose
            runTimeConfig.dumpDefaults()

        if( self.args.write):
            runTimeConfig.writeDefaultConfiguration()
            print 'wrote default configuration to ' +  runTimeConfig.configurationFileName()
            sys.exit(0)

        return  self.args

    def configureLogger(self):

        if( self.args.logLevel[0]== 'debug'): 
            logging.basicConfig(filename=LOG_FILENAME,level=logging.DEBUG)
        else:
            logging.basicConfig(filename=LOG_FILENAME,level=logging.WARN)

        return logger

import ConfigParser
import string

class Configuration:
    """Configuration for any AFScript"""    
    def __init__(self, fileName='rts2-autofocus-offline.cfg'):
        self.configFileName = fileName
        self.values={}
        self.filters=[]
        self.filtersInUse=[]

        self.cp={}

        self.config = ConfigParser.RawConfigParser()
        
        self.cp[('basic', 'CONFIGURATION_FILE')]= '/etc/rts2/autofocus/rts2-autofocus.cfg'
        
        self.cp[('basic', 'BASE_DIRECTORY')]= '/tmp'
        self.cp[('basic', 'TEMP_DIRECTORY')]= '/tmp'
        self.cp[('basic', 'FILE_GLOB')]= 'X/*fits'
        self.cp[('basic', 'FITS_IN_BASE_DIRECTORY')]= False
        self.cp[('basic', 'CCD_CAMERA')]= 'CD'
        self.cp[('basic', 'CHECK_RTS2_CONFIGURATION')]= False

        self.cp[('filter properties', 'U')]= '[0, U, 5074, -1500, 1500, 100, 40]'
        self.cp[('filter properties', 'B')]= '[1, B, 4712, -1500, 1500, 100, 30]'
        self.cp[('filter properties', 'V')]= '[2, V, 4678, -1500, 1500, 100, 20]'
        self.cp[('filter properties', 'R')]= '[4, R, 4700, -1500, 1500, 100, 20]'
        self.cp[('filter properties', 'I')]= '[4, I, 4700, -1500, 1500, 100, 20]'
        self.cp[('filter properties', 'X')]= '[5, X, 3270, -1500, 1500, 100, 10]'
        self.cp[('filter properties', 'Y')]= '[6, Y, 3446, -1500, 1500, 100, 10]'
        self.cp[('filter properties', 'NOFILTER')]= '[6, NOFILTER, 3446, -1500, 1500, 109, 19]'
        
        self.cp[('focuser properties', 'FOCUSER_RESOLUTION')]= 20
        self.cp[('focuser properties', 'FOCUSER_ABSOLUTE_LOWER_LIMIT')]= 1501
        self.cp[('focuser properties', 'FOCUSER_ABSOLUTE_UPPER_LIMIT')]= 6002

        self.cp[('acceptance circle', 'CENTER_OFFSET_X')]= 0
        self.cp[('acceptance circle', 'CENTER_OFFSET_Y')]= 0
        self.cp[('acceptance circle', 'RADIUS')]= 2000.
        
        self.cp[('filters', 'filters')]= 'U:B:V:R:I:X:Y'
        
        self.cp[('DS9', 'DS9_REFERENCE')]= True
        self.cp[('DS9', 'DS9_MATCHED')]= True
        self.cp[('DS9', 'DS9_ALL')]= True
        self.cp[('DS9', 'DS9_DISPLAY_ACCEPTANCE_AREA')]= True
        self.cp[('DS9', 'DS9_REGION_FILE')]= 'ds9-autofocus.reg'
        
        self.cp[('analysis', 'ANALYSIS_UPPER_LIMIT')]= 1.e12
        self.cp[('analysis', 'ANALYSIS_LOWER_LIMIT')]= 0.
        self.cp[('analysis', 'MINIMUM_OBJECTS')]= 5
        self.cp[('analysis', 'INCLUDE_AUTO_FOCUS_RUN')]= False
        self.cp[('analysis', 'SET_LIMITS_ON_SANITY_CHECKS')]= True
        self.cp[('analysis', 'SET_LIMITS_ON_FILTER_FOCUS')]= True
        self.cp[('analysis', 'FIT_RESULT_FILE')]= 'fit-autofocus.dat'
        
        self.cp[('fitting', 'FOCROOT')]= 'rts2-fit-focus'
        
        self.cp[('SExtractor', 'SEXPRG')]= 'sex 2>/dev/null'
        self.cp[('SExtractor', 'SEXCFG')]= '/etc/rts2/autofocus/sex-autofocus.cfg'
        self.cp[('SExtractor', 'SEXPARAM')]= '/etc/rts2/autofocus/sex-autofocus.param'
        self.cp[('SExtractor', 'SEXREFERENCE_PARAM')]= '/etc/rts2/autofocus/sex-autofocus-reference.param'
        self.cp[('SExtractor', 'OBJECT_SEPARATION')]= 10.
        self.cp[('SExtractor', 'ELLIPTICITY')]= .1
        self.cp[('SExtractor', 'ELLIPTICITY_REFERENCE')]= .1
        self.cp[('SExtractor', 'SEXSKY_LIST')]= 'sex-autofocus-assoc-sky.list'
        self.cp[('SExtractor', 'SEXCATALOGUE')]= 'sex-autofocus.cat'
        self.cp[('SExtractor', 'SEX_TMP_CATALOGUE')]= 'sex-autofocus-tmp.cat'
        self.cp[('SExtractor', 'CLEANUP_REFERENCE_CATALOGUE')]= True
        
        self.cp[('mode', 'TAKE_DATA')]= True
        self.cp[('mode', 'SET_FOCUS')]= True
        self.cp[('mode', 'CCD_BINNING')]= 0
        self.cp[('mode', 'AUTO_FOCUS')]= False
        self.cp[('mode', 'NUMBER_OF_AUTO_FOCUS_IMAGES')]= 10
        
        self.cp[('rts2', 'RTS2_DEVICES')]= '/etc/rts2/devices'
        
        self.defaults={}
        for (section, identifier), value in sorted(self.cp.iteritems()):
            self.defaults[(identifier)]= value
#            print 'value ', self.defaults[(identifier)]
 
    def configurationFileName(self):
        return self.configFileName

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

    def filters(self):
        return self.filters

    def filterByName(self, name):
        for filter in  self.filters:
            print "NAME>" + name + "<>" + filter.filterName
            if( name == filter.filterName):
                print "NAME" + name + filter.filterName
                return filter

        return False

    def filtersInUse(self):
        return self.filtersInUse

    def filterInUseByName(self, name):
        for filter in  self.filtersInUse:
            if( name == filter.filterName):
                return filter
        return False

    def writeDefaultConfiguration(self):
        for (section, identifier), value in sorted(self.cp.iteritems()):
#            print section, '=>', identifier, '=>', value
            if( self.config.has_section(section)== False):
                self.config.add_section(section)

            self.config.set(section, identifier, value)

        with open( self.configFileName, 'wb') as configfile:
            configfile.write('# 2010-07-10, Markus Wildi\n')
            configfile.write('# default configuration for rts2-autofocus.py\n')
            configfile.write('# generated with rts2-autofous.py -p\n')
            configfile.write('# see man rts2-autofocus.py for details\n')
            configfile.write('#\n')
            configfile.write('#\n')
            self.config.write(configfile)

    def readConfiguration( self, configFileName):

        config = ConfigParser.ConfigParser()
        try:
            config.readfp(open(configFileName))
        except:
            logger.error('Configuration.readConfiguration: config file ' + configFileName + ' not found, exiting')
            sys.exit(1)

# read the defaults
        for (section, identifier), value in self.configIdentifiers():
             self.values[identifier]= value

# over write the defaults
        for (section, identifier), value in self.configIdentifiers():

            try:
                value = config.get( section, identifier)
            except:
                logger.info('Configuration.readConfiguration: no section ' +  section + ' or identifier ' +  identifier + ' in file ' + configFileName)
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
                    logger.error('Configuration.readConfiguration: no int '+ value+ 'in section ' +  section + ', identifier ' +  identifier + ' in file ' + configFileName)
                    

            elif(isinstance(self.values[identifier], float)):
                try:
                    self.values[identifier]= float(value)
                except:
                    logger.error('Configuration.readConfiguration: no float '+ value+ 'in section ' +  section + ', identifier ' +  identifier + ' in file ' + configFileName)

            else:
                self.values[identifier]= value
                items=[] ;
                if( section == 'filter properties'): 
                    items= value[1:-1].split(',')
#, ToDo, hm
                    for item in items: 
                        item= string.replace( item, ' ', '')

                    items[1]= string.replace( items[1], ' ', '')
                    print '-----------filterInUse---------:>' + items[1] + '<'
                    self.filters.append( Filter( items[1], string.atoi(items[2]), string.atoi(items[3]), string.atoi(items[4]), string.atoi(items[5]), string.atoi(items[6])))
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
               print 'Filter --------' + filter.filterName
               print 'Filter --------%d'  % filter.exposure 

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
        self.paramsSExtractorReference= []
        self.paramsSExtractorAssoc= []

    def lengthAssoc(self):
        return len(self.paramsSExtractorAssoc)

    def elementIndex(self, element):
        for ele in  self.paramsSExtractorAssoc:
            if( element== ele) : 
                return self.paramsSExtractorAssoc.index(ele)
        else:
            return False

    def index(self, element):
        return self.paramsSExtractorAssoc(element)

    def identifiersReference(self):
        return self.paramsSExtractorReference

    def identifiersAssoc(self):
        return self.paramsSExtractorAssoc


    def readSExtractorParams(self):
        params=open( self.paramsFileName, 'r')
        lines= params.readlines()
        pElement = re.compile( r'([\w]+)')
        for (i, line) in enumerate(lines):
            element = pElement.match(line)
            if( element):
                if(verbose):
                    print "element.group(1) : ", element.group(1)
                self.paramsSExtractorReference.append(element.group(1))
                self.paramsSExtractorAssoc.append(element.group(1))

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
            
        for element in self.paramsSExtractorReference:
            self.paramsSExtractorAssoc.append('ASSOC_'+ element)

        self.paramsSExtractorAssoc.append('NUMBER_ASSOC')

        for element in  self.paramsSExtractorReference:
            print "Reference element : >"+ element+"<"
        for element in  self.paramsSExtractorAssoc:
            print "Association element : >" + element+"<"


class Filter:
    """Class for filter properties"""

    def __init__(self, filterName, focDef, lowerLimit, upperLimit, resolution, exposure, ):
        self.filterName= filterName
        self.focDef    = focDef
        self.lowerLimit= lowerLimit
        self.upperLimit= upperLimit
        self.stepSize  = resolution # [tick]
        self.exposure  = exposure

    def filterName(self):
        return self.filterName
    def focDef(self):
        return self.focDef 
    def lowerLimit(self):
        return self.lowerLimit
    def upperLimit(self):
        return  self.upperLimit
    def stepSize(self):
        return self.stepSize
    def exposure(self):
        return self.exposure


class SXobject():
    """Class holding the used properties of SExtractor object"""
    def __init__(self, objectNumber=None, position=None, fwhm=None, flux=None, distance=True, properties=True, isSelected=False):
        self.objectNumber= objectNumber
        self.position= position # is a list (x,y)
        self.isSelected= isSelected
        self.fwhm=-1
        self.flux=-1
        self.distance  = distance
        self.properties= properties


    def position(self):
        return self.position

    def isSelected(self, isSelected=None):
        if( isSelected==None) :
            return  self.isSelected
        else:
             self.isSelected= isSelected

    def fwhm(self):
        return  self.fwhm

    def flux(self):
        return  self.flux

    def distanceOK(self, ok=None):
        if( ok== None):
            return self.distance
        else:
            self.distance= ok

    def propertiesOK(self, ok=None):
        if( ok== None):
            return self.properties
        else:
            self.distance= ok

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

import shlex
import subprocess
import re
from math import sqrt
class Catalogue():
    """Class for a catalogue (SExtractor result)"""
    def __init__(self, fitsHDU=None, SExtractorParams=None):
        self.fitsHDU  = fitsHDU
        self.catalogueFileName= serviceFileOp.expandToCat(self.fitsHDU)
        self.lines= []
        self.catalogue = {}
        self.sxobjects = {}
        self.multiplicity = {}
        self.isReference = False
        self.isValid     = False
        self.objectSeparation2= runTimeConfig.value('OBJECT_SEPARATION')**2
        self.SExtractorParams = SExtractorParams
        self.indexeFlag       = self.SExtractorParams.paramsSExtractorAssoc.index('FLAGS')  
        self.indexellipticity = self.SExtractorParams.paramsSExtractorAssoc.index('ELLIPTICITY')

        Catalogue.__lt__ = lambda self, other: self.fitsHDU.headerElements['FOC_POS'] < other.fitsHDU.headerElements['FOC_POS']

    def fitsHDU(self):
        return self.fitsHDU

    def isReference(self):
        return self.isReference

    def writeCatalogue(self):
        if( not self.isReference):
            return False
        else:
            logger.error( 'Catalogue.writeCatalogue: writing reference catalogue, for ' + self.fitsHDU.fitsFileName) 

        pElement = re.compile( r'#[ ]+([0-9]+)[ ]+([\w]+)')
        pData    = re.compile( r'^[ \t]+([0-9]+)[ \t]+')

        SXcat= open( '/tmp/test', 'wb')
        logger.error( "writeCatalogue====================Catalogue.removeObject: length %d"  % len(self.sxobjects))

        for line in self.lines:
            element= pElement.search(line)
            data   = pData.search(line)
            if(element):
                SXcat.write(line)
            else:
                if((data.group(1)) in self.sxobjects):
                    try:
                        SXcat.write(line)
                    except:
                        logger.error( 'Catalogue.writeCatalogue: could not write line for object ' + data.group(1))
                        sys.exit(1)
                        break
        SXcat.close()

    def extractToCatalogue(self):
        if( verbose):
            print 'sextractor  ' + runTimeConfig.value('SEXPRG')
            print 'sextractor  ' + runTimeConfig.value('SEXCFG')
            print 'sextractor  ' + runTimeConfig.value('SEXREFERENCE_PARAM')

        # 2>/dev/null swallowed by PIPE
        (prg, arg)= runTimeConfig.value('SEXPRG').split(' ')

        if( self.fitsHDU.isReference):
            self.isReference = True 
            cmd= [  prg,
                    self.fitsHDU.fitsFileName, 
                    '-c ',
                    runTimeConfig.value('SEXCFG'),
                    '-CATALOG_NAME',
                    serviceFileOp.expandToSkyList(self.fitsHDU),
                    '-PARAMETERS_NAME',
                    runTimeConfig.value('SEXREFERENCE_PARAM'),]
        else:
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
            logging.error( 'Catalogue.extractToCatalogue: I/O error({0}): {1}'.format(errno, strerror))
            sys.exit(1)

        except:
            logging.error('Catalogue.extractToCatalogue: '+ repr(cmd) + ' died')
            sys.exit(1)

# checking against reference items see http://www.wellho.net/resources/ex.php4?item=y115/re1.py
    def createCatalogue(self):
        if( self.SExtractorParams==None):
            logger.error( 'Catalogue.createCatalogue: no SExtractor parameter configuration given')
            return False

        if( not self.fitsHDU.isReference):
            SXcat= open( self.catalogueFileName, 'r')
        else:
            SXcat= open( serviceFileOp.expandToSkyList(self.fitsHDU), 'r')

        self.lines= SXcat.readlines()
        SXcat.close()

        # rely on the fact that first line is stored as first element in list
        # match, element, data
        ##   2 X_IMAGE                Object position along x                                    [pixel]
        #         1   2976.000      7.473     2773.781     219.8691     42.19405  -5.8554   0.2084     120.5605         6  26     4.05      5.330    0.823
 
        pElement = re.compile( r'#[ ]+([0-9]+)[ ]+([\w]+)')
        pData    = re.compile( r'')
        objectNumber= -1
        itemNrX_IMAGE= self.SExtractorParams.paramsSExtractorAssoc.index('X_IMAGE')
        itemNrY_IMAGE= self.SExtractorParams.paramsSExtractorAssoc.index('Y_IMAGE')
        itemNrFWHM_IMAGE= self.SExtractorParams.paramsSExtractorAssoc.index('FWHM_IMAGE')
        itemNrFLUX_MAX= self.SExtractorParams.paramsSExtractorAssoc.index('FLUX_MAX')

        itemNrASSOC_NUMBER= self.SExtractorParams.paramsSExtractorAssoc.index('ASSOC_NUMBER')

        for (lineNumber, line) in enumerate(self.lines):
            element = pElement.match(line)
            data    = pData.match(line)
            x= -1
            y= -1
            objectNumberASSOC= '-1'
            if( element):
                if( not ( element.group(2)== 'VECTOR_ASSOC')):
                    try:
                        self.SExtractorParams.paramsSExtractorAssoc.index(element.group(2))
                    except:
                        logger.error( 'Catalogue.createCatalogue: no matching element for ' + element.group(2) +' found')
                        break
            elif( data):
                items= line.split()
                objectNumber= items[0] # NUMBER
                
                for (j, item) in enumerate(items):
                    
                    if( j == itemNrX_IMAGE):
                        x= float(item)
                    if( j == itemNrY_IMAGE):
                        y= float(item)
                    if( not self.isReference):
                        if( j == itemNrASSOC_NUMBER):
                            objectNumberASSOC= item 
                    # here
                    self.catalogue[(objectNumber, self.SExtractorParams.paramsSExtractorAssoc[j])]=  float(item)


                self.sxobjects[objectNumber]= SXobject(objectNumber, (float(items[itemNrX_IMAGE]), float(items[itemNrY_IMAGE])), items[itemNrFWHM_IMAGE], items[itemNrFLUX_MAX]) # position, bool is used for clean up (default True, True)
                if( not self.isReference):
                    if( objectNumberASSOC in self.multiplicity): # interesting
                        self.multiplicity[objectNumberASSOC] += 1
                    else:
                        self.multiplicity[objectNumberASSOC]= 1
            else:
                logger.error( 'Catalogue.readCatalogue: no match on line %d' % lineNumber)
                logger.error( 'Catalogue.readCatalogue: ' + line)
                break

        else: # exhausted 
            logger.error( 'Catalogue.readCatalogue: catalogue created ' + self.fitsHDU.fitsFileName)
            self.isValid= True

        return self.isValid

    def printCatalogue(self):
        for (objectNumber,identifier), value in sorted( self.catalogue.iteritems()):
# ToDo, remove me:
            if( objectNumber== "12"):
                print  "printCatalogue " + objectNumber + ">"+ identifier + "< %f"% value 


    def printObject(self, objectNumber):
        if( self.isReference):
            
            for itentifier in self.SExtractorParams.identifiersReference():
            
                if(((objectNumber, itentifier)) in self.catalogue):
                    print "printObject: reference object number " + objectNumber + " identifier " + itentifier + "value %f" % self.catalogue[(objectNumber, itentifier)]
                else:
                    logger.error( "Catalogue.printObject: reference Object number " + objectNumber + " for >" + itentifier + "< not found, break")
                    break
            else:
#                logger.error( "Catalogue.printObject: for object number " + objectNumber + " all elements printed")
                return True
        else:
            for itentifier in self.SExtractorParams.identifiersAssoc():
                if(((objectNumber, itentifier)) in self.catalogue):
                    print "printObject: object number " + objectNumber + " identifier " + itentifier + "value %f" % self.catalogue[(objectNumber, itentifier)]
                else:
                    logger.error( "Catalogue.printObject: object number " + objectNumber + " for >" + itentifier + "< not found, break")
                    break
            else:
#                logger.error( "Catalogue.printObject: for object number " + objectNumber + " all elements printed")
                return True

        return False

    def removeObject(self, objectNumber):
        if( not objectNumber in self.sxobjects):
            logger.error( "Catalogue.removeObject: reference Object number " + objectNumber + " for >" + itentifier + "< not found in sxobjects")
        else:
            if( objectNumber in self.sxobjects):
                del self.sxobjects[objectNumber]
            else:
                logger.error( "Catalogue.removeObject: object number " + objectNumber + " not found")

        if( self.isReference):
            for itentifier in self.SExtractorParams.identifiersReference():
                if(((objectNumber, itentifier)) in self.catalogue):
                    del self.catalogue[(objectNumber, itentifier)]
                else:
                    logger.error( "Catalogue.removeObject: reference Object number " + objectNumber + " for >" + itentifier + "< not found, break")
                    break
            else:
#                logger.error( "Catalogue.removeObject: for object number " + objectNumber + " all elements deleted")
                return True
        else:
            for itentifier in self.SExtractorParams.identifiersAssoc():
                if(((objectNumber, itentifier)) in self.catalogue):
                    del self.catalogue[(objectNumber, itentifier)]
                else:
                    logger.error( "Catalogue.removeObject: object number " + objectNumber + " for >" + itentifier + "< not found, break")
                    break
            else:
#                logger.error( "Catalogue.removeObject: for object number " + objectNumber + " all elements deleted")
                return True

        return False

    def distanceOK( self, position1, position2):

        dx= abs(position2[0]- position1[0])
        dy= abs(position2[1]- position1[1])
        if(( dx < runTimeConfig.value('OBJECT_SEPARATION')) and ( dy< runTimeConfig.value('OBJECT_SEPARATION'))):
            if( (dx**2 + dy**2) < self.objectSeparation2):
                return False
        # TRUE    
        return True

    # now done on catalogue for the sake of flexibility
    # ToDo, define if that shoud go into SXobject (now, better not)
    def checkProperties(self, objectNumber): 
        if( self.catalogue[( objectNumber, 'FLAGS')] != 0): # ToDo, ATTENTION
            return False
        elif( self.catalogue[( objectNumber, 'ELLIPTICITY')] > runTimeConfig.value('ELLIPTICITY_REFERENCE')): # ToDo, ATTENTION
            return False
        # TRUE    
        return True

    def cleanUpReference(self):
        if( not  self.isReference):
            logger.error( 'Catalogue.cleanUpReference: clean up only for a reference catalogue, I am : ' + self.fitsHDU.fitsFileName) 
            return False
        else:
            logger.error( 'Catalogue.cleanUpReference: reference catalogue, I am : ' + self.fitsHDU.fitsFileName)

        for objectNumber1, sxobject1 in self.sxobjects.iteritems():
            for objectNumber2, sxobject2 in self.sxobjects.iteritems():
                if( objectNumber1 != objectNumber2):
                    if( not self.distanceOK( sxobject1.position, sxobject2.position)):
                        sxobject1.distanceOK(False)
                        sxobject2.distanceOK(False)

            else:
                if( not self.checkProperties(objectNumber1)):
                    sxobject1.propertiesOK(False)

        discardedObjects= 0
        objectNumbers= self.sxobjects.keys()
        for objectNumber in objectNumbers:
            if(( not self.sxobjects[objectNumber].distanceOK()) or ( not self.sxobjects[objectNumber].propertiesOK())):
                if( not self.sxobjects[objectNumber].distanceOK()):
                    self.removeObject( objectNumber)
                    discardedObjects += 1

        logger.error("Number of objects discarded %d " % discardedObjects) 

# example how to access the catalogue
    def average(self, variable):
        sum= 0
        i= 0
        for (objectNumber,identifier), value in self.catalogue.iteritems():
            if( identifier == variable):
                sum += float(value)
                i += 1

        if(verbose):
            if( i != 0):
                print 'average ' + variable + ' %f ' % (sum/ float(i)) 
                return (sum/ float(i))
            else:
                print 'Error in average i=0'
                return False

class Catalogues():
    """Class holding Catalogues"""
    def __init__(self):
        self.CataloguesList= []
        self.isReference = False
        self.isValid= False

    def append( self, Catalogue):
        self.CataloguesList.append(Catalogue)

    def catalogues(self):
        return self.CataloguesList

    def isValid():
        return self.isValid

    def findReference(self):
        for cat in self.CataloguesList:
            if( cat.isReference):
                if( verbose):
                    print 'Catalogues.findReference: reference catalogue found for file: ' + cat.fitsHDU.fitsFileName
                return cat
        if( verbose):
            print 'Catalogues: no reference fits file found'
        return False

    def validate(self):
        catr= self.findReference()
        if( catr== False):
            return self.isValid

        if( len(self.CataloguesList) > 1):
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
                        self.CataloguesList.remove(cat)
                    except:
                        logger.error('Catalogues.validate: could not remove cat for ' + cat.fitsHDU.fitsFileName)
                        if(verbose):
                            print "Catalogues.validate: failed to removed cat: " + cat.fitsHDU.fitsFileName
                    break
            else:
                if( len(self.CataloguesList) > 0):
                    self.isValid= True
                    return self.isValid
        return False

import sys
import pyfits

class FitsHDU():
    """Class holding fits file name and ist properties"""
    def __init__(self, fitsFileName=None, isReference=False):
        self.fitsFileName= fitsFileName
        self.isReference= isReference
        self.isValid= False
        self.headerElements={}
        FitsHDU.__lt__ = lambda self, other: self.headerElements['FOC_POS'] < other.headerElements['FOC_POS']

    def keys(self):
        return self.headerElements.keys()

    def isFilter(self, filter):
        if( verbose):
            print "fits file path: " + self.fitsFileName
        try:
            fitsHDU = pyfits.fitsopen(self.fitsFileName)
        except:
            if( self.fitsFileName):
                logger.error('FitsHDU: file not found : ' + self.fitsFileName)
            else:
                logger.error('FitsHDU: file name not given (None), exiting')
            sys.exit(1)

        fitsHDU.close()

        if( filter == fitsHDU[0].header['FILTER']):
            self.headerElements['FILTER'] = fitsHDU[0].header['FILTER']
            self.headerElements['FOC_POS']= fitsHDU[0].header['FOC_POS']
            self.headerElements['BINNING']= fitsHDU[0].header['BINNING']
            self.headerElements['NAXIS1'] = fitsHDU[0].header['NAXIS1']
            self.headerElements['NAXIS2'] = fitsHDU[0].header['NAXIS2']
            self.headerElements['ORIRA']  = fitsHDU[0].header['ORIRA']
            self.headerElements['ORIDEC'] = fitsHDU[0].header['ORIDEC']
            return True
        else:
            return False

class FitsHDUs():
    """Class holding FitsHDU"""
    def __init__(self):
        self.fitsHDUsList= []
        self.isValid= False

    def append(self, FitsHDU):
        self.fitsHDUsList.append(FitsHDU)

    def fitsHDUs(self):
        return self.fitsHDUsList

    def findFintsFileByName( self, fileName):
        for hdu in sorted(self.fitsHDUsList):
            if( fileName== hdu.fitsFileName):
                return hdu
        return False

    def findReference(self):
        for hdu in sorted(self.fitsHDUsList):
            if( hdu.isReference):
                return hdu
        return False

    def findReferenceByFocPos(self, filterName):
        filter=  runTimeConfig.filterByName(filterName)
#        for hdu in sorted(self.fitsHDUsList):
        for hdu in sorted(self.fitsHDUsList):
            if( filter.focDef < hdu.headerElements['FOC_POS']):
                print "FOUND reference by foc pos ==============" + hdu.fitsFileName + "======== %d" % hdu.headerElements['FOC_POS']
                return hdu
        return False

    def isValid(self):
        return self.isValid

    def validate(self):
        hdur= self.findReference()
        if( hdur== False):
            hdur= self.findReferenceByFocPos()
            if( hdur== False):
                if( verbose):
                    print "Nothing found in  findReference and findReferenceByFocPos"
                    return self.isValid

        keys= hdur.keys()
        i= 0
        for hdu in self.fitsHDUsList:
            for key in keys:
                if(key == 'FOC_POS'):
                    continue
                if( hdur.headerElements[key]!= hdu.headerElements[key]):
                    try:
                        self.fitsHDUsList.remove(hdu)
                    except:
                        logger.error('FitsHDUs.validate: could not remove hdu for ' + hdu.fitsFileName)
                        if(verbose):
                            print "FitsHDUs.validate: removed hdu: " + hdu.fitsFileName + "========== %d" %  self.fitsHDUsList.index(hdu)
                    break # really break out the keys loop 
            else: # exhausted
                hdu.isValid= True
                if( verbose):
                    print 'FitsHDUs.validate: valid hdu: ' + hdu.fitsFileName


        if( len(self.fitsHDUsList) > 0):
            self.isValid= True

        return self.isValid

import time
import datetime
import glob
class ServiceFileOperations():
    """Class performing various task on files, e.g. expansio to (full) path"""
    def __init__(self):
        self.now= datetime.datetime.now().isoformat()

    def prefix( self):
        return 'rts2af-'

    def notFits(self, fileName):
        items= fileName.split('/')[-1]
        return items.split('.fits')[0]

    def expandToTmp( self, fileName=None):
        if( fileName==None):
            logger.error('ServiceFileOperations.expandToTmp: no file name given')

        fileName= runTimeConfig.value('TEMP_DIRECTORY') + '/'+ fileName
        return fileName

    def expandToSkyList( self, fitsHDU=None):
        if( fitsHDU==None):
            logger.error('ServiceFileOperations.expandToCat: no hdu given')
        
        items= runTimeConfig.value('SEXSKY_LIST').split('.')
        fileName= self.prefix() + items[0] +  '-' + fitsHDU.headerElements['FILTER'] + '-' +   self.now + '.' + items[1]
        if(verbose):
            print 'ServiceFileOperations:expandToSkyList expanded to ' + fileName
        
        return  self.expandToTmp(fileName)

    def expandToCat( self, fitsHDU=None):
        if( fitsHDU==None):
            logger.error('ServiceFileOperations.expandToCat: no hdu given')
        fileName= self.prefix() + self.notFits(fitsHDU.fitsFileName) + '-' + fitsHDU.headerElements['FILTER'] + '-' + self.now + '.cat'
        return self.expandToTmp(fileName)

    def findFitsHDUs( self):
        if( verbose):
            print "searching in " + runTimeConfig.value('BASE_DIRECTORY') + '/' + runTimeConfig.value('FILE_GLOB')
        return glob.glob( runTimeConfig.value('BASE_DIRECTORY') + '/' + runTimeConfig.value('FILE_GLOB'))

#
# stub, will be called in main script 
runTimeConfig= Configuration()
serviceFileOp= ServiceFileOperations()
verbose= False
class Service():
    """Temporary class for uncategorized methods"""
    def __init__(self):
        print

#  LocalWords:  itentifier

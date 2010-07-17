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
        self.filtersInUse=[]

        self.cp={}

        self.config = ConfigParser.RawConfigParser()
        
        self.cp[('basic', 'CONFIGURATION_FILE')]= '/etc/rts2/autofocus/rts2-autofocus.cfg'
        
        self.cp[('basic', 'BASE_DIRECTORY')]= '/tmp'
        self.cp[('basic', 'TEMP_DIRECTORY')]= '/tmp'
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
        self.cp[('DS9', 'DS9_REGION_FILE')]= '/tmp/ds9-autofocus.reg'
        
        self.cp[('analysis', 'ANALYSIS_UPPER_LIMIT')]= 1.e12
        self.cp[('analysis', 'ANALYSIS_LOWER_LIMIT')]= 0.
        self.cp[('analysis', 'MINIMUM_OBJECTS')]= 5
        self.cp[('analysis', 'INCLUDE_AUTO_FOCUS_RUN')]= False
        self.cp[('analysis', 'SET_LIMITS_ON_SANITY_CHECKS')]= True
        self.cp[('analysis', 'SET_LIMITS_ON_FILTER_FOCUS')]= True
        self.cp[('analysis', 'FIT_RESULT_FILE')]= '/tmp/fit-autofocus.dat'
        
        self.cp[('fitting', 'FOCROOT')]= 'rts2-fit-focus'
        
        self.cp[('SExtractor', 'SEXPRG')]= 'sex 2>/dev/null'
        self.cp[('SExtractor', 'SEXCFG')]= '/etc/rts2/autofocus/sex-autofocus.cfg'
        self.cp[('SExtractor', 'SEXPARAM')]= '/etc/rts2/autofocus/sex-autofocus.param'
        self.cp[('SExtractor', 'SEXREFERENCE_PARAM')]= '/etc/rts2/autofocus/sex-autofocus-reference.param'
        self.cp[('SExtractor', 'OBJECT_SEPARATION')]= 10.
        self.cp[('SExtractor', 'ELLIPTICITY')]= .1
        self.cp[('SExtractor', 'ELLIPTICITY_REFERENCE')]= .1
        self.cp[('SExtractor', 'SEXSKY_LIST')]= '/tmp/sex-autofocus-assoc-sky.list'
        self.cp[('SExtractor', 'SEXCATALOGUE')]= '/tmp/sex-autofocus.cat'
        self.cp[('SExtractor', 'SEX_TMP_CATALOGUE')]= '/tmp/sex-autofocus-tmp.cat'
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

    def filtersInUse(self):
        return self.filtersInUse

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
            logger.error('readConfiguration: config file ' + configFileName + ' not found, exiting')
            sys.exit(1)

# read the defaults
        for (section, identifier), value in self.configIdentifiers():
             self.values[identifier]= value

# over write the defaults
        filters=[]
        for (section, identifier), value in self.configIdentifiers():

            try:
                value = config.get( section, identifier)
            except:
                logger.info('readConfiguration: no section ' +  section + ' or identifier ' +  identifier + ' in file ' + configFileName)
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
                    logger.error('readConfiguration: no int '+ value+ 'in section ' +  section + ', identifier ' +  identifier + ' in file ' + configFileName)
                    

            elif(isinstance(self.values[identifier], float)):
                try:
                    self.values[identifier]= float(value)
                except:
                    logger.error('readConfiguration: no float '+ value+ 'in section ' +  section + ', identifier ' +  identifier + ' in file ' + configFileName)

            else:
                self.values[identifier]= value
                items=[] ;
                if( section == 'filter properties'): 
                    items= value[1:-1].split(',')
#, ToDo, hm
                    for item in items: 
                        item.replace(' ', '')

                    filters.append( Filter( items[1], string.atoi(items[2]), string.atoi(items[3]), string.atoi(items[4]), string.atoi(items[5]), string.atoi(items[6])))
                elif( section == 'filters'):
                    items= value.split(':')
                    for item in items:
                        item.replace(' ', '')
                        if(verbose):
                            print 'filterInUse---------:' + item
                        self.filtersInUse.append(item)

        if(verbose):
            for (identifier), value in self.defaultsIdentifiers():
               print "over ", identifier, self.values[(identifier)]

        if(verbose):
           for filter in filters:
               print 'Filter --------' + filter.filterName
               print 'Filter --------%d'  % filter.exposure 

        if(verbose):
            for filters in self.filtersInUse:
                print "used filters :", filters

        return self.values


class Filter:
    """Class for filter properties"""

    def __init__(self, filterName, focDef, lowerLimit, upperLimit, resolution, exposure, ):
        self.filterName= filterName
        self.focDef    = focDef
        self.lowerLimit= lowerLimit
        self.upperLimit= upperLimit
        self.stepSize  = resolution # [tick]
        self.exposure  = exposure

    def filerName(self):
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

import shlex
import subprocess
import re
class Catalogue():
    """Class for a catalogue (SExtractor result)"""
    def __init__(self, fitsFileName=None):
        self.fitsFileName     = fitsFileName
        self.catalogueFileName= serviceFileOp.expandToCat(fitsFileName)
        self.catalogue = {}
        self.elements  = {}

    def create_catalogue_reference(self):
        self.create_catalogue()
        self.readCatalogue()

    def create_catalogue(self):
        if( verbose):
            print 'sextractor  ' + runTimeConfig.value('SEXPRG')
            print 'sextractor  ' + runTimeConfig.value('SEXCFG')
            print 'sextractor  ' + runTimeConfig.value('SEXREFERENCE_PARAM')

        (prg, arg)= runTimeConfig.value('SEXPRG').split(' ')
        # 2>/dev/null swallowed by PIPE
        cmd= [  prg,
                self.fitsFileName, 
                '-c ',
                runTimeConfig.value('SEXCFG'),
                '-CATALOG_NAME',
                self.catalogueFileName,
                '-PARAMETERS_NAME',
                runTimeConfig.value('SEXREFERENCE_PARAM'),]
                
        try:
            output = subprocess.Popen( cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]

        except OSError as (errno, strerror):
            logging.error( 'create_catalogue: I/O error({0}): {1}'.format(errno, strerror))
            sys.exit(1)

        except:
            logging.error('create_catalogue: '+ repr(cmd) + ' died')
            sys.exit(1)

# checking against reference items see http://www.wellho.net/resources/ex.php4?item=y115/re1.py
    def readCatalogue(self):
        cat= open( self.catalogueFileName, 'r')
        lines= cat.readlines()
        # rely on the fact that first line is stored as first element in list
        # match, element, data
        ##   2 X_IMAGE                Object position along x                                    [pixel]
        #         1   2976.000      7.473     2773.781     219.8691     42.19405  -5.8554   0.2084     120.5605         6  26     4.05      5.330    0.823
 
        #element= re.compile("#[ ]+[0-9]+[ ]+(\w)")
        pElement = re.compile( r'#[ ]+([0-9]+)[ ]+([\w]+)')
        pData    = re.compile( r'')
        for (i, line) in enumerate(lines):
            element = pElement.match(line)
            data    = pData.match(line)
            if( element):
                if(verbose):
                    print "element.group(1) : ", element.group(1)
                    print "element.group(2) : ", element.group(2)
                self.elements[element.group(1)]=element.group(2)

            elif( data):
                if(verbose):
                    print line

                items= line.split()
                if(verbose):
                    for item in items:
                        print item

                for (i, item) in enumerate(items):
                    try:
                        self.catalogue[(i, self.elements[i+1])]= item
                    except:
                        for element, value in self.elements.iteritems():
                            print "element" + element + " " + value 
            else:
                logger.error( 'readCatalogue: no match on line %d' % i)
                logger.error( 'readCatalogue: ' + line)
                


import sys
import pyfits

class FitsFile():
    """Class holding fits file name and ist properties"""
    def __init__(self, fitsFileName=None, isReference=False, catalogueFileName= None):
        self.fitsFileName= fitsFileName
        self.isReference = isReference
        self.isValid= False

        try:
            hdulist = pyfits.fitsopen(fitsFileName)
        except:
            if( fitsFileName):
                logger.error('FitsFile: file not found : ' + fitsFileName)
            else:
                logger.error('FitsFile: file name not given (None), exiting')
            sys.exit(1)
        if( verbose):
            hdulist.info()

        self.filter = hdulist[0].header['FILTER']
        self.foc_pos= hdulist[0].header['FOC_POS']
        self.binning= hdulist[0].header['BINNING']
        self.naxis1 = hdulist[0].header['NAXIS1']
        self.naxis2 = hdulist[0].header['NAXIS2']
        self.oira   = hdulist[0].header['ORIRA']
        self.oridec = hdulist[0].header['ORIDEC']

        hdulist.close()

class FitsFiles():
    """Class holding FitsFile"""
    def __init__(self):
        self.fitsFilesList= []
        self.isValid= False

    def add( self, FitsFile):
        self.fitsFilesList.append(FitsFile)

    def fitsFiles(self):
        return self.fitsFilesList

    def reference(self):
        for hdu in self.fitsFilesList:
            if( hdu.isReference):
                if( verbose):
                    print 'reference fits file found:' + hdu.fitsFileName
                return hdu

        if( verbose):
            print 'no reference fits file found'

        print 'no reference fits file found'
        return None

    def validate(self):
        
        hdur= self.reference()
        if( hdur== None):
            return self.isValid

        if( len(self.fitsFilesList) > 1):
            # default is False
            for hdu in self.fitsFilesList[0:]:
                if( hdu.filter== hdur.filter):
                    if( hdu.binning== hdur.binning):
                        if( hdu.naxis1== hdur.naxis1):
                            if( hdu.naxis2== hdur.naxis2):
                                print 'OK'
                                hdu.isValid= True

                if(hdu.isValid==False):
                    print "rem"
                    self.fitsFilesList.remove(hdu)

            if( len(self.fitsFilesList) > 0):
                self.isValid= True

            return self.isValid
                
    def isValid():
        return self.isValid
import time
import datetime
class ServiceFileOperations():
    """Class performing various task on files, e.g. expansio to (full) path"""
    def __init__(self):
        self.now= datetime.datetime.now().isoformat()

    def prefix( self):
        return 'rts2af-'

    def expandToTmp( self, fileName=None):
        if( fileName==None):
            logger.error('expandToTmp: no file name given')
        
        fileName= runTimeConfig.value('TEMP_DIRECTORY') + '/'+ fileName
        return fileName

    def expandToCat( self, fileName=None):
        if( fileName==None):
            logger.error('expandToCat: no file name given')
        fileName= self.prefix() + fileName.split('.fits')[0] + '-' + self.now + '.cat'
        return self.expandToTmp(fileName)

#
# stub, will be called in main script 
runTimeConfig= Configuration()
serviceFileOp= ServiceFileOperations()
verbose= False
class Service():
    """Temporary class for uncategorized methods"""
    def __init__(self):
        print

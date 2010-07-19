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
    def __init__(self, fitsFile=None):
        self.fitsFile  = fitsFile
        self.catalogueFileName= serviceFileOp.expandToCat(self.fitsFile.fitsFileName)
        self.catalogue = {}
        self.elements  = {}
        self.isReference = False
        self.isValid= False
        print "===============================" + repr(self.isReference) + "=========="  + self.fitsFile.fitsFileName

    def hdu(self):
        return self.fitsFile

    def extractToCatalogue(self):
        if( verbose):
            print 'sextractor  ' + runTimeConfig.value('SEXPRG')
            print 'sextractor  ' + runTimeConfig.value('SEXCFG')
            print 'sextractor  ' + runTimeConfig.value('SEXREFERENCE_PARAM')

        (prg, arg)= runTimeConfig.value('SEXPRG').split(' ')
        # 2>/dev/null swallowed by PIPE
        cmd= [  prg,
                self.fitsFile.fitsFileName, 
                '-c ',
                runTimeConfig.value('SEXCFG'),
                '-CATALOG_NAME',
                self.catalogueFileName,
                '-PARAMETERS_NAME',
                runTimeConfig.value('SEXREFERENCE_PARAM'),]
        try:
            output = subprocess.Popen( cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]

        except OSError as (errno, strerror):
            logging.error( 'extractToCatalogue: I/O error({0}): {1}'.format(errno, strerror))
            sys.exit(1)

        except:
            logging.error('extractToCatalogue: '+ repr(cmd) + ' died')
            sys.exit(1)



# checking against reference items see http://www.wellho.net/resources/ex.php4?item=y115/re1.py
    def createCatalogue(self):
        cat= open( self.catalogueFileName, 'r')
        lines= cat.readlines()
        # rely on the fact that first line is stored as first element in list
        # match, element, data
        ##   2 X_IMAGE                Object position along x                                    [pixel]
        #         1   2976.000      7.473     2773.781     219.8691     42.19405  -5.8554   0.2084     120.5605         6  26     4.05      5.330    0.823
 
        pElement = re.compile( r'#[ ]+([0-9]+)[ ]+([\w]+)')
        pData    = re.compile( r'')
        for (i, line) in enumerate(lines):
            element = pElement.match(line)
            data    = pData.match(line)
            if( element):
#                if(verbose):
#                    print "element.group(1) : ", element.group(1)
#                    print "element.group(2) : ", element.group(2)
                self.elements[element.group(1)]=element.group(2)

            elif( data):
#                if(verbose):
#                    print line

                items= line.split()
#                if(verbose):
#                    for item in items:
#                        print item

                for (i, item) in enumerate(items):
                    try:
                        self.catalogue[self.elements[str(i+1)]]= item
                    except:
                        print 'readCatalogue: exception %d' % i +  ' '+ self.elements[str(i+1)] + ' ' + item 
                        sys.exit(1)
            else:
                logger.error( 'Catalogue.readCatalogue: no match on line %d' % i)
                logger.error( 'Catalogue.readCatalogue: ' + line)

#            if(verbose):    
#                for item, value in  sorted(self.catalogue.iteritems()):
#                    print "item " + item + "value " + value

# example how to access the catalogue
    def average(self, variable):
        sum= 0
        i= 0
        for item, value in  sorted(self.catalogue.iteritems()):
            if( item == variable):
                sum += float(value)
                i += 1

        if(verbose):
            if( i != 0):
                print 'average %f' % (sum/ float(i))
            else:
                print 'Error in average i=0'

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

    def reference(self):
        for cat in self.CataloguesList:
            if( cat.isReference):
                if( verbose):
                    print 'Catalogues.reference: reference catalogue found for file: ' + cat.fitsFile.fitsFileName
                return cat

        if( verbose):
            print 'Catalogues: no reference fits file found'

        return False

    def validate(self):
        
        catr= self.reference()
        if( catr== None):
            return self.isValid

        if( len(self.CataloguesList) > 1):
            # default is False
            for cat in self.CataloguesList:
# dummy
                if( cat.isValid== catr.isValid):
                    if( verbose):
                        print 'Catalogues.validate: valid cat for: ' + cat.fitsFile.fitsFileName
                    cat.isValid= True

                if( verbose):
                    print "catalog set to valid again"
                    cat.isValid= True

                if(cat.isValid==False):
                    if( verbose):
                        print 'Catalogues.validate: removed cat for: ' + cat.fitsFile.fitsFileName
                    self.CataloguesList.remove(cat)

            if( len(self.CataloguesList) > 0):
                self.isValid= True

            return self.isValid

        return False
 

import sys
import pyfits

class FitsFile():
    """Class holding fits file name and ist properties"""
    def __init__(self, fitsFileName=None, filter=None):
        self.fitsFileName= fitsFileName
        self.filter= filter
        self.isValid= False

        FitsFile.__lt__ = lambda self, other: self.headerElements['FOC_POS'] < other.headerElements['FOC_POS']

    def isFilter(self):
        if( verbose):
            print "fits file path: " + self.fitsFileName
        try:
            hdulist = pyfits.fitsopen(self.fitsFileName)
        except:
            if( self.fitsFileName):
                logger.error('FitsFile: file not found : ' + self.fitsFileName)
            else:
                logger.error('FitsFile: file name not given (None), exiting')
            sys.exit(1)

        hdulist.close()

#        if( verbose):
#            hdulist.info()

        if( self.filter==hdulist[0].header['FILTER']):
            self.headerElements={}
            self.headerElements['FILTER'] = hdulist[0].header['FILTER']
            self.headerElements['FOC_POS']= hdulist[0].header['FOC_POS']
            self.headerElements['BINNING']= hdulist[0].header['BINNING']
            self.headerElements['NAXIS1'] = hdulist[0].header['NAXIS1']
            self.headerElements['NAXIS2'] = hdulist[0].header['NAXIS2']
            self.headerElements['ORIRA']  = hdulist[0].header['ORIRA']
            self.headerElements['ORIDEC'] = hdulist[0].header['ORIDEC']
            return True

        else:
            return False

class FitsFiles():
    """Class holding FitsFile"""
    def __init__(self):
        self.fitsFilesList= []
        self.isValid= False

    def append( self, FitsFile):
        self.fitsFilesList.append(FitsFile)

    def fitsFiles(self):
        return self.fitsFilesList

    def findReference(self):

        filter= runTimeConfig.filterByName('X')
        
        for hdu in sorted(self.fitsFilesList):
            if( filter.foc_pos < hdu.headerElements['FOC_POS']):
                print "FOUND ==============" + hdu.fitsFileName + "======== %d" % hdu.headerElements['FOC_POS']
                return hdu
        return False

    def isValid():
        return self.isValid

    def validate( elements):
        i= 0
        for hdu in self.fitsFilesList:
## elents should become a dict, compare values with values and count
            for element in elements:
                if( element== hdu.headerElements[element]):
                    i += 1

            if( i == self.headerElements.len()):
                hdu.isValid= True
                if( verbose):
                    print 'FitsFiles.validate: valid hdu: ' + hdu.fitsFileName
            else:
                try:
                    self.fitsFilesList.remove(hdu)
                except:
                    logger.error('FitsFiles.validate: could not remove hdu for ' + hdu.fitsFileName)
                if(verbose):
                    print "FitsFiles.validate: removed hdu: " + hdu.fitsFileName + "========== %d" %  self.fitsFilesList.index(hdu)


        if( len(self.fitsFilesList) > 0):
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

    def expandToTmp( self, fileName=None):
        if( fileName==None):
            logger.error('ServiceFileOperations.expandToTmp: no file name given')

        fileName= runTimeConfig.value('TEMP_DIRECTORY') + '/'+ fileName
        return fileName

    def expandToCat( self, fileName=None):
        if( fileName==None):
            logger.error('ServiceFileOperations.expandToCat: no file name given')

        items= fileName.split('/')[-1]
        fileName= self.prefix() + items.split('.fits')[0] + '-' + self.now + '.cat'
        return self.expandToTmp(fileName)

#    def expandToPath( self, fileName=None):
#        if( fileName==None):
#            logger.error('ServiceFileOperations.expandToCat: no file name given')
        

    def findFitsFiles( self):
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

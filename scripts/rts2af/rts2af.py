"""Class definitions for rts2af"""
# (C) 2010, Markus Wildi, markus.wildi@one-arcsec.org
#
#   
#   see man 1 rts2af.py
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
        

        parser = argparse.ArgumentParser(description='RTS2 autofocus', epilog="See man 1 rts2af.py for mor information")
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
            logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
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
        # If there is none, we have a serious problems anyway, properties will be overwritten
        self.ccd= CCD( name='CD', binning='1x1', windowOffsetX=-1, windowOffsetY=-1, windowHeight=-1, windowWidth=-1)
        self.cp={}
        self.config = ConfigParser.RawConfigParser()
        
        self.cp[('basic', 'CONFIGURATION_FILE')]= '/etc/rts2/rts2af/rts2af-acquire.cfg'
        self.cp[('basic', 'BASE_DIRECTORY')]= '/tmp/rts2af-focus'
        self.cp[('basic', 'TEMP_DIRECTORY')]= '/tmp/'
        self.cp[('basic', 'FILE_GLOB')]= '*fits'
        self.cp[('basic', 'FITS_IN_BASE_DIRECTORY')]= False
        self.cp[('basic', 'DEFAULT_FOC_POS')]= 3500
        # ToDo new, Docu
        self.cp[('basic', 'TEST_ACQUIRE')]= False
        self.cp[('basic', 'DEFAULT_EXPOSURE')]= 10
        #                                        name
        #                                           offset to clear path
        #                                                 relative lower acquisition limit [tick]
        #                                                        relative upper acquisition limit [tick]
        #                                                              stepsize [tick]
        #                                                                   exposure factor
        self.cp[('filter properties', 'f01')]= '[U, 2074, -1500, 1500, 100, 11.1]'
        self.cp[('filter properties', 'f02')]= '[B, 1712, -1500, 1500, 100, 2.5]'
        self.cp[('filter properties', 'f03')]= '[V, 1678, -1500, 1500, 100, 2.5]'
        self.cp[('filter properties', 'f04')]= '[R, 1700, -1500, 1500, 100, 2.5]'
        self.cp[('filter properties', 'f05')]= '[I, 1700, -1500, 1500, 100, 3.5]'
        self.cp[('filter properties', 'f06')]= '[X, 0, -1500, 1500, 100, 1.]'
        self.cp[('filter properties', 'f07')]= '[H, 1446, -1500, 1500, 100, 13.1]'
        self.cp[('filter properties', 'f08')]= '[b, 1446, -1500, 1500, 100, 1.]'
        self.cp[('filter properties', 'f09')]= '[c, 1446, -1500, 1500, 100, 1.]'
        self.cp[('filter properties', 'f0a')]= '[d, 1446, -1500, 1500, 100, 1.]'
        self.cp[('filter properties', 'f0b')]= '[e, 1446, -1500, 1500, 100, 1.]'
        self.cp[('filter properties', 'f0c')]= '[f, 1446, -1500, 1500, 100, 1.]'
        self.cp[('filter properties', 'f0d')]= '[g, 1446, -1500, 1500, 100, 1.]'
        self.cp[('filter properties', 'f0e')]= '[h, 1446, -1500, 1500, 100, 1.]'
        self.cp[('filter properties', 'nof')]= '[NOF, 0, -1500, 1500, 100, 1.]'
        

        self.cp[('focuser properties', 'FOCUSER_RESOLUTION')]= 20
        self.cp[('focuser properties', 'FOCUSER_ABSOLUTE_LOWER_LIMIT')]= 1501
        self.cp[('focuser properties', 'FOCUSER_ABSOLUTE_UPPER_LIMIT')]= 6002
        self.cp[('focuser properties', 'FOCUSER_SPEED')]= 100.
        self.cp[('focuser properties', 'FOCUSER_STEP_SIZE')]= 1.1428e-6
        self.cp[('focuser properties', 'FOCUSER_TEMPERATURE_COMPENSATION')]= False
        self.cp[('focuser properties', 'FOCUSER_SET_FOC_DEF_FWHM_UPPER_THRESHOLD')]= 5.

        self.cp[('acceptance circle', 'CENTER_OFFSET_X')]= 0.
        self.cp[('acceptance circle', 'CENTER_OFFSET_Y')]= 0.
        self.cp[('acceptance circle', 'RADIUS')]= 2000.
        
        self.cp[('filters', 'filters')]= 'U:B:V:R:I:X:Y'
        
        self.cp[('DS9', 'DS9_REFERENCE')]= True
        self.cp[('DS9', 'DS9_MATCHED')]= True
        self.cp[('DS9', 'DS9_ALL')]= True
        self.cp[('DS9', 'DS9_DISPLAY_ACCEPTANCE_AREA')]= True
        self.cp[('DS9', 'DS9_REGION_FILE')]= 'ds9-rts2af.reg'
        
        self.cp[('analysis', 'MINIMUM_OBJECTS')]= 20
        self.cp[('analysis', 'MINIMUM_FOCUSER_POSITIONS')]= 5
        self.cp[('analysis', 'FIT_RESULT_FILE')]= 'rts2af-fit.dat'
        self.cp[('analysis', 'MATCHED_RATIO')]= 0.1
        self.cp[('analysis', 'SET_FOC_DEF_FWHM_UPPER_THRESHOLD')]= 5.
        
        self.cp[('fitting', 'FITPRG')]= 'rts2af-fit-focus'
        self.cp[('fitting', 'DISPLAYFIT')]= True
        
        self.cp[('SExtractor', 'SEXPRG')]= 'sextractor 2>/dev/null'
        self.cp[('SExtractor', 'SEXCFG')]= '/etc/rts2/rts2af/sex/rts2af-sex.cfg'
        self.cp[('SExtractor', 'SEXPARAM')]= '/etc/rts2/rts2af/sex/rts2af-sex.param'
        self.cp[('SExtractor', 'SEXREFERENCE_PARAM')]= '/etc/rts2/rts2af/sex/rts2af-sex-reference.param'
        self.cp[('SExtractor', 'OBJECT_SEPARATION')]= 10.
        self.cp[('SExtractor', 'ELLIPTICITY')]= .1
        self.cp[('SExtractor', 'ELLIPTICITY_REFERENCE')]= .3
        self.cp[('SExtractor', 'SEXSKY_LIST')]= 'sex-assoc-sky.list'
        self.cp[('SExtractor', 'CLEANUP_REFERENCE_CATALOGUE')]= True
        # ToDo so far that is good for FLI CCD
        # These factors are used for the fitting
        self.cp[('ccd binning mapping', '1x1')] = 0
        self.cp[('ccd binning mapping', '2x2')] = 1
        self.cp[('ccd binning mapping', '4x4')] = 2

        self.cp[('ccd', 'NAME')]= 'CD'
        self.cp[('ccd', 'BINNING')]= '1x1'
        self.cp[('ccd', 'WINDOW')]= '-1 -1 -1 -1'
        self.cp[('ccd', 'PIXELSIZE')]= 9.e-6 # unit meter
        #ToDo: incorrect unit unit, use arcsec
        self.cp[('sky', 'SEEING')]= 27.e-6 # unit meter

        self.cp[('mode', 'SET_FOCUS')]= True
        self.cp[('mode', 'SET_INITIAL_FOC_DEF')]= False

        # mapping of fits header elements to canonical
        self.cp[('fits header mapping', 'AMBIENTTEMPERATURE')]= 'HIERARCH MET_DAV.DOME_TMP'
        self.cp[('fits header mapping', 'DATETIME')]= 'JD'
        self.cp[('fits header mapping', 'EXPOSURE')]= 'EXPOSURE'
        self.cp[('fits header mapping', 'CCD_TEMP')]= 'CCD_TEMP'
        self.cp[('fits header mapping', 'FOC_POS')] = 'FOC_POS'
        self.cp[('fits header mapping', 'DATE-OBS')]= 'DATE-OBS'

        self.cp[('telescope', 'TEL_RADIUS')] = 0.09 # [meter]
        self.cp[('telescope', 'TEL_FOCALLENGTH')] = 1.26 # [meter]
        

        self.cp[('queuing', 'USERNAME')] = 'rts2' # username password for rts2af-queue command, see postgres db
        self.cp[('queuing', 'PASSWORD')] = 'rts2'
        self.cp[('queuing', 'QUEUENAME')]= 'focusing'
        self.cp[('queuing', 'TARGETID')] = '5'
        self.cp[('queuing', 'THRESHOLD')] = 5.12

        self.defaults={}
        for (section, identifier), value in sorted(self.cp.iteritems()):
            self.defaults[(identifier)]= value
 

    def configurationFileName(self):
        return  self.defaults['CONFIGURATION_FILE']

    def configIdentifiers(self):
        return sorted(self.cp.iteritems())

    def defaultsIdentifiers(self):
        return sorted(self.defaults.iteritems())
        
    def dumpDefaults(self):
        for (identifier), value in self.configIdentifiers():
            print "dump defaults :", ', ', identifier, '=>', value

    def valuesIdentifiers(self):
        return sorted(self.values.iteritems())
    # why not values?
    #  runTimeConfig.values('SEXCFG'), TypeError: 'dict' object is not callable
    def value( self, identifier):
        return self.values[ identifier]

    def filterByName(self, name):
        for filter in  self.filters:
            #print "NAME>" + name + "<>" + filter.name
            if( name == filter.name):
                #print "NAME: {0} {1}".format(name, filter.name)
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
            configfile.write('# default configuration for rts2af.py\n')
            configfile.write('# generated with rts2af.py -p\n')
            configfile.write('# see man rts2af.py for details\n')
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
            configfile.write('# default configuration for rts2af.py\n')
            configfile.write('# generated with rts2af.py -p\n')
            configfile.write('# see man rts2af.py for details\n')
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
                continue
                #logging.info('Configuration.readConfiguration: no section ' +  section + ' or identifier ' +  identifier + ' in file ' + configFileName)
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
                    logging.error('Configuration.readConfiguration: no int '+ value+ ' in section ' +  section + ', identifier ' +  identifier + ' in file ' + configFileName)
                    
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
                        print '-----------filter properties---------:>' + items[0] + '<'
                    self.filters.append( Filter( items[0], string.atoi(items[1]), string.atoi(items[2]), string.atoi(items[3]), string.atoi(items[4]), string.atof(items[5])))
                elif( section == 'filters'):
                    items= value.split(':')
                    for item in items:
                        item.replace(' ', '')
                        if(verbose):
                            print 'filterInUse---------:>' + item + '<'
                        self.filtersInUse.append(item)

                elif( section == 'ccd'):

                    if(identifier=='NAME'):
                        self.ccd.name= value

                    elif(identifier=='WINDOW'):
                        items= re.split('[ ]+', value)
                        if( len(items) < 4):
                            logging.warn( 'Configuration.readConfiguration: too few ccd window items {0} {1}, using the whole CCD area'.format(len(items), value))
                            items= [ '-1', '-1', '-1', '-1']

                        for item in items:
                            item.replace(' ', '')
                            if(verbose):
                                print 'ccd window  item {0}'.format(item)
                
                        self.ccd.windowOffsetX=string.atoi(items[0])
                        self.ccd.windowOffsetY=string.atoi(items[1]) 
                        self.ccd.windowWidth  =string.atoi(items[2]) 
                        self.ccd.windowHeight =string.atoi(items[3])
                    
                    elif(identifier=='BINNING'):
                        self.ccd.binning= value

                    elif(identifier=='PIXELSIZE'): # unit meter
                        self.ccd.pixelSize= value

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
class CCD():
    """Class for CCD properties"""
    def __init__(self, name, binning=None, windowOffsetX=None, windowOffsetY=None, windowHeight=None, windowWidth=None, pixelSize=None):
        
        self.name= name
        self.binning=binning
        self.windowOffsetX=windowOffsetX
        self.windowOffsetY=windowOffsetY
        self.windowHeight=windowHeight
        self.windowWidth=windowWidth
        self.pixelSize= pixelSize

class Filter():
    """Class for filter properties"""
    # ToDo: clarify how  this class is used
    def __init__(self, name=None, OffsetToClearPath=None, lowerLimit=None, upperLimit=None, stepSize =None, exposureFactor=1.):
        self.name= name
        self.OffsetToClearPath= OffsetToClearPath# [tick]
        self.relativeLowerLimit= lowerLimit# [tick]
        self.relativeUpperLimit= upperLimit# [tick]
        self.exposureFactor   = exposureFactor 
        self.stepSize  = stepSize # [tick]
        self.offsets= range (self.relativeLowerLimit, int(self.relativeUpperLimit + self.stepSize),  self.stepSize)

class Focuser():
    """Class for focuser properties"""
    # ToDo: clarify how/when this class is instantiated
    def __init__(self, name=None, lowerLimit=None, upperLimit=None, resolution=None, speed=None, stepSize=None):
        self.name= name
        self.lowerLimit=lowerLimit 
        self.upperLimit=upperLimit 
        self.resolution=resolution 
        self.speed=speed 
        self.stepSize=stepSize 

class Telescope():
    """Class holding telescope properties"""
    # ToDo: clarify how/when this class is instantiated
    def __init__(self, radius=None, focalLength=None): # units [meter]

        if( not radius):
            self.radius= runTimeConfig.value('TEL_RADIUS')
        else:
            self.radius= radius

        if( not focalLength):
            self.focalLength= runTimeConfig.value('TEL_FOCALLENGTH')
        else:
            self.focalLength= focalLength

        try:
            self.focalratio = self.radius/self.focalLength
        except:
            logging.error( 'Telescope: no sensible focalLength: {0} given, assuming 1 meter'.format(self.focalLength))
            self.focalratio = self.radius/1000.

        #ToDo: fetch that from RTS2 CCD device
        self.pixelSize= runTimeConfig.value('PIXELSIZE') # 9.e-6 #[meter]
        # self.pixelSize= pixelSize 
        
        #self.seeing= 27.0e-6 #[meter!]
        self.seeing= runTimeConfig.value('SEEING') # [meter]

    def quadraticExposureTimeAtFocPos(self, exposureTimeZero=0, differencefoc_pos=0):
        return ( math.pow( self.starImageRadius(differencefoc_pos), 2) * exposureTimeZero)

    def linearExposureTimeAtFocPos(self, exposureTimeZero=0, differencefoc_pos=0):
        return  self.starImageRadius( differencefoc_pos) * exposureTimeZero

    def starImageRadius(self, differencefoc_pos=0):
        return (abs(differencefoc_pos)  * self.focalratio + self.seeing) / self.seeing

class FitResults(): 
    """Class holding fit results"""
    def __init__(self, averageFwhm=None):
        focPosS= sorted(averageFwhm)
        self.nrDatapoints= len(averageFwhm)
        self.focPosMin=focPosS[0]
        self.focPosMax=focPosS[-1]
        self.date=None
        self.dateEpoch=None
        self.temperature=None
        self.objects=None
        self.constants=[]
        self.fwhmMinimumFocPos=None
        self.fluxMaximumFocPos=None
        self.fwhmMinimum=None
        self.fluxMaximum=None
        self.referenceFileName=None
        self.error=False
        self.fwhmWithinBounds= False
        self.fluxWithinBounds= False
# To parse
#rts2af_fit.py: date: 2012-07-09T11:22:33
#rts2af_fit.py: temperature: 12.11C
#rts2af_fit.py: objects: 13
#rts2af_fit.py: FWHM_FOCUS 3399.8087883, FWHM at Minimum 2.34587076412
#rts2af_fit.py: FLUX_FOCUS 3414.79248047, FWHM at Maximum 11.8584192561  
#rts2af_fit.py: FWHM parameters: [  3.51004711e+02  -2.46516620e-01   5.45253555e-05  -3.58272548e-09]
#rts2af_fit.py: flux parameters: [  1.18584193e+01   3.41479252e+03   2.34838248e+00  -5.58647229e+01 -3.47256366e+07]

        fitResultFileName= serviceFileOp.expandToFitResultPath('rts2af-result-') 
        with open( fitResultFileName, 'r') as frfn:
            for line in frfn:
                line.strip()
                fitPrgMatch= re.search( runTimeConfig.value('FITPRG') + ':', line)
                if( fitPrgMatch==None):
                    continue
                
                dateMatch= re.search( runTimeConfig.value('FITPRG') + ':' + ' date: (.+)', line)
                if( not dateMatch==None):
                    self.date= dateMatch.group(1)
                    try:
                        self.dateEpoch= time.mktime(time.strptime( self.date, '%Y-%m-%dT%H:%M:%S'))
                    except ValueError:
                        try:
                            logging.info('DATE           {0}'.format(self.date))
                            # 2011-04-16-T19:26:36
                            self.dateEpoch= time.mktime(time.strptime( self.date, '%Y-%m-%d-T%H:%M:%S'))

                        except:
                            logging.error('fitResults: problems reading date: {0} %Y-%m-%dT%H:%M:%S, {1}'.format(self.date, line))
                    except:
                        logging.error('fitResults: problems reading date: {0} (hint: not a value error)'.format(self.date))
                    
                    continue

                temperatureMatch= re.search( runTimeConfig.value('FITPRG') + ':' + ' temperature: (.+)', line)
                if( not temperatureMatch==None):
                    self.temperature= temperatureMatch.group(1)
                    continue

                objectsMatch= re.search( runTimeConfig.value('FITPRG') + ':' + ' objects: (.+)', line)
                if( not objectsMatch==None):
                    self.objects= objectsMatch.group(1)
                    continue
# ToDo: complete parameters
#               parametersMatch= re.search( runTimeConfig.value('FITPRG') + ':' + ' result fwhm\w*: (.+)', line)
#               if( not parametersMatch==None):
#                   print 'found something'

                fwhmMatch= re.search( runTimeConfig.value('FITPRG') + ':' + ' FWHM_FOCUS ([\-\.0-9e+]+), FWHM at Minimum ([\-\.0-9e+]+)', line)
                if( not fwhmMatch==None):
                    self.fwhmMinimumFocPos= float(fwhmMatch.group(1))
                    self.fwhmMinimum= float(fwhmMatch.group(2))

                    if( self.focPosMin <= self.fwhmMinimumFocPos <= self.focPosMax):
                        self.fwhmWithinBounds= True
                    else:
                        self.error= True
                    continue

                fluxMatch= re.search( runTimeConfig.value('FITPRG') + ':' + ' FLUX_FOCUS ([\-\.0-9e+]+), FLUX at Maximum ([\-\.0-9e+]+)', line)
                if( not fluxMatch==None):
                    self.fluxMaximumFocPos= float(fluxMatch.group(1))
                    self.fluxMaximumF= float(fluxMatch.group(2))
                    if( self.focPosMin <= self.fluxMaximumFocPos  <= self.focPosMax):
                        self.fluxWithinBounds= True
                    else:
                        self.error= True
                    continue

            frfn.close()

# for now at least FWHM minimum must exist
        if( self.fwhmMinimumFocPos==None):
            self.error= True

       

class SXObject():
    """Class holding the used properties of SExtractor object"""
    def __init__(self, objectNumber=None, focusPosition=None, position=None, fwhm=None, flux=None, fluxError=None, associatedSXobject=None, separationOK=True, propertiesOK=True, acceptanceOK=True):
        self.objectNumber= objectNumber
        self.focusPosition= focusPosition
        self.position= position # is a list (x,y)
        self.matched= False
        self.fwhm= fwhm
        self.flux= flux
        self.fluxError= fluxError
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
import math

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
        # ToDo: might be broken
        Catalogue.__lt__ = lambda self, other: self.fitsHDU.variableHeaderElements['FOC_POS'] < other.fitsHDU.variableHeaderElements['FOC_POS']

    def runSExtractor(self):
        if( verbose):
            print 'sextractor  ' + runTimeConfig.value('SEXPRG')
            print 'sextractor  ' + runTimeConfig.value('SEXCFG')
            print 'sextractor  ' + runTimeConfig.value('SEXREFERENCE_PARAM')

        # 2>/dev/null swallowed by PIPE
        try:
            (prg, arg)= runTimeConfig.value('SEXPRG').split(' ')
        except:
            prg= runTimeConfig.value('SEXPRG')

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
        itemNrFLUX_APER    = self.SExtractorParams.assoc.index('FLUX_APER') 
        itemNrFLUXERR_APER= self.SExtractorParams.assoc.index('FLUXERR_APER') 
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

                if( self.fitsHDU.variableHeaderElements['EXPOSURE'] != 0):
                    # ToDo: FLUX_MAX, candidate: FLUX_APER
                    normalizedFlux= float(items[itemNrFLUX_MAX])/float(self.fitsHDU.variableHeaderElements['EXPOSURE'])
                    normalizedFluxError= float(items[ itemNrFLUXERR_APER])/float(self.fitsHDU.variableHeaderElements['EXPOSURE'])
                else:
                    normalizedFlux= float(items[itemNrFLUX_MAX])
                    normalizedFluxError= float(items[ itemNrFLUXERR_APER])
 
                self.sxObjects[sxObjectNumber]= SXObject(sxObjectNumber, self.fitsHDU.variableHeaderElements['FOC_POS'], (float(items[itemNrX_IMAGE]), float(items[itemNrY_IMAGE])), float(items[itemNrFWHM_IMAGE]), normalizedFlux, normalizedFluxError, items[itemNrASSOC_NUMBER]) # position, bool is used for clean up (default True, True)
                if( sxObjectNumberASSOC in self.multiplicity): # interesting
                    self.multiplicity[sxObjectNumberASSOC] += 1
                else:
                    self.multiplicity[sxObjectNumberASSOC]= 1
            else:
                logging.error( 'Catalogue.createCatalogue: no match on line %d' % lineNumber)
                logging.error( 'Catalogue.createCatalogue: ' + line)
                break

        else: # exhausted 
            logging.info( 'Catalogue.createCatalogue: catalogue created ' + self.fitsHDU.fitsFileName)
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
            ds9RegionFile.write("text (" + str(80) + "," + str(10) + ") # color=magenta font=\"helvetica 15 normal\" text = {FOC_POS="+ str(self.fitsHDU.variableHeaderElements['FOC_POS']) + "}\n")
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
                self.removeSXObject( sxObjectNumber)
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
                logging.error("matching: too few sxObjects matched %d" % matched + " of %d" % len(self.referenceCatalogue.sxObjects) + " required ratio is %f" % ( runTimeConfig.value('MATCHED_RATIO') * len(self.referenceCatalogue.sxObjects)) + " sxobjects at FOC_POS %d= " % self.fitsHDU.variableHeaderElements['FOC_POS'] + "file "+ self.fitsHDU.fitsFileName)
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
            #print 'average at FOC_POS: ' + str(self.fitsHDU.variableHeaderElements['FOC_POS']) + ' '+ variable + ' %f ' % (sum/ float(i)) 
            return (sum/ float(i))
        else:
            print 'average: Error in average i=0'
            return None
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
            print 'average %8s' %(selection) + ' at FOC_POS: ' + str(self.fitsHDU.variableHeaderElements['FOC_POS']) + ' FWHM  %5.2f ' % (sum/ float(i))  + ' number of objects %5d' % (i)
            return (sum/ float(i))
        else:
            print 'averageFWHM: Error in average i=0'
            return False

class ReferenceCatalogue(Catalogue):
    """Class for a catalogue (SExtractor result)"""
    def __init__(self, fitsHDU=None, SExtractorParams=None):
        self.fitsHDU  = fitsHDU
        self.catalogueFileName= serviceFileOp.expandToCat(self.fitsHDU)
        self.ds9RegionFileName= serviceFileOp.expandToDs9RegionFileName(self.fitsHDU)
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
        # ToDo: might be broken
        ReferenceCatalogue.__lt__ = lambda self, other: self.fitsHDU.variableHeaderElements['FOC_POS'] < other.fitsHDU.variableHeaderElements['FOC_POS']

    def sxObjectByNumber(self, sxObjectNumber):
        if(sxObjectNumber in self.sxObjects.keys()):
            return self.sxObjects[sxObjectNumber]
        return None

    def writeCatalogue(self):
        logging.info( 'ReferenceCatalogue.writeCatalogue: writing reference catalogue:{0}, for {1}'.format( self.skyList, self.fitsHDU.fitsFileName)) 

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

    def printSelectedSXobjects(self):
        for sxReferenceObjectNumber, sxReferenceObject in self.sxObjects.items():
            print "======== %5d %5d %7.1f %7.1f" %  (int(sxReferenceObjectNumber), sxReferenceObject.focusPosition, sxReferenceObject.position[0],sxReferenceObject.position[1])

    def runSExtractor(self):
        if( verbose):
            print 'sextractor  ' + runTimeConfig.value('SEXPRG')
            print 'sextractor  ' + runTimeConfig.value('SEXCFG')
            print 'sextractor  ' + runTimeConfig.value('SEXREFERENCE_PARAM')

        # 2>/dev/null swallowed by PIPE
        try:
            (prg, arg)= runTimeConfig.value('SEXPRG').split(' ')
        except:
            prg= runTimeConfig.value('SEXPRG')

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
            logging.error( 'ReferenceCatalogue.runSExtractor: I/O error({0}): {1}, {2}'.format(errno, strerror, repr(cmd)))
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
                # need for speed
                if( not self.checkAcceptanceByValue(float(items[itemNrX_IMAGE]), float(items[itemNrY_IMAGE]), self.circle)):
                    continue

                sxObjectNumber= items[0] # NUMBER
                
                for (j, item) in enumerate(items):
                    self.catalogue[(sxObjectNumber, self.SExtractorParams.reference[j])]=  float(item)

                    
                if( self.fitsHDU.variableHeaderElements['EXPOSURE'] != 0):
                    normalizedFlux= float(items[itemNrFLUX_MAX])/float(self.fitsHDU.variableHeaderElements['EXPOSURE'])
                else:
                    normalizedFlux= float(items[itemNrFLUX_MAX])

                self.sxObjects[sxObjectNumber]= SXReferenceObject(sxObjectNumber, self.fitsHDU.variableHeaderElements['FOC_POS'], (float(items[itemNrX_IMAGE]), float(items[itemNrY_IMAGE])), float(items[itemNrFWHM_IMAGE]), normalizedFlux) # position, bool is used for clean up (default True, True)
            else:
                logging.error( 'ReferenceCatalogue.createCatalogue: no match on line %d' % lineNumber)
                logging.error( 'ReferenceCatalogue.createCatalogue: ' + line)
                break

        else: # exhausted 
            if( self.numberReferenceObjects() > 0):
                logging.info( 'ReferenceCatalogue.createCatalogue: catalogue created {0} number of objects {1}'.format( self.fitsHDU.fitsFileName, self.numberReferenceObjects()))
                self.isValid= True
            else:
                logging.error( 'ReferenceCatalogue.createCatalogue: catalogue created {0} no objects found'.format( self.fitsHDU.fitsFileName))
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
        return self.checkAcceptanceByValue(float(sxObject.position[0]), float(sxObject.position[1]), circle)

    def checkAcceptanceByValue(self, x=None, y=None, circle=None):
        distance= math.sqrt(( float(x)- circle.transformedCenterX)**2 +(float(y)- circle.transformedCenterX)**2)
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

        if(not runTimeConfig.value('CLEANUP_REFERENCE_CATALOGUE')):
            logging.warn( 'ReferenceCatalogue.cleanUpReference: do NOT clean up as set in the configuration: {0}'.format(runTimeConfig.value('CLEANUP_REFERENCE_CATALOGUE'))) 
            return False

        if( not  self.isReference):
            logging.debug( 'ReferenceCatalogue.cleanUpReference: clean up only for a reference catalogue, I am: {0}'.format(self.fitsHDU.fitsFileName)) 
            return False
        else:
            logging.debug( 'ReferenceCatalogue.cleanUpReference: reference catalogue, I am: {0}'.format(self.fitsHDU.fitsFileName))
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

        return self.numberReferenceObjects() 

class AcceptanceRegion():
    """Class holding the properties of the acceptance circle, units are 1x1 pixel"""
    def __init__(self, fitsHDU=None, centerOffsetX=None, centerOffsetY=None, radius=None):
        self.fitsHDU= fitsHDU
        self.naxis1= 0
        self.naxis2= 0
        self.binning= fitsHDU.binning

        try:
            # if binning is used these values represent the rebinned pixels, e.g. 1x1 4096 and 2x2 2048
            self.naxis1 = float(fitsHDU.staticHeaderElements['NAXIS1'])
            self.naxis2 = float(fitsHDU.staticHeaderElements['NAXIS2'])
        except:
            logging.error("AcceptanceRegion.__init__: something went wrong here")

        if( centerOffsetX==None):
            self.centerOffsetX= float(runTimeConfig.value('CENTER_OFFSET_X'))/self.binning
        if( centerOffsetY==None):
            self.centerOffsetY= float(runTimeConfig.value('CENTER_OFFSET_Y'))/self.binning
        if( radius==None):
            self.radius = float(runTimeConfig.value('RADIUS'))/self.binning
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
        self.stdFwhm= {}
        self.averageFlux={}
        self.stdFlux={}
        self.referenceCatalogue= referenceCatalogue
        self.binning=self.referenceCatalogue.fitsHDU.binning
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
            logging.error('Catalogues.countObjectsFoundInAllFiles: too few sxObjects %d < %d' % (self.numberOfObjectsFoundInAllFiles, runTimeConfig.value('MINIMUM_OBJECTS')))
            return False
        else:
            return True

    def writeFitInputValues(self):
        discardedPositions= 0
        fitInput= open( self.dataFileNameFwhm, 'w')
        for focPos in sorted(self.averageFwhm):
            if(self.stdFwhm[focPos]> 0):
                line= "%06d %f %f %f\n" % ( focPos, self.averageFwhm[focPos] * self.binning, runTimeConfig.value('FOCUSER_RESOLUTION'), self.stdFwhm[focPos] * self.binning )
                fitInput.write(line)
            else:
                logging.error('writeFitInputValues: dropped focPos: {0}, due to standard deviation equals zero'.format(focPos))


        fitInput.close()
        fitInput= open( self.dataFileNameFlux, 'w')
        for focPos in sorted(self.averageFlux):
            line= "%06d %f %f %f\n" % ( focPos, self.averageFlux[focPos]/self.maxFlux * self.maxFwhm * self.binning, runTimeConfig.value('FOCUSER_RESOLUTION'), self.stdFlux[focPos]/self.maxFlux * self.maxFwhm * self.binning)
            fitInput.write(line)

        fitInput.close()
        return discardedPositions
    # ToDo: currently not used
    def findExtrema(self):

        b = dict(map(lambda item: (item[1],item[0]),self.averageFwhm.items()))
        try:
            minFocPos = b[min(b.keys())]
        except:
            logging.error('Catalogues.findExtrema: something wrong with self.averageFwhm, length: {0}, returning -1'.format(len(self.averageFwhm)))
            return -1

        # (inverse) weighted mean
        focPosS= self.averageFwhm.keys()
        tmpWeightS= self.averageFwhm.values()
        weightS=[]
        for val in tmpWeightS:
            if( val > 0.):
                weightS.append(1./val)
            else:
                break
        else:
            weightedMean= numpy.average(a=focPosS, axis=0, weights=weightS) 
            logging.info('Catalogues.findExtrema: minimum position: {0}, FWHM: {1}, weighted mean: {2}'.format(minFocPos, self.averageFwhm[minFocPos]* self.binning, weightedMean))

            return weightedMean

        logging.info('Catalogues.findExtrema: weighted mean failed, using minimum position: {0}, FWHM: {1}'.format(minFocPos, self.averageFwhm[minFocPos]* self.binning))

        return minFocPos


    def fitTheValues(self):
        if( not self.countObjectsFoundInAllFiles()):
            logging.error('Catalogues.fitTheValues: too few objects, do not fit')
            return None

        if( not self.__average__()):
            logging.error('fitTheValues: no average values')
            return None

        discardedPositions= self.writeFitInputValues()
            
        if((len(self.averageFwhm) + len(self.averageFlux)- discardedPositions)< 2 * runTimeConfig.value('MINIMUM_FOCUSER_POSITIONS')):
            logging.error('fitTheValues: too few (combined) focuser positions: {0} < {1}, continuing (see MINIMUM_FOCUSER_POSITIONS)'.format( (len(self.averageFwhm) + len(self.averageFlux)- discardedPositions), 2 * runTimeConfig.value('MINIMUM_FOCUSER_POSITIONS')))
            return None

        if(runTimeConfig.value('DISPLAYFIT')):
            display= '1'
        else:
            display= '0'

        try:
            cmd= [ runTimeConfig.value('FITPRG'),
                   display,
                   self.referenceCatalogue.fitsHDU.staticHeaderElements['FILTER'],
                   '{0:.2f}C'.format(self.referenceCatalogue.fitsHDU.variableHeaderElements['AMBIENTTEMPERATURE']),
                   serviceFileOp.runDateTime,
                   str(self.numberOfObjectsFoundInAllFiles),
                   self.dataFileNameFwhm,
                   self.dataFileNameFlux,
                   self.imageFilename]
        except:
            cmd= [ runTimeConfig.value('FITPRG'),
                   display,
                   self.referenceCatalogue.fitsHDU.staticHeaderElements['FILTER'],
                   'NoTemp',
                   serviceFileOp.runDateTime,
                   str(self.numberOfObjectsFoundInAllFiles),
                   self.dataFileNameFwhm,
                   self.dataFileNameFlux,
                   self.imageFilename]

        output = subprocess.Popen( cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()

        fitResultFileName= serviceFileOp.expandToFitResultPath('rts2af-result-') 

        with open( fitResultFileName, 'a') as frfn:
            for item in output:
                frfn.write(item)

            frfn.close()

        # parse the fit results
        fitResults= FitResults(self.averageFwhm)
        # ToDo: today only fwhm is used, combine it with flux in case they are closer than focuser resolution
        if( not fitResults.error):
            fitResults.referenceFileName=self.referenceCatalogue.fitsHDU.fitsFileName
            logging.info('Catalogues.fitTheValues: {0} {1} {2} {3} {4} {5} {6} {7} {8} {9}'.format(fitResults.date, fitResults.fwhmMinimumFocPos, fitResults.fwhmMinimum, fitResults.fwhmWithinBounds, fitResults.fluxMaximumFocPos, fitResults.fluxMaximum, fitResults.fluxWithinBounds, fitResults.temperature, fitResults.referenceFileName, fitResults.constants))

            return fitResults
        else:
            return None


    def __average__(self):
        numberOfObjects = 0
        if( self.averagesCalculated):
            return
        else:
            self.averagesCalculated=True
            
        fwhm= defaultdict(list)
        flux= defaultdict(list)
        fluxError= defaultdict(list)
        for sxReferenceObjectNumber, sxReferenceObject in self.referenceCatalogue.sxObjects.items():
            if(sxReferenceObject.numberOfMatches== len(self.CataloguesList)):
                sxReferenceObject.foundInAll= True
                numberOfObjects += 1
                i=0
                # check your input imhead *fits | grep FOC_POS
                for sxObject in sxReferenceObject.matchedsxObjects:
                    fwhm[sxObject.focusPosition].append(sxObject.fwhm)
                    flux[sxObject.focusPosition].append(sxObject.flux)

                    fluxError[sxObject.focusPosition].append(sxObject.fluxError/sxObject.flux)
                    i += 1

        fwhmList=[]
        fluxList=[]
        for focPos in fwhm:
            self.averageFwhm[focPos] = numpy.mean(fwhm[focPos], axis=0)
            self.stdFwhm[focPos] = numpy.std(fwhm[focPos], axis=0)
            fwhmList.append(self.averageFwhm[focPos])
                
            
        for focPos in flux:
            self.averageFlux[focPos] = numpy.mean(flux[focPos], axis=0)
            self.stdFlux[focPos] = numpy.std(fluxError[focPos], axis=0)
#
# ToDo: FLUX_MAX, FLUX_APER
# The standard deviation as above is meaningless
# Try to find a better error estimator
#            sumFlux= 0
#            for val in flux[focPos]:
#                sumFlux += val
#
#            self.averageFlux[focPos]=sumFlux
#
#            sumFluxError= 0
#            for val in fluxError[focPos]:
#                sumFluxError += math.pow(val, 2)
#
#            self.stdFlux[focPos] = math.sqrt(sumFluxError)
# 
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

        for cat in sorted(self.CataloguesList, key=lambda cat: cat.fitsHDU.variableHeaderElements['FOC_POS']):
            cat.ds9DisplayCatalogue("cyan")
            # if reference catr.ds9DisplayCatalogue("yellow", False)

    def ds9WriteRegionFiles(self):
        
        self.ds9Command= "ds9 -zoom to fit -scale mode zscale\\\n" 

        for cat in sorted(self.CataloguesList, key=lambda cat: cat.fitsHDU.variableHeaderElements['FOC_POS']):

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
            self.fitsFileName= serviceFileOp.expandToRunTimePath(fitsFileName) # can be None
             
        else:
            self.fitsFileName= fitsFileName

        self.referenceFitsHDU= referenceFitsHDU
        self.isValid= False
        self.staticHeaderElements={}
        self.variableHeaderElements={}
        #ToDo: generalize
        self.binning=-1
        self.assumedBinning=False
        # ToDo: might be broken
        FitsHDU.__lt__ = lambda self, other: self.variableHeaderElements['FOC_POS'] < other.variableHeaderElements['FOC_POS']

    def headerProperties(self):

        if( verbose):
            print "fits file path: " + self.fitsFileName
        try:
            fitsHDU = pyfits.open(self.fitsFileName)
        except:
            if( self.fitsFileName):
                logging.error('FitsHDU: file not found: {0}'.format( self.fitsFileName))
            else:
                logging.error('FitsHDU: file name not given (None)')
            return False

        fitsHDU.close()
        #
        # static header elements, must not vary within a focus run
        try:
            self.staticHeaderElements['FILTER']= fitsHDU[0].header['FILTER']
        except:
            self.isValid= True
            self.staticHeaderElements['FILTER']= 'NOF' 
            logging.info('headerProperties: fits file ' + self.fitsFileName + ' the filter header element not found, assuming filter NOF (no filter)')
        try:
            self.staticHeaderElements['BINNING']= fitsHDU[0].header['BINNING']
            if( self.staticHeaderElements['BINNING'] != runTimeConfig.value('BINNING')):
                logging.warn('headerProperties: fits file {0} the binning is different than in the run time configuration'.format( self.fitsFileName))
                
        except:
            self.isValid= True
            self.assumedBinning=True
            self.staticHeaderElements['BINNING']= '1x1'
            logging.info('headerProperties: fits file ' + self.fitsFileName + ' the binning header element not found, assuming 1x1 binning')
            # ToDo return False
        try:
            self.staticHeaderElements['ORIRA']  = fitsHDU[0].header['ORIRA']
            self.staticHeaderElements['ORIDEC'] = fitsHDU[0].header['ORIDEC']
        except:
            self.isValid= True
            logging.info('headerProperties: fits file ' + self.fitsFileName + ' the coordinates header elements: ORIRA, ORIDEC not found ')

        try:
            self.staticHeaderElements['NAXIS1'] = fitsHDU[0].header['NAXIS1']
            self.staticHeaderElements['NAXIS2'] = fitsHDU[0].header['NAXIS2']
        except:
            self.isValid= False
            logging.error('headerProperties: fits file ' + self.fitsFileName + ' the required header elements NAXIS1 or NAXIS2 not found')
            return False

        # variable header elements, may vary within a focus run
        try:
            self.variableHeaderElements['FOC_POS']= fitsHDU[0].header[runTimeConfig.value('FOC_POS')]
        except:
            self.isValid= False
            logging.error('headerProperties: fits file ' + self.fitsFileName + ' the required header elements FOC_POS not found')
            return False

        try:
            self.variableHeaderElements['DATE-OBS']= fitsHDU[0].header[runTimeConfig.value('DATE-OBS')]
        except:
            self.isValid= False
            logging.error('headerProperties: fits file ' + self.fitsFileName + ' the required header elements DATE-OBS not found')
            return False

        try:
            self.variableHeaderElements['EXPOSURE'] = fitsHDU[0].header[runTimeConfig.value('EXPOSURE')]
            self.variableHeaderElements['DATETIME'] = fitsHDU[0].header[runTimeConfig.value('DATETIME')]
            self.variableHeaderElements['CCD_TEMP'] = fitsHDU[0].header[runTimeConfig.value('CCD_TEMP')]
            self.variableHeaderElements['AMBIENTTEMPERATURE'] = fitsHDU[0].header[runTimeConfig.value('AMBIENTTEMPERATURE')]
        except:
            self.isValid= True
            logging.info('headerProperties: fits file ' + self.fitsFileName + ' the header elements: EXPOSURE, JD, CCD_TEMP or HIERARCH MET_AAG.TEMP_IRS not found')

        if( not self.extractBinning()):
            self.isValid= False
            return False
        
        # match the header elements with reference HDU
        if( self.referenceFitsHDU != None):
            if( self.matchStaticHeaderElements(fitsHDU)):
                self.isValid= True
                return True
            else:
                self.isValid= False
                return False
        else:
            self.isValid= True
            return True
            
    def matchStaticHeaderElements(self, fitsHDU):
        keys= self.referenceFitsHDU.staticHeaderElements.keys()
        i= 0
        for key in keys:
            if( self.assumedBinning):
                if(key == 'BINNING'):
                    continue
            if(self.referenceFitsHDU.staticHeaderElements[key]!= fitsHDU[0].header[key]):
                break
            else:
                self.staticHeaderElements[key] = fitsHDU[0].header[key]

        else: # exhausted
            self.isValid= True
            if( verbose):
                print 'headerProperties: header ok : ' + self.fitsFileName

            return True
        
        logging.error("headerProperties: fits file " + self.fitsFileName + " has different header properties")
        return False

    def extractBinning(self):
        #ToDo: clumsy, try ==

        pbinning1x1 = re.compile( r'1x1')
        binning1x1 = pbinning1x1.match(self.staticHeaderElements['BINNING'])
        pbinning2x2 = re.compile( r'2x2')
        binning2x2 = pbinning2x2.match(self.staticHeaderElements['BINNING'])
        pbinning4x4 = re.compile( r'4x4')
        binning4x4 = pbinning2x2.match(self.staticHeaderElements['BINNING'])

        if( binning1x1):
            self.binning= 1
        elif(binning2x2):
            self.binning= 2
        elif(binning4x4):
            self.binning= 4
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
            
    def findReference(self):
        if( self.referenceHDU !=None):
            return self.referenceHDU
        else:
            return False
    def findReferenceByFocPos(self, name):
        filter=  runTimeConfig.filterByName(name)
#        for hdu in sorted(self.fitsHDUsList):
        for hdu in sorted(self.fitsHDUsList):
            if( filter.OffsetToClearPath < hdu.variableHeaderElements['FOC_POS']):
                if(verbose):
                    print "FOUND reference by foc pos ==============" + hdu.fitsFileName + "======== %d" % hdu.variableHeaderElements['FOC_POS']
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

        # static header elements
        keys= hdur.staticHeaderElements.keys()
        for hdu in self.fitsHDUsList:
            for key in keys:
                if( hdur.staticHeaderElements[key]!= hdu.staticHeaderElements[key]):
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

        # ToDo: check that, seems to be always valid (that is not wrong but superfluous)
        # variable header elements
        keys= hdur.variableHeaderElements.keys()
        differentFocuserPositions={}
        for hdu in self.fitsHDUsList:
            for key in keys:
                if(key == 'FOC_POS'):
                    differentFocuserPositions[str(hdu.variableHeaderElements['FOC_POS'])]= 1 # means nothing
                    continue
            else: # exhausted
                hdu.isValid = hdu.isValid and True # 
                if( verbose):
                    print 'FitsHDUs.validate: valid hdu: ' + hdu.fitsFileName

        self.numberOfFocusPositions= len(differentFocuserPositions.keys())

        if( self.numberOfFocusPositions >= runTimeConfig.value('MINIMUM_FOCUSER_POSITIONS')):
            if( len(self.fitsHDUsList) > 0):
                logging.info('FitsHDUs.validate: hdus are valid')
                self.isValid= True
        else:
            logging.error('FitsHDUs.validate: hdus are invalid due too few focuser positions %d required are %d' % (self.numberOfFocusPositions, runTimeConfig.value('MINIMUM_FOCUSER_POSITIONS')))

        # check if binning is the same
        bin0= self.fitsHDUsList[0].staticHeaderElements['BINNING']
        for hdu in self.fitsHDUsList:

            if( bin0 == hdu.staticHeaderElements['BINNING']):
                logging.info('FitsHDUs.validate: binning {0}'.format(hdu.staticHeaderElements['BINNING']))                
            else:
                logging.warn('FitsHDUs.validate: different binning {0}, configuration: {1}'.format( hdu.staticHeaderElements['BINNING']))
                self.isValid= False
                break
        else:
            logging.info('FitsHDUs.validate: binning {0} for all hdus ok'.format( bin0))                
            self.isValid= self.isValid and True

        return self.isValid

    def countFocuserPositions(self):
        differentFocuserPositions={}
        for hdu in self.fitsHDUsList:
            differentFocuserPositions(hdu.variableHeaderElements['FOC_POS'])

        return len(differentFocuserPositions)

import time
import datetime
import glob
import os

class ServiceFileOperations():
    """Class performing various task on files, e.g. expansion to (full) path"""
    def __init__(self, runTimePath=None):
        self.now= datetime.datetime.now().isoformat()
        self.runTimePath= runTimePath
        self.runDateTime=None

    def prefix( self, fileName):
        return 'rts2af-' +fileName

    def notFits(self, fileName):
        items= fileName.split('/')[-1]
        return items.split('.fits')[0]

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
            fileName= self.prefix( items[0] +  '-' + fitsHDU.staticHeaderElements['FILTER'] + '-' +   self.now + '.' + items[1])
        except:
            fileName= self.prefix( items[0] + '-' +   self.now + '.' + items[1])
            
        if(verbose):
            print 'ServiceFileOperations:expandToSkyList expanded to ' + fileName
        
        return  self.expandToTmp(fileName)

    def expandToCat( self, fitsHDU=None):
        if( fitsHDU==None):
            logging.error('ServiceFileOperations.expandToCat: no hdu given')
        try:
            fileName= self.prefix( self.notFits(fitsHDU.fitsFileName) + '-' + fitsHDU.staticHeaderElements['FILTER'] + '-' + self.now + '.cat')
        except:
            fileName= self.prefix( self.notFits(fitsHDU.fitsFileName) + '-' + self.now + '.cat')

        return self.expandToTmp(fileName)

    def expandToFitInput(self, fitsHDU=None, element=None):
        items= runTimeConfig.value('FIT_RESULT_FILE').split('.')
        if((fitsHDU==None) or ( element==None)):
            logging.error('ServiceFileOperations.expandToFitInput: no hdu or elementgiven')

        fileName= items[0] + "-" + fitsHDU.staticHeaderElements['FILTER'] + "-" + self.now +  "-" + element + "." + items[1]
        return self.expandToTmp(self.prefix(fileName))

    def expandToFitImage(self, fitsHDU=None):
        if( fitsHDU==None):
            logging.error('ServiceFileOperations.expandToFitImage: no hdu given')

        items= runTimeConfig.value('FIT_RESULT_FILE').split('.') 
# ToDo png
        fileName= items[0] + "-" + fitsHDU.staticHeaderElements['FILTER'] + "-" + self.now + ".png"
        return self.expandToTmp(self.prefix(fileName))

    def absolutePath(self, fileName=None):
        if( fileName==None):
            logging.error('ServiceFileOperations.absolutePath: no file name given')
            
        pLeadingSlash = re.compile( r'\/.*')
        leadingSlash = pLeadingSlash.match(fileName)
        if( leadingSlash):
            return True
        else:
            return False

    def defineRunTimePath(self, fileName=None):
        for root, dirs, names in os.walk(runTimeConfig.value('BASE_DIRECTORY')):
            if( fileName.rstrip() in names):
                self.runTimePath= root
                return True
        else:
            logging.error('ServiceFileOperations.defineRunTimePath: file not found: {0}'.format(fileName))

        return False

    def expandToRunTimePath(self, pathName=None):
        if( self.absolutePath(pathName)):
            self.runDateTime= pathName.split('/')[-3] # it is rts2af, which creates the directory tree
            return pathName
        else:
            fileName= pathName.split('/')[-1]
            if( self.defineRunTimePath( fileName)):
                self.runDateTime= self.runTimePath.split('/')[-2]
 
                return self.runTimePath + '/' + fileName
            else:
                return None

# ToDo: refactor with expandToSkyList
    def expandToDs9RegionFileName( self, fitsHDU=None):
        if( fitsHDU==None):
            logging.error('ServiceFileOperations.expandToDs9RegionFileName: no hdu given')
        
        
        items= runTimeConfig.value('DS9_REGION_FILE').split('.')
        # not nice
        names= fitsHDU.fitsFileName.split('/')
        try:
            fileName= self.prefix( items[0] +  '-' + fitsHDU.staticHeaderElements['FILTER'] + '-' + self.now + '-' + names[-1] + '-' + str(fitsHDU.variableHeaderElements['FOC_POS']) + '.' + items[1])
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
            fileName= self.prefix( items[0] +  '-' + fitsHDU.staticHeaderElements['FILTER'] + '-' + self.now + '.' + items[1] + '.sh')
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

    def createAcquisitionBasePath(self, filter=None):
        os.makedirs( self.expandToAcquisitionBasePath( filter))
        
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



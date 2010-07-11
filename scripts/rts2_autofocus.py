#!/usr/bin/python
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

import ConfigParser
import argparse
import io
import os
import re
import shutil
import string
import sys
import time
import numpy
import pyfits

class defaultConfiguration:
    """default configuration"""
    
    def __init__(self, fileName='rts2-autofocus-offline.cfg'):
        self.configFileName = fileName

        self.values={}
        self.defaults={}

        self.config = ConfigParser.RawConfigParser()
        
        self.values[('basic', 'CONFIGURATION_FILE')]= '/etc/rts2/autofocus/rts2-autofocus.cfg'
        
        self.values[('basic', 'BASE_DIRECTORY')]= '/tmp'
        self.values[('basic', 'FITS_IN_BASE_DIRECTORY')]= False
        self.values[('basic', 'CCD_CAMERA')]= 'CD'
        self.values[('basic', 'CHECK_RTS2_CONFIGURATION')]= False

        self.values[('filter properties', 'U')]= '[0, 5074, -1500, 1500, 100, 40]'
        self.values[('filter properties', 'B')]= '[1, 4712, -1500, 1500, 100, 30]'
        self.values[('filter properties', 'V')]= '[2, 4678, -1500, 1500, 100, 20]'
        self.values[('filter properties', 'R')]= '[4, 4700, -1500, 1500, 100, 20]'
        self.values[('filter properties', 'I')]= '[4, 4700, -1500, 1500, 100, 20]'
        self.values[('filter properties', 'X')]= '[5, 3270, -1500, 1500, 100, 10]'
        self.values[('filter properties', 'Y')]= '[6, 3446, -1500, 1500, 100, 10]'
        self.values[('filter properties', 'NOFILTER')]= '[6, 3446, -1500, 1500, 109, 19]'
        
        self.values[('focuser properties', 'FOCUSER_RESOLUTION')]= 20
        self.values[('focuser properties', 'FOCUSER_ABSOLUTE_LOWER_LIMIT')]= 1501
        self.values[('focuser properties', 'FOCUSER_ABSOLUTE_UPPER_LIMIT')]= 6002

        self.values[('acceptance circle', 'CENTER_OFFSET_X')]= 0
        self.values[('acceptance circle', 'CENTER_OFFSET_Y')]= 0
        self.values[('acceptance circle', 'RADIUS')]= 2000.
        
        self.values[('filters', 'filters')]= 'U:B:V:R:I:X:Y'
        
        self.values[('DS9', 'DS9_REFERENCE')]= True
        self.values[('DS9', 'DS9_MATCHED')]= True
        self.values[('DS9', 'DS9_ALL')]= True
        self.values[('DS9', 'DS9_DISPLAY_ACCEPTANCE_AREA')]= True
        self.values[('DS9', 'DS9_REGION_FILE')]= '/tmp/ds9-autofocus.reg'
        
        self.values[('analysis', 'ANALYSIS_UPPER_LIMIT')]= 1.e12
        self.values[('analysis', 'ANALYSIS_LOWER_LIMIT')]= 0.
        self.values[('analysis', 'MINIMUM_OBJECTS')]= 5
        self.values[('analysis', 'INCLUDE_AUTO_FOCUS_RUN')]= False
        self.values[('analysis', 'SET_LIMITS_ON_SANITY_CHECKS')]= True
        self.values[('analysis', 'SET_LIMITS_ON_FILTER_FOCUS')]= True
        self.values[('analysis', 'FIT_RESULT_FILE')]= '/tmp/fit-autofocus.dat'
        
        self.values[('fitting', 'FOCROOT')]= 'rts2-fit-focus'
        
        self.values[('SExtractor', 'SEXPRG')]= 'sex 2>/dev/null'
        self.values[('SExtractor', 'SEXCFG')]= '/etc/rts2/autofocus/sex-autofocus.cfg'
        self.values[('SExtractor', 'SEXPARAM')]= '/etc/rts2/autofocus/sex-autofocus.param'
        self.values[('SExtractor', 'SEXREFERENCE_PARAM')]= '/etc/rts2/autofocus/sex-autofocus-reference.param'
        self.values[('SExtractor', 'OBJECT_SEPARATION')]= 10.
        self.values[('SExtractor', 'ELLIPTICITY')]= .1
        self.values[('SExtractor', 'ELLIPTICITY_REFERENCE')]= .1
        self.values[('SExtractor', 'SEXSKY_LIST')]= '/tmp/sex-autofocus-assoc-sky.list'
        self.values[('SExtractor', 'SEXCATALOGUE')]= '/tmp/sex-autofocus.cat'
        self.values[('SExtractor', 'SEX_TMP_CATALOGUE')]= '/tmp/sex-autofocus-tmp.cat'
        self.values[('SExtractor', 'CLEANUP_REFERENCE_CATALOGUE')]= True
        
        self.values[('mode', 'TAKE_DATA')]= True
        self.values[('mode', 'SET_FOCUS')]= True
        self.values[('mode', 'CCD_BINNING')]= 0
        self.values[('mode', 'AUTO_FOCUS')]= False
        self.values[('mode', 'NUMBER_OF_AUTO_FOCUS_IMAGES')]= 10
        
        self.values[('rts2', 'RTS2_DEVICES')]= '/etc/rts2/devices'
        
        for (section, identifier), value in sorted(self.values.iteritems()):
            self.defaults[(identifier)]= value
#            print 'value ', self.defaults[(identifier)]
 

    def configurationFileName(self):
        return self.configFileName

    def identifiers(self):
        return sorted(self.defaults.iteritems())

    def configIdentifiers(self):
        return sorted(self.values.iteritems())
        
    def defaultValue( self, identifier):
        return self.defaults[ identifier]

    def writeDefaultConfiguration(self):
        for (section, identifier), value in sorted(self.values.iteritems()):
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


class Script:
    def __init__(self, scriptName):
        self.scriptName= scriptName


class main(Script):
    """define the focus from a series of images"""
    def __init__(self, scriptName='main' ):
        self.scriptName= scriptName

    def main(self):

        dc= defaultConfiguration()
        parser = argparse.ArgumentParser(description='RTS2 autofocus', epilog="See man 1 rts2-autofocus.py for mor information")
#        parser.add_argument(
#            '--write', action='store', metavar='FILE NAME', 
#            type=argparse.FileType('w'), 
#            default=sys.stdout,
#            help='the file name where the default configuration will be written default: write to stdout')

        parser.add_argument(
            '-w', '--write', action='store_true', 
            help='write defaults to configuration file ' + dc.configurationFileName())

        parser.add_argument('--config', dest='fileName',
                            metavar='CONFIG FILE', nargs=1, type=str,
                            help='configuration file')

#        parser.add_argument('-t', '--test', dest='test', action='store', 
#                            metavar='TEST', nargs=1,
#    no default means None                        default='myTEST',
#                            help=' test case, default: myTEST')

        parser.add_argument('-v', dest='verbose', action='store_true')

        args = parser.parse_args()

        configFileName=''
        if( args.fileName):
            configFileName= args.fileName[0]  
        else:
            configFileName= dc.configurationFileName()
            cmd= 'logger no config file specified, taking default' + configFileName
            print cmd 
            os.system( cmd)

        if(args.verbose):
            print 'config file, taking ' + configFileName
        
        if(args.verbose):
            for (identifier), value in dc.identifiers():
                print "dump defaults :", ', ', identifier, '=>', value

        config = ConfigParser.ConfigParser()
        try:
            config.readfp(open(configFileName))
        except:
            cmd= 'logger config file ' + configFileName + ' not found'
            print cmd
            os.system( cmd)
            sys.exit(1)

# read the defaults
        values={}
        for (section, identifier), value in dc.configIdentifiers():
            values[identifier]= value

# over write the defaults
        for (section, identifier), xvalue in dc.configIdentifiers():

            try:
                value = config.get( section, identifier)
            except:
                cmd= 'logger no section ' +  section + ' or identifier ' +  identifier + ' in file ' + configFileName
                os.system( cmd)
# first bool, then int !
            if(isinstance(values[identifier], bool)):
#ToDO, looking for a direct way
                if( value == 'True'):
                    values[identifier]= True
                else:
                    values[identifier]= False
            elif( isinstance(values[identifier], int)):
                try:
                    values[identifier]= int(value)
                except:
                    cmd= 'logger no int '+ value+ 'in section ' +  section + ', identifier ' +  identifier + ' in file ' + configFileName
                    os.system( cmd)

            elif(isinstance(values[identifier], float)):
                try:
                    values[identifier]= float(value)
                except:
                    cmd= 'logger no float '+ value+ 'in section ' +  section + ', identifier ' +  identifier + ' in file ' + configFileName
                    os.system( cmd)

            else:
                values[identifier]= value
 
        if(args.verbose):
            for (identifier), value in dc.identifiers():
                print "over ", identifier, values[(identifier)]


if __name__ == '__main__':
    main().main()




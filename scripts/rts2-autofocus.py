#!/usr/bin/python
# (C) 2010, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rts-autofocus.py.py 
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

__author__ = 'markus.wildi@one-arcsec.org'

import ConfigParser
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
    
    def __init__(self, fileName='rts2-autofocus.cfgX'):
        self.configFileName = fileName

        self.values={}
        self.dalues={}

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
        
    def configurationFileName( self):
        return self.configFileName
        
    def defaultValue( self, identifier):
        return self.dalues[ identifier]

    def writeDefaultConfiguration(self):
        for (section, identifier), value in sorted(self.values.iteritems()):
            print section, '=>', identifier, '=>', value
            if( self.config.has_section(section)== False):
                self.config.add_section(section)

            self.config.set(section, identifier, value)
            self.dalues[identifier]= value

        with open( self.configFileName, 'wb') as configfile:
            configfile.write('# 2010-07-10, Markus Wildi\n')
            configfile.write('# default configuration for rts2-autofocus.py\n')
            configfile.write('# generated with rts2-autofous.py -p\n')
            configfile.write('# see man rts2-autofocus.py for details\n')
            configfile.write('#\n')
            configfile.write('#\n')
            self.config.write(configfile)
    


class rts2AutofocusScript:
    """define the focus from a series of images"""
#    def __init__(self):

    def main(self):
        if len(sys.argv) == 1:
            print 'Usage: %s  <configuration file>' % (sys.argv[0])
            os.system("logger %s Usage: %s configuration file" % (sys.argv[0], sys.argv[0]))
            sys.exit(1)

        dc= defaultConfiguration()
        dc.writeDefaultConfiguration()
 
        print dc.defaultValue('SEX_TMP_CATALOGUE')
        

        configFN= dc.configurationFileName()  
        config = ConfigParser.SafeConfigParser({'bar': 'Life', 'baz': 'hard'})
        config.read(configFN)
        
#        for (section, identifier), value in sorted(myValues.values.iteritems()):
#            print section, '=>', identifier, '=>', value


#        for (section, identifier), value in sorted(values.iteritems()):
#            print config.get( section, value)

if __name__ == '__main__':
    rts2AutofocusScript().main()

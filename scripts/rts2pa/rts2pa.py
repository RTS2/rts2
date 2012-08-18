"""Class definitions for rts2pa"""

# (C) 2004-2012, Markus Wildi, wildi.markus@bluewin.ch
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

__author__ = 'wildi.markus@bluewin.ch'


import sys
import logging
from optparse import OptionParser
import ConfigParser
import string
import shutil
import copy

class Configuration:
    """Configuration for any PAScript"""    
    def __init__(self, cfn=None):
        self.cfn= cfn
        self.values={}
        self.cp = ConfigParser.RawConfigParser()

        self.cp.add_section('basic')
        self.cp.set('basic', 'CONFIGURATION_FILE','/etc/rts2/rts2pa/rts2pa.cfg')
        self.cp.set('basic', 'BASE_DIRECTORY','/tmp/rts2pa')
        self.cp.set('basic', 'TEMP_DIRECTORY','/tmp/')
        self.cp.set('basic', 'TEST', 'True')
        self.cp.set('basic', 'TEST_FIELDS','./images/CNP-02-10-00-89-00-00.fits,./images/CNP-02-10-30-89-00-20.fits')

        self.cp.add_section('data_taking')
        self.cp.set('data_taking', 'EXPOSURE_TIME', 10.)  #[sec]
        self.cp.set('data_taking', 'DURATION', 1800.)    #[sec]
        self.cp.set('data_taking', 'SLEEP', 1800.)    #[sec]

        self.cp.add_section('astrometry')
#        self.cp.set('astrometry', 'ASTROMETRY_PRG','solve-field'
        self.cp.set('astrometry', 'RADIUS', 5.) #[deg]
        self.cp.set('astrometry', 'REPLACE', 'False')
        self.cp.set('astrometry', 'VERBOSE', 'False')

        self.cp.add_section('mode')
        self.cp.set('mode', 'mode','KingA')

        self.cp.add_section('coordinates')
        self.cp.set('coordinates', 'HA', 7.5) # hour angle [deg]
        self.cp.set('coordinates', 'PD', 1.) # polar distance [deg]

        self.cp.add_section('ccd')
        self.cp.set('ccd', 'ARCSEC_PER_PIX', 2.0)

        self.cp.add_section('fits-header')
        self.cp.set('fits-header', 'JD','JD')
        self.cp.set('fits-header', 'DATE-OBS','DATE-OBS') 

        self.cp.set('fits-header', 'ORIRA','ORIRA')
        self.cp.set('fits-header', 'ORIDEC','ORIDEC') 

        self.cp.set('fits-header', 'SITE-LON','SITE-LON') 
        self.cp.set('fits-header', 'SITE-LAT','SITE-LAT')
        if cfn:
            if not os.path.isfile( cfn):
                logging.error('config file: {0} does not exist, exiting'.format(cfn))
                sys.exit(1)
            self.readConfiguration()

    def dumpDefaultConfiguration(self):
        print '# 2012-07-27, Markus Wildi'
        print '# default configuration for rts2pa.py'
        print '#'
        print '#'
        print self.cp.write(sys.stdout)
            
        print '# above None no idea where it comes from'


    def readConfiguration( self):
        try:
            self.cp.readfp(open(self.cfn))
        except:
            logging.error('Configuration.readConfiguration: config file {0}  has wrong syntax, exiting'.format(self.cfn))
            sys.exit(1)

    def dumpConfigurationDifference(self, defaultConf=None):
        for sec in self.cp.sections():
            print ''
            for opt,val in self.cp.items(sec):
                if str(val) != str(defaultConf.cp.get(sec, opt)):
                    print '{} {}  actual :{}'.format(sec, opt, str(val))
                    print '{} {}  default:{}'.format(sec, opt, str(defaultConf.cp.get(sec, opt)))

    def dumpConfiguration(self):
        for sec in self.cp.sections():
            print ''
            for opt,val in self.cp.items(sec):
                print '{} {} {}'.format(sec, opt, str(val))

import time
import datetime
import os

class Environment():
    """Class performing various task on files, e.g. expansion to (full) path"""
    def __init__(self, rtc=None, logger=None):
        self.rtc=rtc
        self.now= datetime.datetime.now().isoformat()
        self.logger= logger
        self.runTimePath= '{0}/{1}/'.format(self.rtc.cp.get('basic','BASE_DIRECTORY'), self.now) 
        if not os.path.isdir(self.runTimePath):
            os.makedirs( self.runTimePath)
            logger.debug('Environment: directory: {0} created'.format(self.runTimePath))

    def prefix( self, fileName):
        return 'rts2pa-' +fileName

    def expandToTmp( self, fileName=None):
        if os.path.isabs(fileName):
            return fileName

        fileName= self.rtc.value('TEMP_DIRECTORY') + '/'+ fileName
        return fileName

    def moveToRunTimePath(self, pathName=None):
        fn= pathName.split('/')[-1]
        dst= '{0}{1}'.format(self.runTimePath, fn)
        try:
            shutil.move(pathName, dst)
            return dst
        except:
            self.logger('Environment: could not move file {0} to {1}'.format(fn, dst))
            return None


class PAScript:
    """Class for any PA script"""
    def __init__(self, scriptName=None, parser=None):
        self.scriptName= scriptName
        if parser:
            self.parser=parser
        else:
            self.parser= OptionParser()

        self.parser.add_option('--config',help='configuration file name',metavar='CONFIGFILE', nargs=1, type=str, dest='config', default='/etc/rts2/rts2pa/rts2pa.cfg')
        self.parser.add_option('--loglevel',help='log level: usual levels',metavar='LOG_LEVEL', nargs=1, type=str, dest='level', default='DEBUG')
        self.parser.add_option('--logTo',help='log file: filename or - for stdout',metavar='DESTINATION', nargs=1, type=str, dest='logTo', default='/var/log/rts2-debug')
        self.parser.add_option('--verbose',help='verbose output',metavar='', action='store_true', dest='verbose', default=False)
        self.parser.add_option('--dump',help='dump default configuration to stdout',metavar='DEFAULTS', action='store_true', dest='dump', default=False)
#        self.parser.add_option('--',help='',metavar='', nargs=1, type=str, dest='', default='')

        (options,args) = self.parser.parse_args()

        if options.dump:
            Configuration().dumpDefaultConfiguration()
            sys.exit(1)
        try:
            fn = open(options.logTo, 'a')
            fn.close()
        except IOError:
            sys.exit( 'Unable to write to logfile {0}'.format(options.logTo))

        logformat= '%(asctime)s:%(name)s:%(levelname)s:%(message)s'
        try:
            logging.basicConfig(filename=options.logTo, level=options.level.upper(), format= logformat)
            self.logger = logging.getLogger()
        except:
            print 'no log level {0}, exiting'.format(options.level)
            sys.exit(1)

        if(options.logTo== '-'):
            soh = logging.StreamHandler(sys.stdout)
            soh.setLevel(options.level.upper())
            soh.setFormatter(logging.Formatter(logformat))
            self.logger = logging.getLogger()
            self.logger.addHandler(soh)

        self.rtc= Configuration( cfn=options.config) #default configuration, overwrite default values with the ones form options.config
        
        self.env= Environment(rtc=self.rtc, logger=self.logger)

        if self.rtc.cp.get('basic','TEST'):        
            dfcp = type('Configuration', (object, ), dict(Configuration.__dict__))
            dfcp = Configuration()

        if options.verbose:
            self.env.rtc.dumpConfigurationDifference(defaultConf=dfcp)
            self.env.rtc.dumpConfiguration()


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

__author__ = 'wildi.markus@bluewin.ch'

import sys
import time
from rts2.json import JSONProxy


class Filter():
    """Class for filter properties"""
    def __init__(self, name=None, OffsetToClearPath=None, lowerLimit=None, upperLimit=None, stepSize =None, exposureFactor=1.):
        self.name= name
        self.OffsetToClearPath= OffsetToClearPath# [tick]
        self.relativeLowerLimit= lowerLimit# [tick]
        self.relativeUpperLimit= upperLimit# [tick]
        self.exposureFactor   = exposureFactor 
        self.stepSize  = stepSize # [tick]
        self.offsets= range (self.relativeLowerLimit, int(self.relativeUpperLimit + self.stepSize),  self.stepSize)

class FilterWheel():
    """Class for filter wheel properties"""
    def __init__(self, name=None, filters=list()):
        self.name= name
        self.filters=filters
        self.clearPaths=None

class Focuser():
    """Class for focuser properties"""
    def __init__(self, name=None, resolution=None, absLowerLimit=None, absUpperLimit=None, speed=None, stepSize=None, temperatureCompensation=None):
        self.name= name
        self.resolution=resolution 
        self.absLowerLimit=absLowerLimit
        self.absUpperLimit=absUpperLimit
        self.speed=speed
        self.stepSize=stepSize
        self.temperatureCompensation=temperatureCompensation
        self.focDef=None # will be set by class Acquire()

class CCD():
    """Class for CCD properties"""
    def __init__(self, name=None, binning=None, windowOffsetX=None, windowOffsetY=None, windowHeight=None, windowWidth=None, pixelSize=None, baseExposure=None):
        
        self.name= name
        self.binning=binning
        self.windowOffsetX=windowOffsetX
        self.windowOffsetY=windowOffsetY
        self.windowHeight=windowHeight
        self.windowWidth=windowWidth
        self.pixelSize= pixelSize
        self.baseExposure= baseExposure


class CheckDevices(object):
    """Check the presende of the devices and filter slots"""    
    def __init__(self, debug=False, rt=None, logger=None):
        self.debug=debug
        self.rt=rt
        self.logger=logger
        # ToDo move to config
        # JSON proxy
        url='http://127.0.0.1:8889'
        #    url='http://rts2.mayora.csic.es:8889'
        # if tha does not work set in rts2.ini
        # [xmlrpcd]::auth_localhost = true
        username='petr'
        password='test' 
        self.proxy= JSONProxy(url=url,username=username,password=password)
        self.connected=False
        self.errorMessage=None
        # no return value here
        try:
            self.proxy.refresh()
            self.connected=True
        except Exception, e:
            self.errorMessage=e

    def camera(self):
        self.proxy.refresh()
        try:
            self.proxy.getDevice(self.rt.ccd.name)
        except:
            self.logger.error('checkDevices: device: {0} not present'.format(self.ccd.name))        
            return False
        if self.debug: self.logger.debug('checkDevices: camera: {0} present'.format(self.rt.ccd.name))
        return True

    def focuser(self):
        self.proxy.refresh()
        try:
            self.proxy.getDevice(self.rt.foc.name)
        except:
            self.logger.error('checkDevices: device: {0} not present'.format(self.foc.name))        
            return False
        if self.debug: self.logger.debug('checkDevices: focuser: {0} present'.format(self.rt.foc.name))
        return True

    def filterWheels(self):
        self.proxy.refresh()
        for ftw in self.rt.filterWheelsInUse:
            try:
                self.proxy.getDevice(ftw.name)
            except:
                self.logger.error('checkDevices: device {0} not present'.format(ftw.name))        
                return False
            fts=  self.proxy.getSelection(ftw.name, 'filter')
            # check its filters
            for ft in ftw.filters:
                if ft.name in fts:
                    if self.debug: self.logger.debug('checkDevices: filter wheel: {0}, filter: {1} present'.format(ftw.name, ft.name))
                else:
                    self.logger.error('checkDevices: device {0} filter: {1} not present'.format(ftw.name, ft.name))        
                    return False
        return True
            


if __name__ == '__main__':

    import argparse
    try:
        import lib.devices as dev
    except:
        import devices as dev
    try:
        import lib.log as  lg
    except:
        import log as lg
    try:
        import lib.config as cfgd
    except:
        import config as cfgd

    parser= argparse.ArgumentParser(prog=sys.argv[0], description='rts2asaf check devices')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--logfile',dest='logfile', default='/tmp/{0}.log'.format(sys.argv[0]), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--config', dest='config', action='store', default='./rts2saf-my.cfg', help=': %(default)s, configuration file path')

    parser.add_argument('--foc',dest='foc', default='F0', help=': %(default)s, focuser device')
    parser.add_argument('--ftws',dest='ftws', default=['W0'], nargs='+', type=str, metavar='FTW', help=': %(default)s, filter wheels')
    parser.add_argument('--fts',dest='fts', default=['U'], nargs='+', type=str, metavar='FT', help=': %(default)s, filters')

    args=parser.parse_args()

    lgd= lg.Logger(debug=args.debug, args=args) # if you need to chage the log format do it here
    logger= lgd.logger 

    rt=cfgd.Configuration(logger=logger)
    rt.readConfiguration(fileName=args.config)

    cdv= CheckDevices(debug=args.debug, rt=rt, logger=logger)

    cdv.camera()
    cdv.focuser()
    cdv.filterWheels()

#!/usr/bin/python
# (C) 2013, Markus Wildi, markus.wildi@bluewin.ch
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

__author__ = 'markus.wildi@bluewin.ch'

import argparse
import sys
import threading
import time
import Queue
import numpy as np

from rts2.json import JSONProxy


class ScanThread(threading.Thread):
    """Thread scan aqcuires a set of FITS image filenames"""
    def __init__(self, debug=False, dryFitsFiles=None, foc=None, ccd=None, offsets=None, focDef=None, proxy=None, acqu_oq=None, logger=None):
        super(ScanThread, self).__init__()
        self.stoprequest = threading.Event()
        self.debug=debug
        self.dryFitsFiles=dryFitsFiles
        self.foc=foc
        self.ccd=ccd
        self.offsets=offsets
        self.focDef=focDef
        self.proxy=proxy
        self.acqu_oq=acqu_oq
        self.logger=logger

    def run(self):
        if self.debug: self.logger.debug('____ScantThread: running')

        for i,pos in enumerate(self.offsets): 
            self.proxy.setValue(self.foc.name,'FOC_FOFF', pos)
            slt= 1. + float(self.foc.stepSize) / self.foc.speed # ToDo, sleep a bit longer, ok?
            time.sleep( slt)
            self.proxy.refresh()
            focPos = int(self.proxy.getSingleValue(self.foc.name,'FOC_POS'))
            focPosCalc= int(self.focDef) + pos

            while abs( focPosCalc- focPos) > self.foc.resolution:

                if self.debug: self.logger.debug('acquire: focuser position not reached: abs({0:5d}- {1:5d})= {2:5d} > {3:5d} FOC_DEF: {4}, sleep time: {5:3.1f}'.format(focPosCalc, focPos, abs( focPosCalc- focPos), self.foc.resolution, self.focDef, slt))
                self.proxy.refresh()
                focPos = int(self.proxy.getSingleValue(self.foc.name,'FOC_POS'))
                focPosCalc= int(self.focDef) + pos
                time.sleep(.2) # leave it alone
            else:
                self.logger.info('acquire: focuser position reached: abs({0:5d}- {1:5d})= {2:5d} > {3:5d} FOC_DEF:{4}, sleep time: {5:3.1f}'.format(focPosCalc, focPos, abs( focPosCalc- focPos), self.foc.resolution, slt, self.focDef))

            fn=self.expose()
            if self.debug: self.logger.debug('acquire: received fits filename {0}'.format(fn))
            self.acqu_oq.put(fn)
        else:
            if self.debug: self.logger.debug('____ScanThread: ending after full scan')

    def expose(self):
        self.proxy.setValue(self.ccd.name,'exposure',self.ccd.baseExposure)
        self.proxy.executeCommand(self.ccd.name,'expose')

        self.proxy.refresh()
        expEnd = self.proxy.getDevice(self.ccd.name)['exposure_end'][1]
        rdtt= self.proxy.getDevice(self.ccd.name)['readout_time'][1]
        # critical: RTS2 creats the file on start of exposure
        time.sleep(expEnd-time.time() + rdtt)

        self.proxy.refresh()
        fn=self.proxy.getDevice('XMLRPC')['{0}_lastimage'.format(self.ccd.name)][1]

        if self.dryFitsFiles:
            fnd= self.dryFitsFiles.pop()
            if self.debug: self.logger.debug('____ScanThread: dryFits: file from ccd: {0}, returning dry FITS file: {1}'.format(fn, fnd))
            return fnd
        else:
            if self.debug: self.logger.debug('____ScanThread: file from ccd: {0}, reason no more dry FITS files (add more if necessary)'.format(fn))
            return fn 

    def join(self, timeout=None):
        if self.debug: self.logger.debug('____ScanThread: join, timeout {0}, stopping thread on request'.format(timeout))
        self.stoprequest.set()
        super(ScanThread, self).join(timeout)


class Acquire(object):
    """Acquire FITS images for analysis"""
    def __init__(self, 
                 debug=False,
                 dryFitsFiles=None,
                 ftw=None,
                 ft=None,
                 foc=None,
                 ccd=None,
                 filterWheelsInUse=None,
                 acqu_oq=None,
                 logger=None):

        self.debug=debug
        self.dryFitsFiles=dryFitsFiles
        self.ftw=ftw
        self.ft=ft
        self.foc=foc
        self.ccd=ccd
        self.filterWheelsInUse=filterWheelsInUse
        self.acqu_oq=acqu_oq
        self.logger=logger
        # 
        self.exposure= ccd.baseExposure
        # ToDo
        self.ccdRead= 1.
        self.iFocType=None
        self.iFocPos=None
        self.iFocTar=None
        self.iFocDef=None
        self.iFocFoff=None
        self.iFocToff=None
        # ToDo move to config
        # JSON proxy
        url='http://127.0.0.1:8889'
        #    url='http://rts2.mayora.csic.es:8889'
        # if tha does not work set in rts2.ini
        # [xmlrpcd]::auth_localhost = true
        username='petr'
        password='test' 
        self.proxy= JSONProxy(url=url,username=username,password=password)
        self.proxy.refresh()


    def requiredDevices(self):
        try:
            self.proxy.getDevice(self.foc.name)
        except:
            self.logger.error('acquire: {0} not present'.format(self.foc.name))        
            return False
        try:
            self.proxy.getDevice(self.ccd.name)
        except:
            self.logger.error('acquire: {0} not present'.format(self.ccd.name))        
            return False

        for ftw in self.filterWheelsInUse:
            try:
                self.proxy.getDevice(ftw.name)
            except:
                self.logger.error('acquire: {0} not present'.format(ftw.name))        
                return False

        return True

    def initialState(self):
        self.proxy.refresh()
        self.iFocType= self.proxy.getSingleValue(self.foc.name,'FOC_TYPE')    
        self.iFocPos = self.proxy.getSingleValue(self.foc.name,'FOC_POS')    
        self.iFocTar = self.proxy.getSingleValue(self.foc.name,'FOC_TAR')    
        self.iFocDef = self.proxy.getSingleValue(self.foc.name,'FOC_DEF')    
        self.iFocFoff= self.proxy.getSingleValue(self.foc.name,'FOC_FOFF')    
        self.iFocToff= self.proxy.getSingleValue(self.foc.name,'FOC_TOFF')    
        self.foc.focDef=self.iFocDef
        
        self.logger.debug('current focuser: {0}'.format(self.iFocType))        
        self.logger.debug('current FOC_DEF: {0}'.format(self.iFocDef))



        self.proxy.setValue(self.foc.name,'FOC_FOFF', 0)
        self.proxy.setValue(self.foc.name,'FOC_TOFF', 0)
        # ToDo filter
        # set all but ftw to slot clear path
        for ftw in self.filterWheelsInUse:
            if ftw.name not in self.ftw.name:
                if self.debug: self.logger.debug('acquire: filter wheel: {0}, setting empty slot on filter wheel: {1}:{2}, '.format(self.ftw.name,ftw.name, ftw.filters[0].name))
                self.proxy.setValue(ftw.name, 'filter',  ftw.filters[0]) # has been sorted (lowest filter offset)


        self.proxy.setValue(self.ftw.name, 'filter',  self.ft.name)
        if self.debug: self.logger.debug('acquire: setting on filter wheel: {0}, filter: {1}'.format(self.ftw.name, self.ft.name))
                



    def finalState(self):
        self.proxy.setValue(self.foc.name,'FOC_DEF',  self.iFocDef)
        self.proxy.setValue(self.foc.name,'FOC_FOFF', self.iFocFoff)
        self.proxy.setValue(self.foc.name,'FOC_TOFF', self.iFocToff)
        # ToDo filter


    def startScan(self, acqu_oq=None):
        if self.requiredDevices():            
            self.initialState()
            self.scanThread= ScanThread(
                debug=self.debug, 
                dryFitsFiles=self.dryFitsFiles,
                foc=self.foc, 
                ccd=self.ccd, 
                offsets=self.ft.offsets, 
                focDef=self.iFocDef, 
                proxy=self.proxy, 
                acqu_oq=self.acqu_oq, 
                logger=self.logger)

            self.scanThread.start()
            return True
        else:
            self.logger.error('acquire: not all required devices are present')
            return False
            
            
    def stopScan(self, timeout=1.):
        self.scanThread.join(timeout)
        self.finalState

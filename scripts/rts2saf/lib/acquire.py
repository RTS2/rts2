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
import shutil

from rts2.json import JSONProxy

class ScanThread(threading.Thread):
    """Thread scan aqcuires a set of FITS image filenames"""
    def __init__(self, debug=False, dryFitsFiles=None, ftw=None, ft=None, foc=None, ccd=None, focDef=None, exposure=None, blind=False, proxy=None, acqu_oq=None, ev=None, logger=None):
        super(ScanThread, self).__init__()
        self.stoprequest = threading.Event()
        self.debug=debug
        self.dryFitsFiles=dryFitsFiles
        self.ftw=ftw
        self.ft=ft
        self.foc=foc
        self.ccd=ccd
        self.focDef=focDef
        self.exposure=exposure
        self.blind=blind
        self.proxy=proxy
        self.acqu_oq=acqu_oq
        self.ev=ev
        self.logger=logger

    def run(self):
        if self.debug: self.logger.debug('____ScantThread: running')

        for i,pos in enumerate(self.foc.focFoff): 
            if self.blind:
                self.proxy.setValue(self.foc.name,'FOC_TAR', pos)
                if self.debug: self.logger.debug('acquire: set FOC_TAR:{0}'.format(pos))
            else:
                self.proxy.setValue(self.foc.name,'FOC_FOFF', pos)
            slt= 1. + float(self.foc.stepSize) / self.foc.speed # ToDo, sleep a bit longer, ok?
            time.sleep( slt)

            focPosCalc= pos
            if not self.blind:
                focPosCalc += int(self.focDef)

            self.proxy.refresh()
            focPos = int(self.proxy.getSingleValue(self.foc.name,'FOC_POS'))
            if self.debug: self.logger.debug('acquire: focPosCalc:{0}'.format(focPosCalc))
            while abs( focPosCalc- focPos) > self.foc.resolution:

                if self.debug: self.logger.debug('acquire: focuser position not reached: abs({0:5d}- {1:5d})= {2:5d} > {3:5d} FOC_DEF: {4}, sleep time: {5:3.1f}'.format(focPosCalc, focPos, abs( focPosCalc- focPos), self.foc.resolution, self.focDef, slt))
                self.proxy.refresh()
                focPos = int(self.proxy.getSingleValue(self.foc.name,'FOC_POS'))
                time.sleep(.2) # leave it alone
            else:
                self.logger.info('acquire: focuser position reached: abs({0:5d}- {1:5d})= {2:5d} <= {3:5d} FOC_DEF:{4}, sleep time: {5:3.1f}'.format(focPosCalc, focPos, abs( focPosCalc- focPos), self.foc.resolution, self.focDef, slt))

            fn=self.expose()
            if self.debug: self.logger.debug('acquire: received fits filename {0}'.format(fn))
            self.acqu_oq.put(fn)
        else:
            if self.debug: self.logger.debug('____ScanThread: ending after full scan')

    def expose(self):
        if self.exposure:
            exp= self.exposure # args.exposure
        else:
            exp= float(self.ccd.baseExposure)
        exp *= self.ft.exposureFactor

        self.proxy.setValue(self.ccd.name,'exposure', str(exp))
        self.proxy.executeCommand(self.ccd.name,'expose')

        self.proxy.refresh()
        expEnd = self.proxy.getDevice(self.ccd.name)['exposure_end'][1]
        rdtt= self.proxy.getDevice(self.ccd.name)['readout_time'][1]
        # at a fresh start readout_time may be empty
        if not rdtt:
            rdtt= 5. # beeing on the save side, subsequent calls have the correct time, ToDo: might not be true!

        # critical: RTS2 creates the file on start of exposure
        # see TAG WAIT
        time.sleep(expEnd-time.time() + rdtt)

        self.proxy.refresh()
        fn=self.proxy.getDevice('XMLRPC')['{0}_lastimage'.format(self.ccd.name)][1]

        if self.ftw and self.ft:
            if not self.ev.createAcquisitionBasePath(ftwName=self.ftw.name, ftName=self.ft.name):
                return None 
        else:
            if not self.ev.createAcquisitionBasePath(ftwName=None, ftName=None):
                return None 

        if self.dryFitsFiles:
            srcTmpFn= self.dryFitsFiles.pop()
            if self.debug: self.logger.debug('____ScanThread: dryFits: file from ccd: {0}, returning dry FITS file: {1}'.format(fn, srcTmpFn))
        else:
            srcTmpFn=fn
            if self.debug: self.logger.debug('____ScanThread: file from ccd: {0}, reason no more dry FITS files (add more if necessary)'.format(srcTmpFn))
            # ToDo fn ist not moved by RTS2 means (as it was in rts2af)
            # might be not a good idea
            # ask Petr to expand JSON interface
            # ?copy instead of moving?
            # TAG WAIT
            # with dummy camera it takes time until the images in available
            while True:
                try:
                    myfile = open(srcTmpFn, "r") 
                    myfile.close()
                    break
                except Exception, e:                    
                    if self.debug: self.logger.debug('____ScanThread: {0} not yet ready'.format(srcTmpFn))
                time.sleep(.5) # ToDo if it persits: put it to cfg!

                
        if self.ftw and self.ft:
            storeFn=self.ev.expandToAcquisitionBasePath( ftwName=self.ftw.name, ftName=self.ft.name) + srcTmpFn.split('/')[-1]
        else:
            storeFn=self.ev.expandToAcquisitionBasePath( ftwName=None, ftName=None) + srcTmpFn.split('/')[-1]

        try:
            # ToDo shutil.move(src=fn, dst=storeFn)
            shutil.copy(src=srcTmpFn, dst=storeFn)
            return storeFn 
        except Exception, e:
            self.logger.error('____ScanThread: something wrong with file from ccd: {0} or destination {1}:\n{2}'.format(fn, storeFn,e))
            return None 

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
                 rt=None,
                 ev=None,
                 logger=None):

        self.debug=debug
        self.dryFitsFiles=dryFitsFiles
        self.ftw=ftw
        self.ft=ft
        self.foc=foc
        self.ccd=ccd
        self.filterWheelsInUse=filterWheelsInUse
        self.acqu_oq=acqu_oq
        self.rt=rt
        self.ev=ev
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
        self.proxy= JSONProxy(url=self.rt.cfg['URL'],username=self.rt.cfg['USERNAME'],password=self.rt.cfg['PASSWORD'])
        self.connected=False
        self.errorMessage=None
        # no return value here
        try:
            self.proxy.refresh()
            self.connected=True
        except Exception, e:
            self.errorMessage=e

    def __initialState(self):
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
        # NO, please NOT self.proxy.setValue(self.foc.name,'FOC_TOFF', 0)
        # ToDo filter
        # set all but ftw to empty slot
        for ftw in self.filterWheelsInUse:
            if ftw.name not in self.ftw.name:
                if self.debug: self.logger.debug('acquire: filter wheel: {0}, setting empty slot on filter wheel: {1} to {2}'.format(self.ftw.name,ftw.name, ftw.filters[0].name))
                self.proxy.setValue(ftw.name, 'filter',  ftw.filters[0]) # has been sorted (lowest filter offset)

        if self.ft:
            self.proxy.setValue(self.ftw.name, 'filter',  self.ft.name)
            if self.debug: self.logger.debug('acquire: setting on filter wheel: {0}, filter: {1}'.format(self.ftw.name, self.ft.name))

    def __finalState(self):
        self.proxy.setValue(self.foc.name,'FOC_DEF',  self.iFocDef)
        self.proxy.setValue(self.foc.name,'FOC_FOFF', self.iFocFoff)
        # NO, please NOT self.proxy.setValue(self.foc.name,'FOC_TOFF', self.iFocToff)
        # ToDo filter

    def startScan(self, exposure=None, blind=False):
        if not self.connected:
            self.logger.error('acquire: no connection to rts2-centrald:\n{0}'.format(self.errorMessage))
            return False
            
        self.__initialState()
        if self.ft:
            ftt=self.ft
        else:
            ftt=None

        self.scanThread= ScanThread(
            debug=self.debug, 
            dryFitsFiles=self.dryFitsFiles,
            ftw=self.ftw,
            ft= ftt,
            foc=self.foc, 
            ccd=self.ccd, 
            focDef=self.iFocDef, 
            exposure=exposure,
            blind=blind,
            proxy=self.proxy, 
            acqu_oq=self.acqu_oq, 
            ev=self.ev,
            logger=self.logger)


            
        self.scanThread.start()
        return True
            
    def stopScan(self, timeout=1.):
        self.scanThread.join(timeout)
        self.__finalState

    def writeFocDef(self):
        if self.foc.focMn and self.foc.focMx:
            if self.foc.focMn < self.foc.focDef < self.foc.focMx:
                self.proxy.setValue(self.foc.name,'FOC_DEF', self.foc.focDef)
            else:
                self.logger.warn('acquire: focuser: {0} not writing FOC_DEF value: {1} out of bounds ({2}, {3})'.format(self.foc.name, self.foc.focDef, self.foc.focMn, self.foc.focMx))
        else:
            self.logger.warn('acquire: focuser: {0} not writing FOC_DEF, no minimum or maximum value present'.format(self.foc.name))

    def writeOffsets(self, ftw=None):
        """Write only for camd::filter filters variable the offsets """
        self.proxy.refresh()
        theWheel=  self.proxy.getSingleValue(self.ccd.name, 'wheel')
        if ftw.name in theWheel:
            filterNames=  self.proxy.getSelection(self.ccd.name, 'filter')
            # order matters :-))
            ftns= map( lambda x: x.name, ftw.filters)
            offsets=str()
            for ftn in filterNames:
                try:
                    ind= ftns.index(ftn)
                except:
                    self.logger.error('acquire: filter wheel: {0} incomplete list of filter offsets, do not write them to CCD: {1}'.format(ftw.name, self.ccd.name))
                    
                    break
                offsets += '{0} '.format(int(ftw.filters[ind].OffsetToEmptySlot))
                if self.debug: self.logger.debug('acquire:ft.name: {0}, offset:{1}, offsets: {2}'.format(ftn, ftw.filters[ind].OffsetToEmptySlot, offsets))
            else:
                self.proxy.setValue(self.ccd.name,'filter_offsets', offsets[:-1])

        else:
            if self.debug: self.logger.debug('acquire:ft.name: {0} is not the main wheel'.format(ftn, theWheel))






if __name__ == '__main__':

    import Queue
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
    try:
        import lib.devices as dev
    except:
        import devices as dev

    try:
        import lib.environ as env
    except:
        import environ as env
    try:
        import lib.sextract as sx
    except:
        import sextract as sx

    parser= argparse.ArgumentParser(prog=sys.argv[0], description='rts2asaf check devices')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--logfile',dest='logfile', default='/tmp/{0}.log'.format(sys.argv[0]), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--config', dest='config', action='store', default='./configs/rts2saf-one-filter-wheel.cfg', help=': %(default)s, configuration file path')
    parser.add_argument('--blind', dest='blind', action='store_true', default=False, help=': %(default)s, focus run within range(RTS2::foc_min,RTS2::foc_max, RTS2::foc_step), if --focStep is defined it is used to set the range')

    args=parser.parse_args()

    lgd= lg.Logger(debug=args.debug, args=args) # if you need to chage the log format do it here
    logger= lgd.logger 

    rt=cfgd.Configuration(logger=logger)
    rt.readConfiguration(fileName=args.config)

    ev=env.Environment(debug=args.debug, rt=rt,logger=logger)
    ev.createAcquisitionBasePath(ftwName=None, ftName=None)

    cdv= dev.CheckDevices(debug=args.debug, args=args, rt=rt, logger=logger)
    if not cdv.statusDevices():
        logger.error('acquire: exiting, check the configuration file: {0}'.format(args.config))
        sys.exit(1)

    acqu_oq = Queue.Queue()

    dFF=None
    ftw = rt.filterWheelsInUse[0]
    ft = rt.filterWheelsInUse[0].filters[0]
    exposure= 1.
    blind=args.blind
    stepSize=2
    rt.foc.stepSize=stepSize
    rt.foc.focFoff=range(-5,5,stepSize)

    acqu= Acquire(debug=args.debug, dryFitsFiles=dFF, ftw=ftw, ft=ft, foc=rt.foc, ccd=rt.ccd, filterWheelsInUse=rt.filterWheelsInUse, acqu_oq=acqu_oq, rt=rt, ev=ev, logger=logger)
    if not acqu.startScan(exposure=exposure, blind=blind):
        self.logger.error('acquire: exiting')
        sys.exit(1)

    for i,st in enumerate(rt.foc.focFoff):
        while True:
            try:
                fitsFn= acqu_oq.get(block=True, timeout=.2)
            except Queue.Empty:
                # if args.debug: self.logger.debug('acquire: continue')
                continue
                    
            logger.info('acquire: got FITS file name: {}'.format(fitsFn))
            break
        else:
            if args.debug: logger.debug('acquire: got all images')
            acqu.stopScan(timeout=1.)
    else:
        logger.info('acquire: DONE')        
        sys.exit(1)

    logger.info('acquire: something went wrong')        

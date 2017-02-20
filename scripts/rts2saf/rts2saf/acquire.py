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
"""Acquire steers CCD, filter wheels and focuser, acquires the FITS 
file names and copies the files to BASE_DIRECTORY.

"""
__author__ = 'markus.wildi@bluewin.ch'

import sys
import threading
import time
import shutil

# Todo: ugly
def waitForFocuser(foc=None, focPosCalc=None, focDef=None, proxy=None, debug=None, logger=None):
    debug=True
    focPos = int(proxy.getSingleValue(foc.name,'FOC_POS'))
    slt= abs(float(focPosCalc-focPos)/ foc.speed) 
    logger.info('waitForFocuser: focPosCalc:{0}, focPos: {1}, speed:{2}, sleep: {3:4.2f} sec'.format(focPosCalc, focPos, foc.speed, slt))
    time.sleep( slt)
    proxy.refresh()
    focPos = int(proxy.getSingleValue(foc.name,'FOC_POS'))
    commitSucide=1000
    while abs( focPosCalc- focPos) > foc.resolution:
                
        # if self.debug: self.logger.debug('acquire: focuser position not reached: abs({0:5d}- {1:5d})= {2:5d} > {3:5d} FOC_DEF: {4}, sleep time: {5:3.1f}'.format(int(focPosCalc), focPos, int(abs( focPosCalc- focPos)), self.foc.resolution, self.focDef, slt))
        proxy.refresh()
        focPos = int(proxy.getSingleValue(foc.name,'FOC_POS'))
        if debug: logger.debug('waitForFocuser: not yet reached focuser position: {0}, now:{1}, sleeping'.format(focPosCalc, focPos))
        time.sleep(.1) # leave it alone
        commitSucide -= 1
        if commitSucide < 0:
            logger.error('waitForFocuser: can not reach FOC_TAR: {} after {} seconds, exiting'.format(focPosCalc, commitSucide * .1))
            sys.exit(1)
        else:
            if debug: logger.debug('waitForFocuser: focuser position reached: abs({0:5d}- {1:5.0f}= {2:5.0f} <= {3:5.0f} FOC_DEF:{4}, sleep time: {5:4.2f} sec'.format(int(focPosCalc), focPos, abs( focPosCalc- focPos), foc.resolution, focDef, slt))



class ScanThread(threading.Thread):
    """Thread ScanThread acquires a set of FITS image filenames and writes them to :py:mod:`Queue` acqu_oq.

    :var debug: enable more debug output with --debug and --level
    :var dryFitsFiles: FITS files injected if the CCD can not provide them
    :var ftw: :py:mod:`rts2saf.devices.FilterWheel`
    :var ft: :py:mod:`rts2saf.devices.Filter`
    :var foc: :py:mod:`rts2saf.devices.Focuser`
    :var ccd: :py:mod:`rts2saf.devices.CCD`
    :var focDef: default focuser position (FOC_DEF)
    :var exposure: exposure 
    :var blind: flavor of the focus run
    :var writeToDevices: True, write to the devices 
    :var proxy: :py:mod:`rts2.json.JSONProxy`
    :var acqu_oq: :py:mod:`Queue`
    :var rt: run time configuration,  :py:mod:`rts2saf.config.Configuration`, usually read from /usr/local/etc/rts2/rts2saf/rts2saf.cfg
    :var ev: helper module for house keeping, :py:mod:`rts2saf.environ.Environment`
    :var logger:  :py:mod:`rts2saf.log`

"""

    def __init__(self, debug=False, dryFitsFiles=None, ftw=None, ft=None, foc=None, ccd=None, focDef=None, exposure=None, blind=False, writeToDevices=True, proxy=None, acqu_oq=None, rt=None, ev=None, logger=None):
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
        self.writeToDevices=writeToDevices
        self.proxy=proxy
        self.acqu_oq=acqu_oq
        self.rt=rt
        self.ev=ev
        self.logger=logger

    def run(self):
        """Move focuser to FOC_TOFF + FOC_DEF (regular mode) or FOC_TAR (blind mode), expose CCD and write FITS file name to :py:mod:`Queue`.  
        """
        if self.debug: self.logger.debug('____ScantThread: running')
        
        for i,pos in enumerate(self.foc.focToff): 
            if self.stoprequest.is_set():
                break

            if self.writeToDevices:
                if self.blind:
                    self.proxy.setValue(self.foc.name,'FOC_TAR', pos)
                        
                    if self.debug: self.logger.debug('acquire: set FOC_TAR:{0}'.format(pos))
                else:
                    self.proxy.setValue(self.foc.name,'FOC_TOFF', pos)
            else:
                self.logger.warn('acquire: disabled setting FOC_TAR or FOC_TOFF: {0}'.format(pos))

            focPosCalc= pos
            if not self.blind:
                focPosCalc += int(self.focDef)
            
            if self.writeToDevices:
                waitForFocuser(foc=self.foc, focPosCalc=focPosCalc, focDef=self.focDef, proxy=self.proxy, debug=self.debug, logger=self.logger)

                fn=self.expose()
            if fn:
                if self.debug: self.logger.debug('acquire: received fits filename {0}'.format(fn))
                # 
                self.acqu_oq.put(fn)
                if self.debug: self.logger.debug('acquire: received fits filename {0}'.format(fn))
            else:
                self.acqu_oq.put('continue')
                self.logger.warn('acquire: no fits file received')
        else:
            if self.debug: self.logger.debug('____ScanThread: ending after full scan')


    # ToDo find a solution for: ValueError: signal only works in main thread
    # outside this class: not an option
    #@timeout(seconds=1, error_message=os.strerror(errno.ETIMEDOUT))
    def expose(self):
        """Expose CCD 

        :return storeFn: FITS file name
        :rtype: str

        """
        if self.exposure:
            exp= self.exposure # args.exposure
        else:
            exp= self.ccd.baseExposure

        if self.ftw!=None: # CCD without filter wheel
            exp *= self.ft.exposureFactor

        self.logger.info('acquire: exposing for: {0:7.3f} sec, filter factor: {1:3.1f}'.format(exp, self.ft.exposureFactor))
        if self.writeToDevices:
                self.proxy.setValue(self.ccd.name,'exposure', str(exp))
                self.proxy.executeCommand(self.ccd.name,'expose')
        else:
            self.logger.warn('acquire: disabled setting exposure/expose: {0}'.format(exp))

        self.proxy.refresh()
        # does not really work expEnd = self.proxy.getDevice(self.ccd.name)['exposure_end'][1]
        rdtt= self.proxy.getDevice(self.ccd.name)['readout_time'][1]
        trtt= self.proxy.getDevice(self.ccd.name)['transfer_time'][1]
        
        # at a fresh start readout_time may be empty
        # ToDo check that at run time
        if not rdtt:
            rdtt= 5. # being on the save side, subsequent calls have the correct time, ToDo: might not be true!
            self.logger.warn('acquire: no read out time received, that is ok if RTS2 CCD driver has just been (re-)started, sleeping:{}'.format(rdtt))
        if not trtt:
            trtt= 5. 
            self.logger.warn('acquire: no transfer time received, that is ok if RTS2 CCD driver has just been (re-)started, sleeping:{}'.format(trtt))
        
        # critical: RTS2 creates the file on start of exposure
        # see TAG WAIT
        if self.writeToDevices:
            # sleep necessary
            try:
                self.logger.info('acquire:time.sleep for: {0:3.1f} sec, waiting for exposure end'.format(exp + rdtt + trtt))
                time.sleep(exp + rdtt + trtt)
            except Exception, e:
                self.logger.warn('acquire:time.sleep received read out time: {0:3.1f}, transfer time: {0:3.1f}, should sleep: {0:3.1f}'.format(rdtt, trtt, exp + rdtt + trtt))
                self.logger.warn('acquire: not sleeping, error from time.sleep(): {}'.format(e))
                
        self.proxy.refresh()
        fn=self.proxy.getDevice('HTTPD')['{0}_lastimage'.format(self.ccd.name)][1]

        if self.ftw and self.ft:
            if not self.ev.createAcquisitionBasePath(ftwName=self.ftw.name, ftName=self.ft.name):
                return None 
        else:
            if not self.ev.createAcquisitionBasePath(ftwName=None, ftName=None):
                return None 

        if self.dryFitsFiles:
            srcTmpFn= self.dryFitsFiles.pop()
            self.logger.info('____ScanThread, expose: dryFits: file from ccd: {0}, returning dry FITS file: {1}'.format(fn, srcTmpFn))
        else:
            srcTmpFn=fn
            if self.debug: self.logger.debug('____ScanThread: file from ccd: {0}, in case this run took place with --dryfitsfile: no more dry FITS files (add more if necessary)'.format(srcTmpFn))

        if self.ftw and self.ft:
            storeFn=self.ev.expandToAcquisitionBasePath(ftwName=self.ftw.name, ftName=self.ft.name) + srcTmpFn.split('/')[-1]
        else:
            storeFn=self.ev.expandToAcquisitionBasePath(ftwName=None, ftName=None) + srcTmpFn.split('/')[-1]
        # ToDo fn is not moved by RTS2 means (as it was in rts2af)
        # might be not a good idea
        # ask Petr to expand JSON interface
        # ?copy instead of moving?
        # TAG WAIT
        # with dummy camera it takes time until the images are available
        cnt=0
        ready=True
        while True:
            try:
                # ToDo shutil.move(src=fn, dst=storeFn)
                # the file belongs to root or user.group
                # ToDo inadequate use pynotify
                time.sleep(10) # ToDo

                shutil.copy(src=srcTmpFn, dst=storeFn)
                if not ready:
                    self.logger.info('____ScanThread: received {0}'.format(srcTmpFn))
                return storeFn 
            except Exception, e:
                self.logger.warn('____ScanThread: not yet ready {0}, error: {1}'.format(srcTmpFn, e))
                # on slow system this is glitch use pynotify
                time.sleep(.2) # ToDo
                cnt +=1
                if cnt > 40:
                    self.logger.error('____ScanThread: not received file {0}, returning'.format(srcTmpFn))
                    return None

    def join(self, timeout=None):
        """Stop thread on request.
        """
        self.logger.info('____ScanThread: join, timeout {0}, stopping thread on request'.format(timeout))
        self.stoprequest.set()
        super(ScanThread, self).join(timeout)


class Acquire(object):
    """Prepare acquisition of FITS images for analysis.

    :var debug: enable more debug output with --debug and --level
    :var proxy: :py:mod:`rts2.json.JSONProxy`
    :var dryFitsFiles: FITS files injected if the CCD can not provide them
    :var ft: :py:mod:`rts2saf.devices.Filter`
    :var foc: :py:mod:`rts2saf.devices.Focuser`
    :var ccd: :py:mod:`rts2saf.devices.CCD`
    :var ftws: list of :py:mod:`rts2saf.devices.FilterWheel`
    :var acqu_oq: :py:mod:`Queue`
    :var writeToDevices: True, write to the devices 
    :var rt: run time configuration,  :py:mod:`rts2saf.config.Configuration`, usually read from /usr/local/etc/rts2/rts2saf/rts2saf.cfg
    :var ev: helper module for house keeping, :py:mod:`rts2saf.environ.Environment`
    :var logger:  :py:mod:`rts2saf.log`

    """
    def __init__(self, 
                 debug=False,
                 proxy=None,
                 dryFitsFiles=None,
                 ftw=None,
                 ft=None,
                 foc=None,
                 ccd=None,
                 ftws=None,
                 acqu_oq=None,
                 writeToDevices=True,
                 rt=None,
                 ev=None,
                 logger=None):

        self.debug=debug
        self.proxy=proxy
        self.dryFitsFiles=dryFitsFiles
        self.ftw=ftw
        self.ft=ft
        self.foc=foc
        self.ccd=ccd
        self.ftws=ftws
        self.acqu_oq=acqu_oq
        self.writeToDevices=writeToDevices
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
        self.iFocFilteroff=None
        self.iFocFoff=None
        self.iFocToff=None
        self.iBinning=None
        self.iWindow=None
        self.proxy= proxy
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
        self.iFocFilteroff = self.proxy.getSingleValue(self.foc.name,'FOC_FILTEROFF')    
        self.iFocFoff= self.proxy.getSingleValue(self.foc.name,'FOC_FOFF')    
        self.iFocToff= self.proxy.getSingleValue(self.foc.name,'FOC_TOFF')    
        self.iBinning= self.proxy.getSingleValue(self.ccd.name,'binning')    
        self.iWindow = self.proxy.getValue(self.ccd.name,'WINDOW')    

        self.logger.debug('acquire: current focuser: {0}'.format(self.iFocType))        
        self.logger.debug('acquire: current FOC_DEF: {0}'.format(self.iFocDef))

        if self.writeToDevices:
            # FOC_FILTEROFF, FOC_FOFF may be non zero
            self.proxy.setValue(self.foc.name,'FOC_FILTEROFF', 0)
            tar=self.iFocDef + self.iFocFoff + self.iFocToff
            waitForFocuser(foc=self.foc, focPosCalc=tar, focDef=self.iFocDef, proxy=self.proxy, debug=self.debug, logger=self.logger)

            self.proxy.setValue(self.foc.name,'FOC_FOFF', 0)
            tar=self.iFocDef + self.iFocToff
            waitForFocuser(foc=self.foc, focPosCalc=tar, focDef=self.iFocDef, proxy=self.proxy, debug=self.debug, logger=self.logger)

            self.proxy.setValue(self.foc.name,'FOC_TOFF', 0)
            tar=self.iFocDef 
            waitForFocuser(foc=self.foc, focPosCalc=tar, focDef=self.iFocDef, proxy=self.proxy, debug=self.debug, logger=self.logger)


            self.proxy.setValue(self.ccd.name,'binning', self.ccd.binning)
            self.proxy.setValue(self.ccd.name,'WINDOW', ' '.join(str(x) for x in self.ccd.window))
        else:
            self.logger.warn('acquire: disabled setting FOC_TOFF, CCD binning, WINDOW')

        # ToDo filter
        # set all but ftw to empty slot
        for ftw in self.ftws:
            # no real filter wheel, used for setups without filter wheels
            if ftw.name in 'FAKE_FTW':
                return

            if ftw.name not in self.ftw.name:
                if self.debug: self.logger.debug('acquire:  {0}, setting empty slot on  {1} to {2}'.format(self.ftw.name,ftw.name, ftw.filters[0].name))
                if self.writeToDevices:
                    self.proxy.setValue(ftw.name, 'filter',  ftw.filters[0].name) # has been sorted (lowest filter offset)
                else:
                    self.logger.warn('acquire: disabled setting filter: {0} on {1}'.format(ftw.filters[0].name,ftw.name))

        # no real filter, used for CCDs without filter wheels
        if self.ft and not self.ft.name in 'FAKE_FT':
            if self.writeToDevices:
                self.proxy.setValue(self.ftw.name, 'filter',  self.ft.name)
                if self.debug: self.logger.debug('acquire: setting on  {0}, filter: {1}'.format(self.ftw.name, self.ft.name))
            else:
                self.logger.warn('acquire: disabled setting filter: {0} on {1}'.format(self.ft.name, self.ftw.name,ftw.name))

    def __finalState(self, blind=None):
        if self.writeToDevices:
            if blind:
                # do nothing
                pass # NO self.proxy.setValue(self.foc.name,'FOC_TAR', self.iFocDef)
            else:
                self.proxy.setValue(self.foc.name,'FOC_TOFF', 0)
                tar=self.iFocDef 
                waitForFocuser(foc=self.foc, focPosCalc=tar, focDef=self.iFocDef, proxy=self.proxy, debug=self.debug, logger=self.logger)

            self.proxy.setValue(self.ccd.name,'binning', self.iBinning)
            self.proxy.setValue(self.ccd.name,'WINDOW', ' '.join(str(x) for x in self.iWindow))
        else:
            self.logger.warn('acquire: disabled setting FOC_DEF: {0}, FOC_TOFF: 0',format(self.iFocDef))

    def writeFocDef(self):
        # either from device or from configuration
        if self.foc.focMn < self.foc.focDef < self.foc.focMx:
            if self.writeToDevices:
                self.proxy.setValue(self.foc.name,'FOC_DEF', int(self.foc.focDef))
                self.logger.info('writeFocDef: set FOC_DEF: {0}, {1}, {2}'.format(int(self.foc.focDef), self.foc.focMn,  self.foc.focMx))
            else:
                self.logger.warn('acquire: disabled writing FOC_DEF: {0}'.format(self.foc.focDef))
        else:
            self.logger.warn('acquire:  {0} not writing FOC_DEF value: {1} out of bounds ({2}, {3})'.format(self.foc.name, self.foc.focDef, self.foc.focMn, self.foc.focMx))

    def writeOffsets(self, ftw=None):
        """ 
        Write only for camd::filter filters variable the offsets 
        This method has never been really used and therefore assume it does not work.
        """
        theWheel= self.ccd.name
        self.proxy.refresh()
        if self.ccd.name not in self.ftws[0].name: # if it is real wheel and not CCD 'built in' filter wheel 
            theWheel=  self.proxy.getSingleValue(self.ccd.name, 'wheel')

        if ftw.name in theWheel:
            filterNames=  self.proxy.getSelection(self.ccd.name, 'filter')
            # order matters :-))
            ftns= map( lambda x: x.name, ftw.filters)
            offsets=str()
            for ftn in filterNames:
                # ToDo this is a glitch!!
                # yes, it is really (2014-03-06
                try:
                    ind= ftns.index(ftn)
                except Exception, e:
                    self.logger.error('acquire:  {0} filter: {1} not found in configuration (dropped? error: {2})'.format(ftw.name, ftn, e))
                    self.logger.error('acquire:  {0} incomplete list of filter offsets, do not write them to CCD: {1}'.format(ftw.name, self.ccd.name))
                    break

                offsets += '{0} '.format(int(ftw.filters[ind].OffsetToEmptySlot))
                if self.debug: self.logger.debug('acquire:ft.name: {0}, offset:{1}, offsets: {2}'.format(ftn, ftw.filters[ind].OffsetToEmptySlot, offsets))
            else:
                if self.writeToDevices:
                    self.proxy.setValue(self.ccd.name,'filter_offsets', offsets[:-1])
                else:
                    self.logger.warn('acquire: disabled writing filter_offsets: {0} '.format(offsets[:-1]))

        else:
            if self.debug: self.logger.debug('acquire:ftw.name: {0} is not the main wheel'.format(ftw.name))

    def startScan(self, exposure=None, blind=False):
        """Save RTS2 devices initial state. Start acquisition thread :py:mod:`rts2saf.acquire.ScanThread`.

        :param blind: flavor of the focus run
        :type blind: bool
        """

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
            writeToDevices=self.writeToDevices,
            proxy=self.proxy, 
            acqu_oq=self.acqu_oq, 
            rt=self.rt,
            ev=self.ev,
            logger=self.logger)

        self.scanThread.start()
        return True
            
    def stopScan(self, timeout=1., blind=None):
        """Stop acquisition thread :py:mod:`rts2saf.acquire.ScanThread`. Restore RTS2 devices initial state."""
        self.scanThread.join(timeout)
        self.__finalState(blind=blind)


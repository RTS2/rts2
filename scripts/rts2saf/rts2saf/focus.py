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
"""

"""

__author__ = 'markus.wildi@bluewin.ch'

import signal
import sys
import time
import Queue
import collections
import rts2saf.acquire as acq
import rts2saf.sextract as sx
import rts2saf.analyze as  an

TERM=False
def receive_signal(signum, stack):
    global TERM
    TERM=True


signal.signal(signal.SIGUSR1, receive_signal)
signal.signal(signal.SIGUSR2, receive_signal)
signal.signal(signal.SIGTERM, receive_signal)
signal.signal(signal.SIGHUP,  receive_signal)
signal.signal(signal.SIGQUIT, receive_signal)
signal.signal(signal.SIGINT,  receive_signal)


class Focus(object):
    """Main control class for online data acquisition.

    :var debug: enable more debug output with --debug and --level
    :var proxy: :py:mod:`rts2.json.JSONProxy`
    :var args: command line arguments or their defaults
    :var dryFitsFiles: FITS files injected if the CCD can not provide them
    :var ccd: :py:mod:`rts2saf.devices.CCD`
    :var foc: :py:mod:`rts2saf.devices.Focuser`
    :var ftws: :py:mod:`rts2saf.devices.FilterWheel`
    :var ft: :py:mod:`rts2saf.devices.Filter`
    :var rt: run time configuration,  :py:mod:`rts2saf.config.Configuration`, usually read from /usr/local/etc/rts2/rts2saf/rts2saf.cfg
    :var ev: helper module for house keeping, :py:mod:`rts2saf.environ.Environment`
    :var logger:  :py:mod:`rts2saf.log`

    """
    def __init__(self, debug=False, proxy=None, args=None, dryFitsFiles=None, ccd=None, foc=None, ftws=None, rt=None, ev=None, logger=None, xdisplay = None):
        self.debug=debug
        self.proxy=proxy
        self.args=args
        self.dryFitsFiles=dryFitsFiles
        self.ccd=ccd
        self.foc=foc
        self.ftws=ftws
        self.ev=ev
        self.rt=rt
        self.logger=logger
        self.xdisplay = xdisplay
        self.foc.focDef = None

    def run(self):
        """Loop over filter wheels, their filters and offsets (FOC_TOFF)
        """
        for k, ftw in enumerate(self.ftws):
            # only interesting in case multiple filter wheels are present
            if len(ftw.filters) ==1 and k>0:
                # self.ftws is sorted
                # these are filter wheels which have atleast one empty slot and no real filters 
                # they must appear in self.ftws in order to set the empty slot
                continue
        
            
            self.foc.focDef = None
            rFts= list()
            for ft in ftw.filters:
                if self.debug: self.logger.debug('Focus: start filter wheel: {}, filter:{}'.format(ftw.name, ft.name))

                dFF=None
                if self.dryFitsFiles:
                    dFF=self.dryFitsFiles[:]
                    self.logger.info('Focus: using dry FITS files')
                    if self.debug: self.logger.debug('Focus: using dry FITS files from: {}'.format(self.dryFitsFiles))
                # acquisition
                acqu_oq = Queue.Queue()

                acqu= acq.Acquire(
                    debug=self.debug, 
                    proxy=self.proxy, 
                    dryFitsFiles=dFF, 
                    ftw=ftw, ft=ft, 
                    foc=self.foc, 
                    ccd=self.ccd, 
                    ftws=self.ftws, 
                    acqu_oq=acqu_oq, 
                    rt=self.rt, 
                    ev=self.ev, 
                    logger=self.logger)

                # steps are defined per filter, if blind in focuser
                if not self.args.blind:
                    self.foc.focToff=ft.focToff

                # start acquisition thread
                if not acqu.startScan(exposure=self.args.exposure, blind=self.args.blind):
                    self.logger.error('Focus: exiting')
                    sys.exit(1)

                # acquire FITS from thread
                dataSxtr=list()
                global TERM
                for st in self.foc.focToff:
                    while True:
                        # if some external source wants to stop me
                        if TERM:
                            acqu.stopScan(timeout=1.)
                            time.sleep(2.)
                            self.logger.info('Focus: received signal, reset FOC_TOFF, stopped thread, exiting')
                            # threads are zombies
                            while acqu.scanThread.isAlive():
                                acqu.scanThread._Thread__stop()
                                time.sleep(1)

                            self.logger.info('Focus: exit now')
                            sys.exit(1)
                        try:
                            fitsFn= acqu_oq.get(block=True, timeout=.2)
                            break
                        except Queue.Empty:
                            # if self.debug: self.logger.debug('Focus: continue')
                            continue
                    # if expose () did not return a file name (e.g. destination not writeable), continue
                    if 'continue' in fitsFn:
                        if self.debug: self.logger.debug('Focus: continue on request from thread queue')
                        continue
                    sxtr= sx.Sextract(debug=self.debug,rt=self.rt,logger=self.logger)

                    if self.rt.cfg['ANALYZE_FLUX']:
                        sxtr.appendFluxFields()

                    dSx=sxtr.sextract(fitsFn=fitsFn)
                    if dSx.fitsFn==None:
                        self.logger.warn('Focus: sextractor failed on fits file: {0}'.format(fitsFn))
                        # ToDo get out of the loop if MINIMUM_FOCUSER_POSITIONS can not be achieved any more
                        continue

                    self.logger.info('Focus: pos: {0:5d}, fwhm: {1:5.2f}, stdFwhm {2:5.2f}, objects: {3:4d}, file: {4}'.format(int(st), dSx.fwhm, dSx.stdFwhm, len(dSx.catalog), fitsFn))
                    dataSxtr.append(dSx)

                else:
                    if self.debug: self.logger.debug('Focus: got all images')

                acqu.stopScan(timeout=1.)
                
                pos=collections.defaultdict(int)
                for dSx in dataSxtr:
                    pos[dSx.focPos] += 1

                if len(pos) <= self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
                    self.logger.warn('Focus: to few DIFFERENT focuser positions: {0}<={1} (see MINIMUM_FOCUSER_POSITIONS), continuing'.format(len(pos), self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']))
                    if self.debug:
                        for p,v in pos.iteritems():
                            self.logger.debug('Focus:{0:5.0f}: {1}'.format(p,v))
                    continue

                # appears on plot
                date=dataSxtr[0].date.split('.')[0]

                # no need to wait, might go to a thread too
                if self.args.catalogAnalysis:
                    anr=CatalogAnalysis(
                        debug=self.debug, 
                        dataSxtr=dataSxtr, 
                        Ds9Display=self.args.Ds9Display, 
                        FitDisplay=self.args.FitDisplay, 
                        xdisplay = self.xdisplay,
                        focRes=self.foc.resolution, 
                        moduleName=args.criteria, 
                        ev=self.ev, 
                        rt=rt, 
                        logger=self.logger)

                    rFt, rMns=anr.selectAndAnalyze()
                else:
                    anr= an.SimpleAnalysis(
                        dataSxtr=dataSxtr, 
                        Ds9Display=self.args.Ds9Display, 
                        FitDisplay=self.args.FitDisplay, 
                        xdisplay = self.xdisplay,
                        ftwName=ftw.name, 
                        ftName=ft.name, 
                        focRes=self.foc.resolution, 
                        ev=self.ev, 
                        logger=self.logger)

                    rFt, rMns= anr.analyze()

                # save them for filter offset calucaltion
                rFts.append(rFt)

                if rFt.fitFlag:
                    anr.display()

                if rFt.extrFitPos:
                    self.logger.info('Focus: {0:5.0f}: minFitPos,            filter wheel:{1}, filter:{2}'.format(rFt.extrFitPos, ftw.name, ft.name))
                    self.logger.info('Focus: {0:5.2f}: minFitFwhm,           filter wheel:{1}, filter:{2}'.format(rFt.extrFitVal, ftw.name, ft.name))
                else:
                    self.logger.warn('Focus: no fitted minimum found')

                # say something
                rMns.logWeightedMeans(ftw=ftw, ft=ft)

                # if in self.rt.cfg['EMPTY_SLOT_NAMES']
                # or
                # blind
                if ft.name in self.rt.cfg['EMPTY_SLOT_NAMES']:

                    if self.args.blind:
                        self.foc.focDef= rMns.combined
                    else:
                        self.foc.focDef= rFt.extrFitPos
                    if self.debug: self.logger.debug('Focus: set self.foc.focDef: {0}'.format(int(self.foc.focDef)))

                    # FOC_DEF (is set first)
                    if self.rt.cfg['SET_FOC_DEF']:
                        # ToDo the correct values are stored in Focuser() object
                        if self.rt.cfg['FWHM_MIN'] < rFt.extrFitVal < self.rt.cfg['FWHM_MAX']:
                            self.foc.focDef=rFt.extrFitPos
                            acqu.writeFocDef()
                        else:
                            self.logger.warn('Focus: not writing FOC_DEF: {0}, minFitFwhm: {1}, out of bounds: {2},{3} (FWHM_MIN,FWHM_MAX)'.format(int(rFt.extrFitPos), rFt.extrFitVal, self.rt.cfg['FWHM_MIN'], self.rt.cfg['FWHM_MAX']))
                    else:
                        # ToDo subtract filter offset and set FOC_DEF
                        self.logger.info('Focus: filter: {0} not setting FOC_DEF (see configuration)'.format(ft.name))


                if self.debug: self.logger.debug('Focus: end filter wheel: {}, filter:{}'.format(ftw.name, ft.name))
            else:
                # for a given filter wheel, all filters processed
                # no incomplete set of offsets are written
                if self.rt.cfg['WRITE_FILTER_OFFSETS']:
                    acqu.writeOffsets(ftw=ftw)
            #
            # log filter offsets
            if self.foc.focDef is not None:
                for ft in ftw.filters:
                    for rFt in rFts:
                        if ft.name == rFt.ftName:
                            ft.OffsetToEmptySlot=int(rFt.extrFitPos- self.foc.focDef)
                            self.logger.info('Focus: ft: {0}, set ft.OffsetToEmptySlot: {1}'.format(ft.name, int(ft.OffsetToEmptySlot)))
                            break
                    else:
                        self.logger.warn('Focus: ft: {0}, not found in result, rFt.ftName: {1}, that is ok if this is a unittest run)'.format(ft.name, rFt.ftName))

            elif len(ftw.filters) > 1:
                self.logger.warn('Focus: found ft: {0} but no empty slot'.format(ft.name))
            elif len(ftw.filters) == 1:
                if self.debug: self.logger.debug('Focus: found ft: {0}'.format(fr.name))
            else:
                self.logger.warn('Focus: neither self.foc.focDef (empty slot), nor non empty slots found'.format(ft.name))

            if self.debug: self.logger.debug('Focus: end filter wheel: {}'.format(ftw.name))
        else:
            if self.debug: self.logger.debug('Focus: all filter wheels done')

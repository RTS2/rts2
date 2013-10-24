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

import Queue
import sys

import rts2saf.acquire as acq
import rts2saf.sextract as sx
import rts2saf.analyze as  an

class Focus(object):
    def __init__(self, debug=False, proxy=None, args=None, dryFitsFiles=None, ccd=None, foc=None, ftws=None, rt=None, ev=None, logger=None):
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

    def run(self):
        # loop over filter wheels, their filters and offsets (FOC_TOFF)
        for k, ftw in enumerate(self.ftws):
            # only interesting in case multiple filter wheels are present
            if len(ftw.filters) ==1 and k>0:
                # self.ftws is sorted
                # these are filter wheels which have no real filters (defined in config) 
                # they must appear in self.ftws in order to set the empty slot
                continue
        
            for ft in ftw.filters:
                if self.debug: self.logger.debug('Focus: start filter wheel: {}, filter:{}'.format(ftw.name, ft.name))

                dFF=None
                if self.dryFitsFiles:
                    dFF=self.dryFitsFiles[:]
                    self.logger.info('Focus: using dry FITS files')
                    if self.debug: self.logger.debug('Focus: using dry FITS files from: {}'.format(self.dryFitsFiles))
                # acquisition
                acqu_oq = Queue.Queue()
                #
                acqu= acq.Acquire(debug=self.debug, proxy=self.proxy, dryFitsFiles=dFF, ftw=ftw, ft=ft, foc=self.foc, ccd=self.ccd, filterWheelsInUse=self.ftws, acqu_oq=acqu_oq, rt=self.rt, ev=self.ev, logger=self.logger)
                # 
                # steps are defined per filter, if blind in focuser
                if not self.args.blind:
                    self.foc.focFoff=ft.focFoff

                # start acquisition thread
                if not acqu.startScan(exposure=self.args.exposure, blind=self.args.blind):
                    self.logger.error('Focus: exiting')
                    sys.exit(1)

                # acquire FITS from thread
                dataSex=list()
                for st in self.foc.focFoff:
                    while True:
                        try:
                            fitsFn= acqu_oq.get(block=True, timeout=.2)
                        except Queue.Empty:
                            # if self.debug: self.logger.debug('Focus: continue')
                            continue
                    
                        sxtr= sx.Sextract(debug=self.debug,rt=self.rt,logger=self.logger)
                        dSx=sxtr.sextract(fitsFn=fitsFn)
                        if dSx.fitsFn==None:
                            self.logger.error('Focus: sextractor failed on fits file: {0}'.format(fitsFn))
                            # ToDo get out of the loop if MINIMUM_FOCUSER_POSITIONS can not be achieved
                            self.logger.error('Focus: try to continue')
                            break

                        self.logger.info('Focus: pos: {0:5d}, objects: {1:4d}, file: {2}'.format(int(st), len(dSx.catalog), fitsFn))
                        dataSex.append(dSx)
                        break
                else:
                    if self.debug: self.logger.debug('Focus: got all images')

                acqu.stopScan(timeout=1.)

                pos=dict()
                # ToDO might be not pythonic
                for dSx in dataSex:
                    try:
                        pos[dSx.focPos] += 1
                    except:
                        pos[dSx.focPos] = 1

                if len(pos) <= self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
                    self.logger.warn('Focus: to few DIFFERENT focuser positions: {0}<={1} (see MINIMUM_FOCUSER_POSITIONS), continuing'.format(len(pos), self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']))
                    if self.debug:
                        for p,v in pos.iteritems():
                            self.logger.debug('Focus:{0:5.0f}: {1}'.format(p,v))
                    continue

                # might go to a thread too
                anr= an.SimpleAnalysis(dataSex=dataSex, displayDs9=self.args.displayDs9, displayFit=self.args.displayFit, ftwName=ftw.name, ftName=ft.name, dryFits=True, focRes=self.foc.resolution, ev=self.ev, logger=self.logger)

                rFt= anr.analyze()
                # 
                anr.display()

                if rFt.weightedMeanObjects:
                    self.logger.info('Focus: {0:5.0f}: weightmedMeanObjects, filter wheel:{1}, filter:{2}'.format(rFt.weightedMeanObjects, ftw.name, ft.name))
                if rFt.weightedMeanFwhm:
                    self.logger.info('Focus: {0:5.0f}: weightedMeanFwhm,     filter wheel:{1}, filter:{2}'.format(rFt.weightedMeanFwhm, ftw.name, ft.name))

                if rFt.weightedMeanStdFwhm:
                    self.logger.info('Focus: {0:5.0f}: weightedMeanStdFwhm,  filter wheel:{1}, filter:{2}'.format(rFt.weightedMeanStdFwhm, ftw.name, ft.name))

                if rFt.weightedMeanCombined:
                    self.logger.info('Focus: {0:5.0f}: weightedMeanCombined, filter wheel:{1}, filter:{2}'.format(rFt.weightedMeanCombined, ftw.name, ft.name))

                if rFt.minFitPos:
                    self.logger.info('Focus: {0:5.0f}: minFitPos,            filter wheel:{1}, filter:{2}'.format(rFt.minFitPos, ftw.name, ft.name))
                    self.logger.info('Focus: {0:5.2f}: minFitFwhm,           filter wheel:{1}, filter:{2}'.format(rFt.minFitFwhm, ftw.name, ft.name))
                else:
                    self.logger.warn('Focus: no fitted minimum found')

                # if in self.rt.cfg['EMPTY_SLOT_NAMES']
                # or
                # blind
                if ft.name in self.rt.cfg['EMPTY_SLOT_NAMES']:

                    if self.args.blind:
                        self.foc.focDef= rFt.weightedMeanCombined
                    else:
                        self.foc.focDef= rFt.minFitPos
                    if self.debug: self.logger.debug('Focus: set self.foc.focDef: {0}'.format(int(self.foc.focDef)))

                    # FOC_DEF (is set first)
                    if self.rt.cfg['SET_FOC_DEF']:
                        # ToDo th correct values are stored in Focuser() object
                        if self.rt.cfg['FWHM_MIN'] < rFt.minFitFwhm < self.rt.cfg['FWHM_MAX']:
                            acqu.writeFocDef()
                            self.logger.info('Focus: set FOC_DEF: {0}'.format(int(rFt.minFitPos)))
                        else:
                            self.logger.warn('Focus: not writing FOC_DEF: {0}, minFitFwhm: {1}, out of bounds: {2},{3} (FWHM_MIN,FWHM_MAX)'.format(int(rFt.minFitPos), rFt.minFitFwhm, self.rt.cfg['FWHM_MIN'], self.rt.cfg['FWHM_MAX']))
                    else:
                        # ToDo subtract filter offset and set FOC_DEF
                        self.logger.info('Focus: filter: {0} not setting FOC_DEF (see configuration)'.format(ft.name))

                else:
                    ft.OffsetToEmptySlot=int(rFt.minFitPos- self.foc.focDef)
                    if self.debug: self.logger.debug('Focus: set ft.OffsetToEmptySlot: {0}'.format(int(ft.OffsetToEmptySlot)))

                if self.debug: self.logger.debug('Focus: end filter wheel: {}, filter:{}'.format(ftw.name, ft.name))
            else:
                # no incomplete set of offsets are written
                if self.rt.cfg['WRITE_FILTER_OFFSETS']:
                    acqu.writeOffsets(ftw=ftw)

            if self.debug: self.logger.debug('Focus: end filter wheel: {}'.format(ftw.name))
        else:
            if self.debug: self.logger.debug('Focus: all filter wheels done')

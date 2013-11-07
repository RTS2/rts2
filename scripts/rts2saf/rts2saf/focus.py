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
import collections
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
                acqu= acq.Acquire(debug=self.debug, proxy=self.proxy, dryFitsFiles=dFF, ftw=ftw, ft=ft, foc=self.foc, ccd=self.ccd, ftws=self.ftws, acqu_oq=acqu_oq, rt=self.rt, ev=self.ev, logger=self.logger)
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
                            # ToDo get out of the loop if MINIMUM_FOCUSER_POSITIONS can not be achieved any more
                            break

                        self.logger.info('Focus: pos: {0:5d}, objects: {1:4d}, file: {2}'.format(int(st), len(dSx.catalog), fitsFn))
                        dataSex.append(dSx)
                        break
                else:
                    if self.debug: self.logger.debug('Focus: got all images')

                acqu.stopScan(timeout=1.)
                
                pos=collections.defaultdict(int)
                for dSx in dataSex:
                    pos[dSx.focPos] += 1

                if len(pos) <= self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
                    self.logger.warn('Focus: to few DIFFERENT focuser positions: {0}<={1} (see MINIMUM_FOCUSER_POSITIONS), continuing'.format(len(pos), self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']))
                    if self.debug:
                        for p,v in pos.iteritems():
                            self.logger.debug('Focus:{0:5.0f}: {1}'.format(p,v))
                    continue

                # might go to a thread too
                if self.args.catalogAnalysis:
                    anr=CatalogAnalysis(debug=self.debug, dataSex=dataSex, Ds9Display=self.args.Ds9Display, FitDisplay=self.args.FitDisplay, focRes=self.foc.resolution, moduleName=args.criteria, ev=self.ev, rt=rt, logger=self.logger)
                    rFt, rMns=anr.selectAndAnalyze()
                else:
                    anr= an.SimpleAnalysis(dataSex=dataSex, Ds9Display=self.args.Ds9Display, FitDisplay=self.args.FitDisplay, ftwName=ftw.name, ftName=ft.name, dryFits=True, focRes=self.foc.resolution, ev=self.ev, logger=self.logger)
                    rFt, rMns= anr.analyze()
                # 
                anr.display()

                if rMns.objects:
                    self.logger.info('Focus: {0:5.0f}: weightmedMeanObjects, filter wheel:{1}, filter:{2}'.format(rMns.objects, ftw.name, ft.name))
                if rMns.val:
                    self.logger.info('Focus: {0:5.0f}: weightedMeanFwhm,     filter wheel:{1}, filter:{2}'.format(rMns.val, ftw.name, ft.name))

                if rMns.stdVal:
                    self.logger.info('Focus: {0:5.0f}: weightedMeanStdFwhm,  filter wheel:{1}, filter:{2}'.format(rMns.stdVal, ftw.name, ft.name))

                if rMns.combined:
                    self.logger.info('Focus: {0:5.0f}: weightedMeanCombined, filter wheel:{1}, filter:{2}'.format(rMns.combined, ftw.name, ft.name))

                if rFt.extrFitPos:
                    self.logger.info('Focus: {0:5.0f}: minFitPos,            filter wheel:{1}, filter:{2}'.format(rFt.extrFitPos, ftw.name, ft.name))
                    self.logger.info('Focus: {0:5.2f}: minFitFwhm,           filter wheel:{1}, filter:{2}'.format(rFt.extrFitVal, ftw.name, ft.name))
                else:
                    self.logger.warn('Focus: no fitted minimum found')

                # if in self.rt.cfg['EMPTY_SLOT_NAMES']
                # or
                # blind
                if ft.name in self.rt.cfg['EMPTY_SLOT_NAMES']:

                    if self.args.blind:
                        self.foc.focDef= rFt.weightedMeanCombined
                    else:
                        self.foc.focDef= rFt.extrFitPos
                    if self.debug: self.logger.debug('Focus: set self.foc.focDef: {0}'.format(int(self.foc.focDef)))

                    # FOC_DEF (is set first)
                    if self.rt.cfg['SET_FOC_DEF']:
                        # ToDo th correct values are stored in Focuser() object
                        if self.rt.cfg['FWHM_MIN'] < rFt.minFitFwhm < self.rt.cfg['FWHM_MAX']:
                            acqu.writeFocDef()
                            self.logger.info('Focus: set FOC_DEF: {0}'.format(int(rFt.extrFitPos)))
                        else:
                            self.logger.warn('Focus: not writing FOC_DEF: {0}, minFitFwhm: {1}, out of bounds: {2},{3} (FWHM_MIN,FWHM_MAX)'.format(int(rFt.extrFitPos), rFt.minFitFwhm, self.rt.cfg['FWHM_MIN'], self.rt.cfg['FWHM_MAX']))
                    else:
                        # ToDo subtract filter offset and set FOC_DEF
                        self.logger.info('Focus: filter: {0} not setting FOC_DEF (see configuration)'.format(ft.name))

                else:
                    ft.OffsetToEmptySlot=int(rFt.extrFitPos- self.foc.focDef)
                    if self.debug: self.logger.debug('Focus: set ft.OffsetToEmptySlot: {0}'.format(int(ft.OffsetToEmptySlot)))

                if self.debug: self.logger.debug('Focus: end filter wheel: {}, filter:{}'.format(ftw.name, ft.name))
            else:
                # no incomplete set of offsets are written
                if self.rt.cfg['WRITE_FILTER_OFFSETS']:
                    acqu.writeOffsets(ftw=ftw)

            if self.debug: self.logger.debug('Focus: end filter wheel: {}'.format(ftw.name))
        else:
            if self.debug: self.logger.debug('Focus: all filter wheels done')

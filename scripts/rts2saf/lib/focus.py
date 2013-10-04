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

import lib.acquire as acq
import lib.sextract as sx
import lib.analyze as  an

# ToDo this class is bad by design (cut/copy/paste)
# create a method wich can be called by class FocusFilterWheels
class Focus(object):
    def __init__(self, debug=False, args=None, dryFitsFiles=None, rt=None, ev=None, logger=None):
        self.debug=debug
        self.args=args
        self.dryFitsFiles=dryFitsFiles 
        self.ev=ev
        self.rt=rt
        self.logger=logger
        
    def run(self):
        ftw=None
        ft=None
        dFF=None
        if self.dryFitsFiles:
            dFF=self.dryFitsFiles[:]
            self.logger.info('Focus: using dry FITS files')
            if self.debug: self.logger.debug('Focus: using dry FITS files from: {}'.format(self.dryFitsFiles))

        # acquisition
        acqu_oq = Queue.Queue()
        acqu= acq.Acquire(debug=self.debug, dryFitsFiles=dFF, ftw=ftw, ft=ft, foc=self.rt.foc, ccd=self.rt.ccd, filterWheelsInUse=self.rt.filterWheelsInUse, acqu_oq=acqu_oq, rt=self.rt, ev=self.ev, logger=self.logger)

        if self.args.focDef: 
            self.rt.foc.focDef=args.focDef
            acqu.writeFocDef()

        # start acquisition thread
        if not acqu.startScan(exposure=self.args.exposure, blind=self.args.blind):
            self.logger.error('Focus: exiting')
            sys.exit(1)

        # acquire FITS
        dataSex=dict()
        # do not store dSx==None
        i=0
        for st in self.rt.foc.focFoff:
            while True:
                try:
                    fitsFn= acqu_oq.get(block=True, timeout=.2)
                except Queue.Empty:
                    # if self.debug: self.logger.debug('Focus: continue')
                    continue
                    
                sxtr= sx.Sextract(debug=self.debug,rt=self.rt,logger=self.logger)
                dSx=sxtr.sextract(fitsFn=fitsFn)
                if dSx==None:
                    self.logger.error('Focus: sextractor failed on fits file: {0}'.format(fitsFn))
                    # ToDo get out of the loop if MINIMUM_FOCUSER_POSITIONS can not be achieved
                    self.logger.error('Focus: try to continue')
                    break

                self.logger.info('Focus: pos: {0:5d}, objects: {1:4d} sextracted FITS file: {2}'.format(int(st), len(dSx.catalog), fitsFn))
                dataSex[i]=dSx
                i += 1
                break
            else:
                if self.debug: self.logger.debug('Focus: got all images')

        acqu.stopScan(timeout=1.)

        # check the number of different positions
        pos=dict()
        # ToDO might be not pythonic
        for cnt in dataSex.keys():
            try:
                pos[dataSex[cnt].focPos] += 1
            except:
                pos[dataSex[cnt].focPos] = 1

        if len(pos) <= self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
            self.logger.warn('Focus: to few DIFFERENT focuser positions: {0}<={1} (see MINIMUM_FOCUSER_POSITIONS), returning'.format(len(pos), self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']))
            if self.debug:
                for p,v in pos.iteritems():
                     self.logger.debug('Focus:{0:5.0f}: {1}'.format(p,v))
            return

        # might go to thread too
        if self.dryFitsFiles:
            anr= an.SimpleAnalysis(dataSex=dataSex, displayDs9=True, displayFit=True, ftwName=None, ftName=None, dryFits=True, focRes=self.rt.foc.resolution, ev=self.ev, logger=self.logger)
        else:
            anr= an.SimpleAnalysis(dataSex=dataSex, displayDs9=False, displayFit=False, ftwName=None, ftName=None, dryFits=False, focRes=self.rt.foc.resolution, ev=self.ev, logger=self.logger)

        rFt= anr.analyze()

        if rFt.weightedMeanObjects:
            self.logger.info('FocusFilterWheels: {0:5.0f}: weightmedMeanObjects'.format(rFt.weightedMeanObjects))
        if rFt.weightedMeanFwhm:
            self.logger.info('FocusFilterWheels: {0:5.0f}: weightedMeanFwhm'.format(rFt.weightedMeanFwhm))

        if rFt.weightedMeanStdFwhm:
            self.logger.info('FocusFilterWheels: {0:5.0f}: weightedMeanStdFwhm'.format(rFt.weightedMeanStdFwhm))

        if rFt.weightedMeanCombined:
            self.logger.info('FocusFilterWheels: {0:5.0f}: weightedMeanCombined'.format(rFt.weightedMeanCombined))

        if rFt.minFitPos:
            self.logger.info('FocusFilterWheels: {0:5.0f}: minFitPos'.format(rFt.minFitPos))
            self.logger.info('FocusFilterWheels: {0:5.2f}: minFitFwhm'.format(rFt.minFitFwhm))


        # currently write FOC_DEF only in case fit converged
        if rFt.minFitPos:
            self.logger.info('Focus: {0:5.0f}: minFitPos'.format(rFt.minFitPos))
            self.logger.info('Focus: {0:5.2f}: minFitFwhm'.format(rFt.minFitFwhm))
            # FOC_DEF (is set first)
            self.rt.foc.focDef= rFt.minFitPos
            if self.rt.cfg['SET_FOCUS']:
                if self.rt.cfg['FWHM_MIN'] < rFt.minFitFwhm < self.rt.cfg['FWHM_MAX']:
                    acqu.writeFocDef()
                    self.logger.info('Focus: set FOC_DEF: {0}'.format(int(rFt.minFitPos)))
                else:
                    self.logger.warn('Focus: not writing FOC_DEF: {0}, minFitFwhm: {1}, out of bounds: {2},{3}'.format(int(rFt.minFitPos), rFt.minFitFwhm, self.rt.cfg['FWHM_MIN'], self.rt.cfg['FWHM_MAX']))
            else:
                self.logger.warn('Focus: not writing FOC_DEF: {0}'.format(int(rFt.minFitPos)))
        else:
            self.logger.warn('Focus: no fitted minimum found')


class FocusFilterWheels(object):
    def __init__(self, debug=False, args=None, dryFitsFiles=None, rt=None, ev=None, logger=None):
        self.debug=debug
        self.args=args
        self.dryFitsFiles=dryFitsFiles
        self.ev=ev
        self.rt=rt
        self.logger=logger

    def run(self):
        # loop over filter wheels, their filters and offsets (FOC_TOFF)
        for k, ftw in enumerate(self.rt.filterWheelsInUse):
            # only interesting in case multiple filter wheels are present
            if len(ftw.filters) ==1 and k>0:
                # self.rt.filterWheelsInUse is sorted
                # these are filter wheels which have no real filters (defined in config) 
                # they must appear in self.rt.filterWheelsInUse in order to set the empty slot
                continue
        
            for ft in ftw.filters:
                if self.debug: self.logger.debug('FocusFilterWheels: start filter wheel: {}, filter:{}'.format(ftw.name, ft.name))

                dFF=None
                if self.dryFitsFiles:
                    dFF=self.dryFitsFiles[:]
                    self.logger.info('FocusFilterWheels: using dry FITS files')
                    if self.debug: self.logger.debug('FocusFilterWheels: using dry FITS files from: {}'.format(self.dryFitsFiles))
                # acquisition
                acqu_oq = Queue.Queue()
                #
                acqu= acq.Acquire(debug=self.debug, dryFitsFiles=dFF, ftw=ftw, ft=ft, foc=self.rt.foc, ccd=self.rt.ccd, filterWheelsInUse=self.rt.filterWheelsInUse, acqu_oq=acqu_oq, rt=self.rt, ev=self.ev, logger=self.logger)
                # 
                if self.args.focDef: #  ToDo think about that, it is a bit misplaced
                    self.rt.foc.focDef=args.focDef
                    acqu.writeFocDef()
                # steps are defined per filter, if blind in focuser
                if not self.args.blind:
                    self.rt.foc.focFoff=ft.focFoff

                # start acquisition thread
                if not acqu.startScan(exposure=self.args.exposure, blind=self.args.blind):
                    self.logger.error('FocusFilterWheels: exiting')
                    sys.exit(1)

                # acquire FITS
                dataSex=dict()
                i=0
                for st in self.rt.foc.focFoff:
                    while True:
                        try:
                            fitsFn= acqu_oq.get(block=True, timeout=.2)
                        except Queue.Empty:
                            # if self.debug: self.logger.debug('FocusFilterWheels: continue')
                            continue
                    
                        sxtr= sx.Sextract(debug=self.debug,rt=self.rt,logger=self.logger)
                        dSx=sxtr.sextract(fitsFn=fitsFn)
                        if dSx==None:
                            self.logger.error('FocusFilterWheels: sextractor failed on fits file: {0}'.format(fitsFn))
                            # ToDo get out of the loop if MINIMUM_FOCUSER_POSITIONS can not be achieved
                            self.logger.error('FocusFilterWheels: try to continue')
                            break

                        self.logger.info('FocusFilterWheels: pos: {0:5d}, objects: {1:4d}, sextracted FITS file: {2}'.format(int(st), len(dSx.catalog), fitsFn))
                        dataSex[i]=dSx
                        i += 1
                        break
                else:
                    if self.debug: self.logger.debug('FocusFilterWheels: got all images')

                acqu.stopScan(timeout=1.)

                pos=dict()
                # ToDO might be not pythonic
                for cnt in dataSex.keys():
                    try:
                        pos[dataSex[cnt].focPos] += 1
                    except:
                        pos[dataSex[cnt].focPos] = 1

                if len(pos) <= self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
                    self.logger.warn('FocusFilterWheels: to few DIFFERENT focuser positions: {0}<={1} (see MINIMUM_FOCUSER_POSITIONS), continuing'.format(len(pos), self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']))
                    if self.debug:
                        for p,v in pos.iteritems():
                            self.logger.debug('FocusFilterWheels:{0:5.0f}: {1}'.format(p,v))
                    continue

                # might go to a thread too
                if self.dryFitsFiles:
                    anr= an.SimpleAnalysis(dataSex=dataSex, displayDs9=True, displayFit=True, ftwName=ftw.name, ftName=ft.name, dryFits=True, focRes=self.rt.foc.resolution, ev=self.ev, logger=self.logger)
                else:
                    anr= an.SimpleAnalysis(dataSex=dataSex, displayDs9=False, displayFit=False, ftwName=ftw.name, ftName=ft.name, dryFits=False, focRes=self.rt.foc.resolution, ev=self.ev, logger=self.logger)

                rFt= anr.analyze()

                if rFt.weightedMeanObjects:
                    self.logger.info('FocusFilterWheels: {0:5.0f}: weightmedMeanObjects, filter wheel:{1}, filter:{2}'.format(rFt.weightedMeanObjects, ftw.name, ft.name))
                if rFt.weightedMeanFwhm:
                    self.logger.info('FocusFilterWheels: {0:5.0f}: weightedMeanFwhm, filter wheel:{1}, filter:{2}'.format(rFt.weightedMeanFwhm, ftw.name, ft.name))

                if rFt.weightedMeanStdFwhm:
                    self.logger.info('FocusFilterWheels: {0:5.0f}: weightedMeanStdFwhm, filter wheel:{1}, filter:{2}'.format(rFt.weightedMeanStdFwhm, ftw.name, ft.name))

                if rFt.weightedMeanCombined:
                    self.logger.info('FocusFilterWheels: {0:5.0f}: weightedMeanCombined, filter wheel:{1}, filter:{2}'.format(rFt.weightedMeanCombined, ftw.name, ft.name))

                if rFt.minFitPos:
                    self.logger.info('FocusFilterWheels: {0:5.0f}: minFitPos, filter wheel:{1}, filter:{2}'.format(rFt.minFitPo, ftw.name, ft.names))
                    self.logger.info('FocusFilterWheels: {0:5.2f}: minFitFwhm, filter wheel:{1}, filter:{2}'.format(rFt.minFitFwhm, ftw.name, ft.name))

                    self.rt.foc.focDef= rFt.minFitPos
                    # ToDo better criteria is EMPTY_SLOT_NAMES
                    if ft.OffsetToEmptySlot == 0.:
                        # FOC_DEF (is set first)
                        if self.rt.cfg['SET_FOCUS']:
                            if self.rt.cfg['FWHM_MIN'] < rFt.minFitFwhm < self.rt.cfg['FWHM_MAX']:
                                acqu.writeFocDef()
                                self.logger.info('FocusFilterWheels: set FOC_DEF: {0}'.format(int(rFt.minFitPos)))
                            else:
                                self.logger.warn('FocusFilterWheels: not writing FOC_DEF: {0}, minFitFwhm: {1}, out of bounds: {2},{3}'.format(int(rFt.minFitPos), rFt.minFitFwhm, self.rt.cfg['FWHM_MIN'], self.rt.cfg['FWHM_MAX']))
                        else:
                            self.logger.warn('FocusFilterWheels: not writing FOC_DEF: {0}'.format(int(rFt.minFitPos)))
                    else:
                        self.logger.info('FocusFilterWheels: filter: {0} has an offset: {1}, not setting FOC_DEF'.format(ft.name, int(rFt.minFitPos- self.rt.foc.focDef)))
                        ft.OffsetToEmptySlot=int(rFt.minFitPos- self.rt.foc.focDef)
                else:
                    self.logger.warn('FocusFilterWheels: no fitted minimum found')

                if self.debug: self.logger.debug('FocusFilterWheels: end filter wheel: {}, filter:{}'.format(ftw.name, ft.name))
            else:
                # no incomplete set of offsets are written
                if self.rt.cfg['WRITE_FILTER_OFFSETS']:
                    acqu.writeOffsets(ftw=ftw)

            if self.debug: self.logger.debug('FocusFilterWheels: end filter wheel: {}'.format(ftw.name))
        else:
            if self.debug: self.logger.debug('FocusFilterWheels: all filter wheels done')

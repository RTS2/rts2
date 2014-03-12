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

""" AnalyzeRuns performs the offline analysis of many runs.
"""

__author__ = 'markus.wildi@bluewin.ch'


import fnmatch
import os
import re
import sys

from rts2saf.analyze import SimpleAnalysis, CatalogAnalysis
from rts2saf.datarun import DataRun

# thanks http://stackoverflow.com/questions/635483/what-is-the-best-way-to-implement-nested-dictionaries-in-python
class AutoVivification(dict):
    """Implementation of perl's autovivification feature."""
    def __getitem__(self, item):
        try:
            return dict.__getitem__(self, item)
        except KeyError:
            value = self[item] = type(self)()
            return value

# attempt to group sub directories into focus runs
# goal calculate filter offsets
#
#2013-09-04T22:18:35.077526/fliterwheel/filter
#                                                                                                         date    ftw  ft
MANYFILTERWHEELS = re.compile('.*/([0-9]{4,4}-[0-9]{2,2}-[0-9]{2,2}T[0-9]{2,2}\:[0-9]{2,2}\:[0-9]{2,2}\.[0-9]+)/(.+?)/(.+)')
#                                                                                                  date    ft
ONEFILTER = re.compile('.*/([0-9]{4,4}-[0-9]{2,2}-[0-9]{2,2}T[0-9]{2,2}\:[0-9]{2,2}\:[0-9]{2,2}\.[0-9]+)/(.+?)')
# no filter
#2013-09-04T22:18:35.077526/
DATE = re.compile('.*/([0-9]{4,4}-[0-9]{2,2}-[0-9]{2,2}T[0-9]{2,2}\:[0-9]{2,2}\:[0-9]{2,2}\.[0-9]+)')

class AnalyzeRuns(object):
    """AnalyzeRuns analize focus runs in a subdirectory.

    :var debug: enable more debug output with --debug and --level
    :var basePath: where the FITS files are stored 
    :var args: command line arguments or their defaults
    :var rt: run time configuration usually read from /usr/local/etc/rts2/rts2saf/rts2saf.cfg
    :var ev: helper module for house keeping
    :var logger:  logger object
    :var fs: contains the subdirectory structure and the associated FITS files

    """

    def __init__(self, debug=False, basePath=None, args=None, rt=None, ev=None, logger=None, xdisplay = None):

        self.debug = debug
        self.basePath = basePath
        self.args = args
        self.rt = rt
        self.ev = ev
        self.logger = logger
        self.xdisplay = xdisplay
        self.fS = AutoVivification()

    def analyzeRun(self, fitsFns = None):
        """Analyze a set of FITS files having all the same filter set  forming a 
        single focus run.


        :param fitsFns:  list of FITS file names. 
        :type fitsFns: list of strings
        :rtype: ResultFit and ResultMeans.

        """

        # define the reference FITS as the one with the most sextracted objects
        # ToDo may be weighted means
        dataRnR = DataRun(debug = self.debug, args = self.args, rt = self.rt, logger = self.logger)
        dataRnR.sextractLoop(fitsFns = fitsFns)

        if len(dataRnR.dataSxtrs)==0:
            self.logger.warn('analyzeRun: no results for files: {}'.format(fitsFns))
            return None, None

        dataRnR.dataSxtrs.sort(key = lambda x: x.nObjs, reverse = True)
        fitsFnR = dataRnR.dataSxtrs[0].fitsFn
        self.logger.info('analyzeRun: reference at FOC_POS: {0}, {1}'.format(dataRnR.dataSxtrs[0].focPos, fitsFnR))

        dSxR = dataRnR.createAssocList(fitsFn = fitsFnR)
        if self.args.associate:
            dataRn = DataRun(debug = self.debug, dSxReference = dSxR, args = self.args, rt = self.rt, logger = self.logger)
        else:
            dataRn = DataRun(debug = self.debug, args = self.args, rt = self.rt, logger = self.logger)

        dataRn.sextractLoop(fitsFns = fitsFns)

        if not dataRn.numberOfFocPos():
            return None, None 

        if self.args.associate:
            dataRn.onAlmostImagesAssoc()
        else:
            # the reference is needed for option --fraction
            dataRn.dSxReference=dSxR
            dataRn.onAlmostImages()

        if len(dataRn.dataSxtrs) <= self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
            self.logger.warn('analyzeRuns: to few DIFFERENT focuser positions: {0}<={1} (see MINIMUM_FOCUSER_POSITIONS), continuing'.format(len(dataRn.dataSxtrs), self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']))
            return None, None

        date = dataRn.dataSxtrs[0].date.split('.')[0]
        if self.args.catalogAnalysis:
            an = CatalogAnalysis(
                debug = self.debug, 
                date = date, 
                dataSxtr = dataRn.dataSxtrs, 
                Ds9Display = self.args.Ds9Display, 
                FitDisplay = self.args.FitDisplay, 
                xdisplay = self.xdisplay,
                focRes = float(self.rt.cfg['FOCUSER_RESOLUTION']), 
                moduleName = self.args.criteria, 
                ev=self.ev, 
                rt = self.rt, 
                logger = self.logger)
            arFt, rrFt, allrFt, arMns, rrMns, allrMns= an.selectAndAnalyze()
        else:
            an = SimpleAnalysis(
                debug = self.debug, 
                date = date, 
                dataSxtr = dataRn.dataSxtrs, 
                Ds9Display = self.args.Ds9Display, 
                FitDisplay = self.args.FitDisplay, 
                xdisplay = self.xdisplay,
                focRes = float(self.rt.cfg['FOCUSER_RESOLUTION']), 
                ev = self.ev, 
                logger = self.logger)

            arFt, arMns= an.analyze()
            #ToDo matplotlib issue
            if not self.args.model:
                if arFt.fitFlag:
                    an.display()

        if not arMns != None:
            arMns.logWeightedMeans()
        return arFt, arMns

    def analyzeRuns(self):
        """Loop over self.fs and call analyzeRun(). Calculate and log the filter offsets.

        :returns: ResultFit
        :rtype: list

        """
        rFts = list()
        for dftw, ft  in self.fS.iteritems():
            info = '{} :: '.format(repr(dftw))
            out = False
            # ToDo dictionary comprehension
            for k, fns in ft.iteritems():
                info += 'Ftname: {}, images: {}; '.format(k, len(fns))
                if len(fns) > self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
                    out = True
                    rFt, rMns = self.analyzeRun(fitsFns = fns)
                    if rFt != None and rFt.extrFitPos != None:
                        rFts.append(rFt)
                else:
                    self.logger.warn('analyzeRuns: to few DIFFERENT focuser positions: {0}<={1} (see MINIMUM_FOCUSER_POSITIONS), continuing'.format(len(fns), self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']))
            if out:
                self.logger.info( 'analyzeRuns: {}'.format(info))

        openMinFitPos = None
        for rFt in rFts:
            if rFt.ftName in self.rt.cfg['EMPTY_SLOT_NAMES']:
                openMinFitPos = int(rFt.extrFitPos)
                break
        else:
            # no empty slot found
            if len(rFts) > 1:
                self.logger.warn('analyzeRuns: found {} fitted data sets, but no empty slot, do not calculate filter offsets'.format(len(rFts)))

        for rFt in rFts:
            if rFt.extrFitPos is not None:
                if openMinFitPos is not None:
                    self.logger.info('analyzeRuns: {0:5d} minPos, {1:5d}  offset, {2} ftName'.format( int(rFt.extrFitPos), int(rFt.extrFitPos)-openMinFitPos, rFt.ftName.rjust(8, ' ')))
                else:
                    self.logger.info('analyzeRuns: {0:5d} minPos, {1} ftName'.format( int(rFt.extrFitPos),  rFt.ftName.rjust(8, ' ')))

                self.logger.info('analyzeRuns: fitPar {} '.format(rFt.fitPar ))

        return rFts

    def aggregateRuns(self):
        """Find FITS files which form a focus run and store them in self.fs."""
        for root, dirnames, filenames in os.walk(self.args.basePath):
            fns = fnmatch.filter(filenames, '*.fits')
            if len(fns)==0:
                continue
            mFtws =  MANYFILTERWHEELS.match(root)
            oFtw  =  ONEFILTER.match(root)
            d     =  DATE.match(root)
            FpFns = [os.path.join(root, fn) for fn in fns]
            if mFtws:

                if self.args.filterNames:
                    if mFtws.group(3) in self.args.filterNames: 
                        self.fS[(mFtws.group(1), mFtws.group(2))] [mFtws.group(3)] = FpFns
                else:
                    self.fS[(mFtws.group(1), mFtws.group(2))] [mFtws.group(3)] = FpFns
            elif oFtw:
                self.fS[(oFtw.group(1), 'NOFTW')] [oFtw.group(2)] = FpFns
            elif d:
                self.fS[(d.group(1), 'NOFTW')] ['NOFT'] = FpFns
            else:
                self.fS[('NODATE', 'NOFTW')] ['NOFT'] = FpFns

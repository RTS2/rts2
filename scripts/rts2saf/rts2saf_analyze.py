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

"""rts2saf_analyze.py performs the offline analysis of many runs and 
optionally creates a linear temperature model. class ``Do`` provides the environment.

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

class Do(object):
    """Do provides an environment for the analysis.

    :var debug: enable more debug output with --debug and --level
    :var basePath: where the FITS files are stored 
    :var args: commandline arguments or their defaults
    :var rt: run time configuration usually read from /usr/local/etc/rts2/rts2saf/rts2saf.cfg
    :var ev: helper module for house keeping
    :var logger:  logger object
    :var fs: contains the subdirectory structure and the associated FITS files

    """

    def __init__(self, debug=False, basePath=None, args=None, rt=None, ev=None, logger=None):

        self.debug = debug
        self.basePath = basePath
        self.args = args
        self.rt = rt
        self.ev = ev
        self.logger = logger
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
            an = CatalogAnalysis(debug = self.debug, date = date, dataSxtr = dataRn.dataSxtrs, Ds9Display = self.args.Ds9Display, FitDisplay = self.args.FitDisplay, focRes = float(self.rt.cfg['FOCUSER_RESOLUTION']), moduleName = self.args.criteria, ev=self.ev, rt = self.rt, logger = self.logger)
            rFt, rMns = an.selectAndAnalyze()
        else:
            an = SimpleAnalysis(debug = self.debug, date = date, dataSxtr = dataRn.dataSxtrs, Ds9Display = self.args.Ds9Display, FitDisplay = self.args.FitDisplay, focRes = float(self.rt.cfg['FOCUSER_RESOLUTION']), ev = self.ev, logger = self.logger)
            rFt, rMns = an.analyze()
            #ToDo matplotlib issue
            if not self.args.model:
                if rFt.fitFlag:
                    an.display()

        if not rMns != None:
            rMns.logWeightedMeans()
        return rFt, rMns

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
                info += '{} {} '.format(k, len(fns))
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
            # is not a filter wheel run 
            return rFts

        for rFt in rFts:
            if rFt.extrFitPos != None:
                self.logger.info('analyzeRuns: {0:5d} minPos, {1:5d}  offset, {2} ftName'.format( int(rFt.extrFitPos), int(rFt.extrFitPos)-openMinFitPos, rFt.ftName.rjust(8, ' ')))

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
            

if __name__ == '__main__':

    import argparse
    from rts2saf.config import Configuration
    from rts2saf.environ import Environment
    from rts2saf.log import Logger

    script=os.path.basename(__file__)
    parser =  argparse.ArgumentParser(prog = script, description = 'rts2asaf analysis')
    parser.add_argument('--debug', dest = 'debug', action = 'store_true', default = False, help = ': %(default)s,add more output')
    parser.add_argument('--sxdebug', dest = 'sxDebug', action = 'store_true', default = False, help = ': %(default)s,add more output on SExtract')
    parser.add_argument('--level', dest = 'level', default = 'INFO', help = ': %(default)s, debug level')
    parser.add_argument('--topath', dest = 'toPath', metavar = 'PATH', action = 'store', default = '.', help = ': %(default)s, write log file to path')
    parser.add_argument('--logfile', dest = 'logfile', default = '{0}.log'.format(script), help = ': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest = 'toconsole', action = 'store_true', default = False, help = ': %(default)s, log to console')
    parser.add_argument('--config', dest = 'config', action = 'store', default = '/usr/local/etc/rts2/rts2saf/rts2saf.cfg', help = ': %(default)s, configuration file path')
    parser.add_argument('--basepath', dest = 'basePath', action = 'store', default = None, help = ': %(default)s, directory where FITS images from possibly many focus runs are stored')
    parser.add_argument('--filternames', dest = 'filterNames', action = 'store', default = None, type = str, nargs = '+', help = ': %(default)s, list of filters to analyzed, None: all')
#ToDo    parser.add_argument('--ds9region', dest = 'ds9region', action = 'store_true', default = False, help = ': %(default)s, create ds9 region files')
    parser.add_argument('--ds9display', dest = 'Ds9Display', action = 'store_true', default = False, help = ': %(default)s, display fits images and region files')
    parser.add_argument('--fitdisplay', dest = 'FitDisplay', action = 'store_true', default = False, help = ': %(default)s, display fit')
    parser.add_argument('--cataloganalysis', dest = 'catalogAnalysis', action = 'store_true', default = False, help = ': %(default)s, ananlys is done with CatalogAnalysis')
    parser.add_argument('--criteria', dest = 'criteria', action = 'store', default = 'rts2saf.criteria_radius', help = ': %(default)s, CatalogAnalysis criteria Python module to load at run time')
    parser.add_argument('--associate', dest = 'associate', action = 'store_true', default = False, help = ': %(default)s, let sextractor associate the objects among images')
    parser.add_argument('--flux', dest = 'flux', action = 'store_true', default = False, help = ': %(default)s, do flux analysis')
    parser.add_argument('--model', dest = 'model', action = 'store_true', default = False, help = ': %(default)s, fit temperature model')
    parser.add_argument('--fraction', dest = 'fractObjs', action = 'store', default = 0.5, type = float, help = ': %(default)s, fraction of objects which must be present on each image, base: object number on reference image, this option is used only together with --associate')

    args = parser.parse_args()

    if args.debug:
        args.level =  'DEBUG'
        args.toconsole = True

    # logger
    logger =  Logger(debug = args.debug, args = args).logger # if you need to chage the log format do it here
    # config
    rtc = Configuration(logger = logger)
    rtc.readConfiguration(fileName = args.config)
    rtc.checkConfiguration()
    # environment
    ev = Environment(debug = args.debug, rt = rtc, logger = logger)

    if not args.basePath:
        parser.print_help()
        logger.warn('rts2saf_analyze: no --basepath specified')
        sys.exit(1)

    if not args.toconsole:
        print 'you may wish to enable logging to console --toconsole'
        print 'log file is writte to: {}'.format(args.logfile)


    do = Do(debug = args.debug, basePath = args.basePath, args = args, rt = rtc, ev = ev, logger = logger)
    do.aggregateRuns()
    if len(do.fS) == 0:
        logger.warn('rts2saf_analyze: exiting, no files found in basepath: {}'.format(args.basePath))
        sys.exit(1)

    rFf = do.analyzeRuns()
    
    if len(rFf)==0:
        logger.error('rts2saf_analyze: no results, exiting')
        sys.exit(1)

    if rFf[0].ambientTemp in 'NoTemp':
        logger.warn('rts2saf_analyze: no ambient temperature available in FITS files, no model fitted')
    else:
        if args.model:
            from rts2saf.temperaturemodel import TemperatureFocPosModel
            # imported here because otherwise I get a
            #
            ## This call to matplotlib.use() has no effect
            ## because the the backend has already been chosen;
            ## matplotlib.use() must be called *before* pylab, matplotlib.pyplot,
            ## or matplotlib.backends is imported for the first time.
            #
            # and indeed it was already called in fitsdisplay
            # matplotlib is a source of headache 
            

            # temperature model
            PLOTFN = ev.expandToPlotFileName( plotFn = '{}/temp-model.png'.format(args.basePath))
            dom = TemperatureFocPosModel(showPlot = True, date = ev.startTime[0:19],  comment = 'test run', plotFn = PLOTFN, resultFitFwhm = rFf, logger = logger)
            dom.fitData()
            dom.plotData()
            logger.info('rts2saf_analyze: storing plot at: {}'.format(PLOTFN))
        

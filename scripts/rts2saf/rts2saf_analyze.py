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

import os
import sys

from rts2saf.analyzeruns import AnalyzeRuns

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
    if not rtc.readConfiguration(fileName=args.config):
        logger.error('rts2saf_focus: exiting, wrong syntax, check the configuration file: {0}'.format(args.config))
        sys.exit(1)

    rtc.checkConfiguration(args=args)
    # environment
    ev = Environment(debug = args.debug, rt = rtc, logger = logger)

    if not args.basePath:
        parser.print_help()
        logger.warn('rts2saf_analyze: no --basepath specified')
        sys.exit(1)

    if not args.toconsole:
        print 'you may wish to enable logging to console --toconsole'
        print 'log file is written to: {}'.format(args.logfile)


    aRs = AnalyzeRuns(debug = args.debug, basePath = args.basePath, args = args, rt = rtc, ev = ev, logger = logger)
    aRs.aggregateRuns()
    if len(aRs.fS) == 0:
        logger.warn('rts2saf_analyze: exiting, no files found in basepath: {}'.format(args.basePath))
        sys.exit(1)

    rFf = aRs.analyzeRuns()
    
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
        

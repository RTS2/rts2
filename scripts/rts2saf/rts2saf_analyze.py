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


# if executed in background or without X Window plt.figure() fails
#
# http://stackoverflow.com/questions/1027894/detect-if-x11-is-available-python                                                                                    

import psutil
import matplotlib

XDISPLAY=None

import pkg_resources
p1, p2, p3= pkg_resources.get_distribution("psutil").version.split('.')

if int(p1) >= 2:
    pnm=psutil.Process(psutil.Process(os.getpid()).ppid()).name()
elif int(p1) >= 1:
    pnm=psutil.Process(psutil.Process(os.getpid()).ppid).name
# ok if it dies here

if 'init' in pnm or 'rts2-executor' in pnm:
    matplotlib.use('Agg')    
    XDISPLAY=False
else:
    from subprocess import Popen, PIPE
    p = Popen(["xset", "-q"], stdout=PIPE, stderr=PIPE)
    p.communicate()
    if p.returncode == 0:
        XDISPLAY=True
    else:
        matplotlib.use('Agg')    
        XDISPLAY=False


from rts2saf.analyzeruns import AnalyzeRuns
from rts2saf.temperaturemodel import TemperatureFocPosModel

if __name__ == '__main__':

    import argparse
    from rts2saf.config import Configuration
    from rts2saf.environ import Environment
    from rts2saf.log import Logger
    defaultCfg = '/usr/local/etc/rts2/rts2saf/rts2saf.cfg'
    script=os.path.basename(__file__)
    parser =  argparse.ArgumentParser(prog = script, description = 'rts2asaf analysis')
    parser.add_argument('--debug', dest = 'debug', action = 'store_true', default = False, help = ': %(default)s,add more output')
    parser.add_argument('--sxdebug', dest = 'sxDebug', action = 'store_true', default = False, help = ': %(default)s,add more output on SExtract')
    parser.add_argument('--level', dest = 'level', default = 'INFO', help = ': %(default)s, debug level')
    parser.add_argument('--topath', dest = 'toPath', metavar = 'PATH', action = 'store', default = '.', help = ': %(default)s, write log file to path')
    parser.add_argument('--logfile', dest = 'logfile', default = '{0}.log'.format(script), help = ': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest = 'toconsole', action = 'store_true', default = False, help = ': %(default)s, log to console')
    parser.add_argument('--config', dest = 'config', action = 'store', default = defaultCfg, help = ': %(default)s, configuration file path')
    parser.add_argument('--basepath', dest = 'basePath', action = 'store', default = None, help = ': %(default)s, directory where FITS images from possibly many focus runs are stored')
    parser.add_argument('--filternames', dest = 'filterNames', action = 'store', default = None, type = str, nargs = '+', help = ': %(default)s, list of SPACE separated filters to analyzed, None: all')
#ToDo    parser.add_argument('--ds9region', dest = 'ds9region', action = 'store_true', default = False, help = ': %(default)s, create ds9 region files')
    parser.add_argument('--ds9display', dest = 'Ds9Display', action = 'store_true', default = False, help = ': %(default)s, display fits images and region files')
    parser.add_argument('--fitdisplay', dest = 'FitDisplay', action = 'store_true', default = False, help = ': %(default)s, display fit')
    parser.add_argument('--cataloganalysis', dest = 'catalogAnalysis', action = 'store_true', default = False, help = ': %(default)s, ananlys is done with CatalogAnalysis')
    parser.add_argument('--criteria', dest = 'criteria', action = 'store', default = 'rts2saf.criteria_radius', help = ': %(default)s, CatalogAnalysis criteria Python module to load at run time')
    parser.add_argument('--associate', dest = 'associate', action = 'store_true', default = False, help = ': %(default)s, let sextractor associate the objects among images')
    parser.add_argument('--flux', dest = 'flux', action = 'store_true', default = False, help = ': %(default)s, do flux analysis')
    parser.add_argument('--model', dest = 'model', action = 'store_true', default = False, help = ': %(default)s, fit temperature model')
    parser.add_argument('--fraction', dest = 'fractObjs', action = 'store', default = 0.5, type = float, help = ': %(default)s, fraction of objects which must be present on each image, base: object number on reference image, this option is used only together with --associate')
    parser.add_argument('--emptySlots', dest = 'emptySlots', action = 'store', default = None, type = str, nargs = '+', help = ': %(default)s, list of SPACE separated names of the empty slots')
    parser.add_argument('--focuser-interval', dest = 'focuserInterval', action = 'store', default = list(), type = int, nargs = '+', help = ': %(default)s, focuser position interval, positions out side this interval will be ignored')
    parser.add_argument('--display-failures', dest = 'display_failures', action = 'store_true', default = False, help = ': %(default)s, display focus run where the fit failed')

    args = parser.parse_args()

    if args.debug:
        args.level =  'DEBUG'
        args.toconsole = True

    # logger
    logger =  Logger(debug = args.debug, args = args).logger # if you need to chage the log format do it here
    # hint to the user
    if defaultCfg in args.config:
        logger.info('rts2saf_focus: using default configuration file: {0}'.format(args.config))

    # config
    rtc = Configuration(logger = logger)
    if not rtc.readConfiguration(fileName=args.config):
        logger.error('rts2saf_focus: exiting, wrong syntax, check the configuration file: {0}'.format(args.config))
        sys.exit(1)

    # overwrite config defaults
    rtc.cfg['ANALYZE_FLUX'] = args.flux  
    rtc.cfg['ANALYZE_ASSOC'] = args.associate
    rtc.cfg['ANALYZE_ASSOC_FRACTION'] = args.fractObjs
    rtc.cfg['FOCUSER_INTERVAL'] = args.focuserInterval

    if args.emptySlots is not None:
        rtc.cfg['EMPTY_SLOT_NAMES'] = [ x.strip() for x in  args.emptySlots ]

    # ToDo ugly
    if args.filterNames is not None:
        fts = [ x.strip() for x in  args.filterNames ]
        args.filterNames = fts

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


    aRs = AnalyzeRuns(debug = args.debug, basePath = args.basePath, args = args, rt = rtc, ev = ev, logger = logger, xdisplay = XDISPLAY)
    aRs.aggregateRuns()
    if len(aRs.fS) == 0:
        logger.warn('rts2saf_analyze: exiting, no files found in basepath: {}'.format(args.basePath))
        sys.exit(1)

    rFf = aRs.analyzeRuns()
    
    if len(rFf)==0:
        logger.error('rts2saf_analyze: no results, exiting')
        sys.exit(1)

    if args.model:            
        if rFf[0].ambientTemp in 'NoTemp':
            logger.warn('rts2saf_analyze: no ambient temperature available in FITS files, no model fitted')
        else:
            # temperature model
            PLOTFN = ev.expandToPlotFileName( plotFn = '{}/temp-model.png'.format(args.basePath))
            dom = TemperatureFocPosModel(showPlot = True, date = ev.startTime[0:19],  comment = 'test run', plotFn = PLOTFN, resultFitFwhm = rFf, logger = logger)
            dom.fitData()
            dom.plotData()
            logger.info('rts2saf_analyze: storing plot at: {}'.format(PLOTFN))
        

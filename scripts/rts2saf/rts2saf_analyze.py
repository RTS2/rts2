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






if __name__ == '__main__':

    import argparse
    import sys
    import logging
    import os
    import glob

    try:
        import lib.config as cfgd
    except:
        import config as cfgd
    try:
        import lib.sextract as sx
    except:
        import sextract as sx

    try:
        import lib.environ as env
    except:
        import environ as env

    try:
        import lib.analyze as anr
    except:
        import analyze as anr

    parser= argparse.ArgumentParser(prog=sys.argv[0], description='rts2asaf analysis')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--debugSex', dest='debugSex', action='store_true', default=False, help=': %(default)s,add more output on SExtract')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--logfile',dest='logfile', default='/tmp/{0}.log'.format(sys.argv[0]), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--config', dest='config', action='store', default='/etc/rts2/rts2saf/rts2saf.cfg', help=': %(default)s, configuration file path')
    parser.add_argument('--dryfitsfiles', dest='dryfitsfiles', action='store', default='./samples', help=': %(default)s, directory where a FITS files are stored from a earlier focus run')
#ToDo    parser.add_argument('--ds9region', dest='ds9region', action='store_true', default=False, help=': %(default)s, create ds9 region files')
    parser.add_argument('--displayds9', dest='displayDs9', action='store_true', default=False, help=': %(default)s, display fits images and region files')
    parser.add_argument('--displayfit', dest='displayFit', action='store_true', default=False, help=': %(default)s, display fit')

    args=parser.parse_args()

    logformat= '%(asctime)s:%(name)s:%(levelname)s:%(message)s'
    logging.basicConfig(filename=args.logfile, level=args.level.upper(), format= logformat)
    logger = logging.getLogger()

    if args.level in 'DEBUG' or args.level in 'INFO':
        toconsole=True
    else:
        toconsole=args.toconsole

    if toconsole:
    #http://www.mglerner.com/blog/?p=8
        soh = logging.StreamHandler(sys.stdout)
        soh.setLevel(args.level)
        logger.addHandler(soh)

    rt=cfgd.Configuration(logger=logger)
    rt.readConfiguration(fileName=args.config)

    # get the environment
    ev=env.Environment(debug=args.debug, rt=rt,logger=logger)
    ev.createAcquisitionBasePath(ftwName=None, ftName=None)
    if args.dryfitsfiles:
        dryFitsFiles=glob.glob('{0}/{1}'.format(args.dryfitsfiles, rt.cfg['FILE_GLOB']))

        if len(dryFitsFiles)==0:
            logger.error('analyze: no FITS files found in:{}'.format(args.dryfitsfiles))
            logger.info('analyze: set --dryfitsfiles or'.format(args.dryfitsfiles))
            logger.info('analyze: download a sample from wget http://azug.minpet.unibas.ch/~wildi/rts2saf-test-focus-2013-09-14.tgz')
            logger.info('analyze: and store it in directory: {0}'.format(args.dryfitsfiles))
            sys.exit(1)
    else:
        fitsFiles= args.fitsFiles.split()

    cnt=0
    dataSex=dict()
    for fitsFn in dryFitsFiles:
        
        logger.info('analyze: processing fits file: {0}'.format(fitsFn))
        rsx= sx.Sextract(debug=args.debugSex, rt=rt, logger=logger)
        dataSex[cnt]=rsx.sextract(fitsFn=fitsFn) 
        cnt +=1

    an=anr.Analyze(debug=args.debug, dataSex=dataSex, displayDs9=args.displayDs9, displayFit=args.displayFit, ev=ev, logger=logger)
    weightedMeanObjects, weightedMeanFwhm, minFwhmPos, fwhm= an.analyze()

    logger.info('analyze: result: {0}, {1}, {2}, {3}'.format(weightedMeanObjects, weightedMeanFwhm, minFwhmPos, fwhm))

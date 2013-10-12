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


import argparse
import sys
import re
import rts2.queue
from rts2.json import JSONProxy

import rts2saf.log as lg
import rts2saf.config as cfgd
import rts2saf.sextract as sx

if __name__ == '__main__':

    prg= re.split('/', sys.argv[0])[-1]
    parser= argparse.ArgumentParser(prog=prg, description='rts2asaf fwhm')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--topath', dest='toPath', metavar='PATH', action='store', default='/tmp', help=': %(default)s, write log file to path') # needs always to write somwhere
    parser.add_argument('--logfile',dest='logfile', default='{0}.log'.format(prg), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--config', dest='config', action='store', default='/etc/rts2/rts2saf/rts2saf.cfg', help=': %(default)s, configuration file path')
    parser.add_argument('--queue', dest='queue', action='store', default='focusing', help=': %(default)s, queue where to set a focus run')
    parser.add_argument('--tarid', dest='tarId', action='store', default=5, help=': %(default)s, target id of OnTargetFocus')
    parser.add_argument('--fwhmthreshold', dest='fwhmThreshold', action='store', type=float, default=None, help=': %(default)s, threshold to trigger a focus run')
    parser.add_argument('--fitsFn', dest='fitsFn', action='store', default=None, help=': %(default)s, fits file to process')
    args=parser.parse_args()
    # logger
    if args.toconsole or args.debug:
        lgd= lg.Logger(debug=args.debug, args=args) # if you need to chage the log format do it here
        logger= lgd.logger 
    else: 
        # started by IMGP
        import logging
        logging.basicConfig(filename='/var/log/rts2-debug', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
        logger = logging.getLogger()
    # read the run time configuration
    rt=cfgd.Configuration(logger=logger)
    rt.readConfiguration(fileName=args.config)
    if not rt.checkConfiguration():
        logger.error('rts2saf_focus: exiting, check the configuration file: {0}'.format(args.config))
        sys.exit(1)

    sex= sx.Sextract(debug=args.debug, rt=rt, logger=logger)
    try:
        dataSex=sex.sextract(fitsFn=args.fitsFn) 
    except Exception, e:
        logger.info('rts2af_fwhm: sextractor failed on file: {0}\nerror: {1}\nexiting'.format(args.fitsFn, e))
        sys.exit(1)

    fwhmTreshold=rt.cfg['FWHM_LOWER_THRESH']

    if args.fwhmThreshold:
        fwhmTreshold=args.fwhmThreshold

    if( dataSex.fwhm > fwhmTreshold):
        proxy= JSONProxy(url=rt.cfg['URL'],username=rt.cfg['USERNAME'],password=rt.cfg['PASSWORD'])
        try:
            q=rts2.queue.Queue(jsonProxy=proxy,name=args.queue)
        except Exception, e:
            logger.info('rts2af_fwhm: no queue named: {0}, exiting'.format(args.queue))
            sys.exit(1)

        q.load()
        for x in q.entries:
            if 'OnTargetFocus' in x.get_target().name:
                logger.info('rts2af_fwhm: focus run already queued')
                break
        else:
            q.add_target(str(args.tarId))
            logger.info('rts2af_fwhm: focus run  queued, fwhm: {0:5.2f} > {1:5.2f} (thershold)'.format(dataSex.fwhm, fwhmTreshold))
    else:
        logger.info('rts2af_fwhm: no focus run  queued, fwhm: {0:5.2f} < {1:5.2f} (thershold)'.format(dataSex.fwhm, fwhmTreshold))

    logger.info('rts2af_fwhm: DONE')

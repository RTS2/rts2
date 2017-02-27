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

"""rts2saf_fwhm.py performs the FWHM analysis  and queues a focus 
run if FWHM is above threshold (see configuration). 

"""

__author__ = 'markus.wildi@bluewin.ch'


import argparse
import sys
import re
import os
import rts2
from rts2.json import JSONProxy

from rts2saf.sextract import Sextract
from rts2saf.config import Configuration
from rts2saf.log import Logger

if __name__ == '__main__':

    prg= re.split('/', sys.argv[0])[-1]
    parser= argparse.ArgumentParser(prog=prg, description='rts2saf calculate fwhm')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--sxdebug', dest = 'sxDebug', action = 'store_true', default = False, help = ': %(default)s,add more output on SExtract')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--topath', dest='toPath', metavar='PATH', action='store', default='/var/log/', help=': %(default)s, write log file to path') # needs always to write somwhere
    parser.add_argument('--logfile',dest='logfile', default='rts2-debug'.format(prg), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--config', dest='config', action='store', default='/usr/local/etc/rts2/rts2saf/rts2saf.cfg', help=': %(default)s, configuration file path')
    parser.add_argument('--queue', dest='queue', action='store', default='focusing', help=': %(default)s, queue where to set a focus run')
    parser.add_argument('--tarid', dest='tarId', action='store', default=5, help=': %(default)s, target id of OnTargetFocus')
    parser.add_argument('--fwhmthreshold', dest='fwhmThreshold', action='store', type=float, default=None, help=': %(default)s, threshold to trigger a focus run')
    parser.add_argument('--fitsFn', dest='fitsFn', action='store', default=None, help=': %(default)s, fits file to process')
    parser.add_argument('--ds9display', dest='Ds9Display', action='store_true', default=False, help=': %(default)s, display fits images and region files')
    args=parser.parse_args()
    # logger
    logger= Logger(debug=args.debug, args=args).logger # if you need to chage the log format do it here

    # read the run time configuration
    rt=Configuration(logger=logger)
    if not rt.readConfiguration(fileName=args.config):
        logger.error('rts2saf_focus: exiting, wrong syntax, check the configuration file: {0}'.format(args.config))
        sys.exit(1)

    if not rt.checkConfiguration(args=args):
        logger.error('rts2saf_focus: exiting, check the configuration file: {0}'.format(args.config))
        sys.exit(1)

    sex= Sextract(debug=args.debug, rt=rt, logger=logger)
    if args.fitsFn==None:
        logger.error('rts2af_fwhm: no --fitsFn specified, exiting'.format(args.fitsFn))
        sys.exit(1)

    elif not os.path.exists(args.fitsFn):
        logger.error('rts2af_fwhm: file: {0}, does not exist, exiting'.format(args.fitsFn))
        sys.exit(1)

    try:
        dataSxtr=sex.sextract(fitsFn=args.fitsFn) 
    except Exception, e:
        logger.error('rts2af_fwhm: sextractor failed on file: {0}\nerror from sex.sextract: {1}\nexiting'.format(args.fitsFn, e))
        sys.exit(1)

    fwhmTreshold=rt.cfg['FWHM_LOWER_THRESH'] 

    if args.fwhmThreshold:
        fwhmTreshold=args.fwhmThreshold

    if dataSxtr is None:
        logger.error('rts2af_fwhm: failed on file: {0}, exiting'.format(args.fitsFn))
        sys.exit(1)

    if( dataSxtr.fwhm <= fwhmTreshold):
        try:
            logger.info('rts2af_fwhm: no focus run  queued, fwhm: {0:5.2f} < {1:5.2f} (thershold)'.format(float(dataSxtr.fwhm), float(fwhmTreshold)))
        except Exception, e:
            logger.info('rts2af_fwhm: no focus run  queued, no FWHM calculated, error: {}'.format(e))

        q = None
        rts2.createProxy(url=rt.cfg['URL'], username=rt.cfg['RTS2_HTTPD_USERNAME'], password=rt.cfg['PASSWORD'])
        try:
            q = rts2.Queue(rts2.json.getProxy(), args.queue)
        except Exception, e:
            logger.error('rts2af_fwhm: no queue named: {0}, doing nothing'.format(args.queue))

        if q is not None:
            q.load()
            for x in q.entries:
                if x.get_target().name and 'OnTargetFocus' in x.get_target().name:
                    q.clear()
                    logger.info('rts2af_fwhm: cleared queue: {}'.format(args.queue))
                    break
    else:
        proxy=JSONProxy(url=rt.cfg['URL'],username=rt.cfg['RTS2_HTTPD_USERNAME'],password=rt.cfg['PASSWORD'])
        try:
            proxy.refresh()
        except Exception, e:
            logger.warn('rts2af_fwhm: JSON proxy connection failed: {}, exiting'.format(e))
            sys.exit(1)

        tarType = proxy.getSingleValue('EXEC','current_type')
        tarName = proxy.getSingleValue('EXEC','current_name')
        if 'GLORIA teleoperation' in tarName: # reserved observing time
            logger.info('rts2af_fwhm: there is a ongoing GLORIA teleoperation, no focus run queued')
        elif 'G' in tarType: # it is a GRB
            logger.info('rts2af_fwhm: there is now a GRB target selected, no focus run queued')

        else:
            q = None
            rts2.createProxy(url=rt.cfg['URL'], username=rt.cfg['RTS2_HTTPD_USERNAME'], password=rt.cfg['PASSWORD'])
            try:
                q = rts2.Queue(rts2.json.getProxy(), args.queue)
            except Exception, e:
                logger.error('rts2af_fwhm: no queue named: {0}, doing nothing'.format(args.queue))

            if q is not None:
                q.load()
                for x in q.entries:
                    if x.get_target().name and 'OnTargetFocus' in x.get_target().name:
                        logger.info('rts2af_fwhm: focus run already queued')
                        break
                else:
                    try:
                        q.add_target(str(args.tarId))
                        logger.info('rts2af_fwhm: fwhm: {0:5.2f} > {1:5.2f} (thershold), focus run queued'.format(dataSxtr.fwhm, fwhmTreshold))
                    except Exception, e:
                        logger.error('rts2af_fwhm: queing failed: {0}'.format(e))

            
    # display fits and regions if necessary
    if args.Ds9Display:
        from rts2saf.ds9region import Ds9DisplayThread
        from subprocess import Popen, PIPE
        p = Popen(["xset", "-q"], stdout=PIPE, stderr=PIPE)
        p.communicate()
        if p.returncode == 0:
            XDISPLAY=True
        else:
            XDISPLAY=False

        # start thread 
        ds9DisplayThread = Ds9DisplayThread(debug=args.debug, dataSxtr=[dataSxtr], logger= logger)
        ds9DisplayThread.start()
        var = raw_input("enter any key to stop")
        ds9DisplayThread.join(timeout=1.)

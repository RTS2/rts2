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

import sys
import argparse

import lib.config as cfgd
import lib.log as  lg
import lib.environ as env
import lib.focus as fc
import lib.devices as dev


if __name__ == '__main__':
    # since rts2 can not pass options, any option needs a decent default value
    parser= argparse.ArgumentParser(prog=sys.argv[0], description='rts2asaf analysis')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--logfile',dest='logfile', default='/tmp/{0}.log'.format(sys.argv[0]), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--dryfitsfiles', metavar='DIRECTORY', dest='dryFitsFiles', action='store', default=None, help=': %(default)s, directory where a set of FITS files are stored from a previous focus run')
    parser.add_argument('--config', dest='config', action='store', default='/etc/rts2/rts2saf/rts2saf.cfg', help=': %(default)s, configuration file path')
    parser.add_argument('--verbose', dest='verbose', action='store_true', default=False, help=': %(default)s, print device properties and add more messages')
    parser.add_argument('--exposure', dest='exposure', action='store', default=None, type=float, help=': %(default)s, exposure time for CCD')
    parser.add_argument('--focStep', dest='focStep', action='store', default=None, type=int, help=': %(default)s, focuser step size during blind run, see --blind')
    parser.add_argument('--blind', dest='blind', action='store_true', default=False, help=': %(default)s, focus run within range(RTS2::foc_min,RTS2::foc_max, RTS2::foc_step), if --focStep is defined it is used to set the range')
    parser.add_argument('--checkconfig', dest='checkConfig', action='store_true', default=False, help=': %(default)s, check connection to RTS2, the devices and their configuration, then EXIT')

    args=parser.parse_args()
    if args.verbose:
        args.level='DEBUG'
        args.toconsole=True
    #
    if args.checkConfig:
        args.debug=True
    # logger
    lgd= lg.Logger(debug=args.debug, args=args) # if you need to chage the log format do it here
    logger= lgd.logger 
    # read the run time configuration
    rt=cfgd.Configuration(logger=logger)
    rt.readConfiguration(fileName=args.config)
    if not rt.checkConfiguration():
        logger.error('rts2saf_focus: exiting, check the configuration file: {0}'.format(args.config))
        sys.exit(1)

    # get the environment
    ev=env.Environment(debug=args.debug, rt=rt,logger=logger)

    # check the presence of the devices and if there is an empty slot on each wheel
    cdv= dev.CheckDevices(debug=args.debug, args=args, rt=rt, logger=logger)
    if not cdv.statusDevices():
        logger.error('rts2saf_focus: exiting, check the configuration file: {0}'.format(args.config))
        sys.exit(1)
    #
    if not cdv.deviceWriteAccess():
            logger.error('rts2saf_focus:  exiting')
            sys.exit(1)

    # these files are injected in case no actual night sky images are available
    # neverthless ccd is exposing, filter wheels and focuser are moving
    dryFitsFiles=None
    if args.dryFitsFiles:
        dryFitsFiles=glob.glob('{0}/{1}'.format(args.dryFitsFiles, rt.cfg['FILE_GLOB']))
        if len(dryFitsFiles)==0:
            logger.error('rts2saf_focus: no FITS files found in:{}'.format(args.dryFitsFiles))
            logger.info('rts2saf_focus: download a sample from wget http://azug.minpet.unibas.ch/~wildi/rts2saf-test-focus-2013-09-14.tgz')
            sys.exit(1)

    if args.checkConfig:
        logger.info('rts2saf_focus: configuration check done for config file:{0}, exiting'.format(args.config))
        sys.exit(1)

    # start acquistion and analysis
    logger.info('rts2saf_focus: starting scan at: {0}'.format(ev.startTime))
    #
    if len(rt.filterWheelsInUse):
        fftw=fc.FocusFilterWheels(debug=args.debug, args=args, dryFitsFiles=dryFitsFiles, rt=rt, ev=ev, logger=logger)
        fftw.run()
    else:
        # without filter wheel
        f=gtf.Focus(debug=args.debug, args=args, dryFitsFiles=dryFitsFiles, rt=rt,ev=ev,logger=logger)
        f.run()

    logger.info('rts2saf_focus: end scan at: {0}'.format(ev.startTime))


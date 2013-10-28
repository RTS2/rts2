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
import re
import glob

from rts2.json import JSONProxy

from rts2saf.config import Configuration
from rts2saf.environ import Environment
from rts2saf.log import Logger
from rts2saf.focus import Focus
from rts2saf.checkdevices import CheckDevices
from rts2saf.createdevices import CreateCCD,CreateFocuser,CreateFilters,CreateFilterWheels
from rts2saf.devices import CCD,Focuser,Filter,FilterWheel


if __name__ == '__main__':
    # since rts2 can not pass options, any option needs a decent default value
    prg= re.split('/', sys.argv[0])[-1]
    parser= argparse.ArgumentParser(prog=prg, description='rts2asaf analysis')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--topath', dest='toPath', metavar='PATH', action='store', default='.', help=': %(default)s, write log file to path') # needs a path where it can always write
    parser.add_argument('--logfile',dest='logfile', default='{0}.log'.format(prg), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--dryfitsfiles', metavar='DIRECTORY', dest='dryFitsFiles', action='store', default=None, help=': %(default)s, directory where a set of FITS files are stored from a previous focus run')
    parser.add_argument('--config', dest='config', action='store', default='/usr/local/etc/rts2/rts2saf/rts2saf.cfg', help=': %(default)s, configuration file path')
    parser.add_argument('--verbose', dest='verbose', action='store_true', default=False, help=': %(default)s, print device properties and add more messages')
    parser.add_argument('--checkconfig', dest='checkConfig', action='store_true', default=False, help=': %(default)s, check connection to RTS2, the devices and their configuration, then EXIT')
    parser.add_argument('--focrange', dest='focRange', action='store', default=None,type=int, nargs='+', help=': %(default)s, focuser range given as "ll ul st" used only during blind run')
    parser.add_argument('--exposure', dest='exposure', action='store', default=None, type=float, help=': %(default)s, exposure time for CCD')
    parser.add_argument('--focdef', dest='focDef', action='store', default=None, type=float, help=': %(default)s, set FOC_DEF to value')
    parser.add_argument('--blind', dest='blind', action='store_true', default=False, help=': %(default)s, focus range and step size are defined in configuration, if --focrange is defined it is used to set the range')
    parser.add_argument('--ds9display', dest='Ds9Display', action='store_true', default=False, help=': %(default)s, display fits images and region files')
    parser.add_argument('--fitdisplay', dest='FitDisplay', action='store_true', default=False, help=': %(default)s, display fit')

    args=parser.parse_args()
    if args.verbose:
        args.debug=True
        args.level='DEBUG'
        args.toconsole=True
        
    if args.checkConfig:
        args.toconsole=True

    # logger
    logger= Logger(debug=args.debug, args=args).logger # if you need to chage the log format do it here
    # read the run time configuration
    rt=Configuration(logger=logger)
    rt.readConfiguration(fileName=args.config)
    if not rt.checkConfiguration():
        logger.error('rts2saf_focus: exiting, check the configuration file: {0}'.format(args.config))
        sys.exit(1)

    # get the environment
    ev=Environment(debug=args.debug, rt=rt,logger=logger)

    if not args.blind and args.focRange:
        logger.error('rts2saf_focus: --focrange has no effect without --blind'.format(args.focRange))
        sys.exit(1)

    if args.focRange:
        if (args.focRange[0] >= args.focRange[1]) or args.focRange[2] <= 0: 
            logger.error('rts2saf_focus: bad range values: {}, exiting'.format(args.focRange))
            sys.exit(1)

    # establish a connection
    proxy=JSONProxy(url=rt.cfg['URL'],username=rt.cfg['USERNAME'],password=rt.cfg['PASSWORD'])
    try:
        proxy.refresh()
    except Exception, e:
        logger.error('rts2saf_focus: no JSON connection for: {0}, {1}, {2}'.format(rt.cfg['URL'],rt.cfg['USERNAME'],rt.cfg['PASSWORD']))
        sys.exit(1)
    # create all devices
    # attention: .create() at the end
    # filters are not yet devices
    # check always True, we need to set limits either from device or from configuration
    foc= CreateFocuser(debug=args.debug, proxy=proxy, check=True, rangeFocToff=args.focRange, blind=args.blind, verbose=args.verbose, rt=rt, logger=logger).create()
    if foc==None or not isinstance(foc, Focuser):
        logger.error('rts2saf_focus: could not create object for focuser: {}, exiting'.format(rt.cfg['FOCUSER_NAME']))
        sys.exit(1)

    if args.focDef:
        foc.writeFocDef(proxy=proxy, focDef=args.focDef)
        logger.info('rts2saf_focus: {0} FOC_DEF: {1} set'.format(foc.name,args.focDef))

    filters= CreateFilters(debug=args.debug, proxy=proxy, check=args.checkConfig, blind=args.blind, verbose=args.verbose, rt=rt, logger=logger).create()
    ftws   = CreateFilterWheels(filters=filters, foc=foc, debug=args.debug, proxy=proxy, check=args.checkConfig, blind=args.blind, verbose=args.verbose, rt=rt, logger=logger).create()

    # at least one even if it is FAKE_FTW
    if ftws==None or not isinstance(ftws[0], FilterWheel):
        logger.error('rts2saf_focus: could not create object for filter wheel: {}, exiting'.format(rt.cfg['FILTER WHEELS INUSE']))
        sys.exit(1)

    ccd= CreateCCD(debug=args.debug, proxy=proxy, ftws=ftws, check=args.checkConfig, blind=args.blind, verbose=args.verbose, rt=rt, logger=logger).create()
    if ccd==None or not isinstance(ccd, CCD):
        logger.error('rts2saf_focus: could not create object for CCD: {}, exiting'.format(rt.cfg['CCD_NAME']))
        sys.exit(1)

    # while called from IMGP hopefully every device is there
    if args.checkConfig:
        # check the presence of the devices and if there is an empty slot on each wheel
        cdv= CheckDevices(debug=args.debug, proxy=proxy, blind=args.blind, verbose=args.verbose, ccd=ccd, ftws=ftws, foc=foc, logger=logger)
        cdv.summaryDevices()

        # are devices writable
        if not cdv.deviceWriteAccess():
            logger.error('rts2saf_focus: exiting')
            logger.info('rts2saf_focus: run {0} --verbose'.format(prg))
            sys.exit(1)

        logger.info('rts2saf_focus: configuration check done for config file:{0}, exiting'.format(args.config))
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


    # start acquistion and analysis
    logger.info('rts2saf_focus: starting scan at: {0}'.format(ev.startTime))
    fs=Focus(debug=args.debug, proxy=proxy, args=args, dryFitsFiles=dryFitsFiles, ccd=ccd, foc=foc, ftws=ftws, rt=rt, ev=ev, logger=logger)
    fs.run()
    logger.info('rts2saf_focus: end scan at: {0}'.format(ev.startTime))


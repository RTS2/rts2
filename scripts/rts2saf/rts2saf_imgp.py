#!/usr/bin/python
# (C) 2011-2013, Markus Wildi, markus.wildi@one-arcsec.org
#
#   rts2saf_imgp.py is the script called by IMGP after expusre has been
#   completed.
#   It serves as a wrapper for several tasks to be performed, currently
#   1) define the FWHM and write it to the log file (rts2-debug)
#   2) do astrometric calibration
#
#   planned:
#   3) dark frame subtraction and flat fielding
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
"""rts2saf_imgp.py is usually called by RTS2 IMGP to perform FWHM analysis 
and the astrometric calibration. 

"""

__author__ = 'markus.wildi@one-arcsec.org'

import sys
import subprocess
import re


class ImgpAnalysis():
    """Called by IMGP to do astrometric calibration and FWHM analysis"""

    def __init__(self, scriptName=None, fwhmCmd=None, astrometryCmd=None, fitsFileName=None, config=None, logger=None):
        # RTS2 has no possibilities to pass arguments to a command, defining the defaults
        self.scriptName= scriptName 
        self.fitsFileName= fitsFileName
        self.config=config
        self.logger=logger
        self.fwhmCmd= fwhmCmd
        # uses header created by standard RTS2
        self.astrometryCmd=astrometryCmd

    def spawnProcess( self, cmd=None, wait=None):
        """Spawn a process and wait or do not wait for completion"""
        subpr=None
        try:
            if(wait):
                subpr  = subprocess.Popen( cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            else:
                subpr  = subprocess.Popen( cmd)

        except OSError as (errno, strerror):
            self.logger.error('{0}: returning due to I/O error({1}): {2}\ncommand:{3}'.format(self.scriptName, errno, strerror, cmd))
            return None

        except:
            self.logger.error('{0}: returning due to: {1} died'.format(self.scriptName, repr(cmd)))
            return None

        return subpr

    def run(self):
        """ Execute rts2saf_fwhm.py and what is defined as astrometric calibration script in configuration (see SCRIPT_ASTROMETRY). Child's stdout and stderr are copied to stdout and stderr which is read by RTS2 IMGP."""
        self.logger.info( '{0}: starting'.format(self.scriptName))

        cmd= [  self.fwhmCmd,
                '--config',
                self.config,
                '--fitsFn',
                self.fitsFileName,
                ]
        # no time to waste, the decision toqueue a focus run is done in rts2saf_fwhm.py
        try:
            fwhmLine= self.spawnProcess(cmd, False)
        except:
            self.logger.error( '{0}: starting suprocess: {1} failed, continuing with astrometrical calibration'.format(self.scriptName, cmd))

        cmd= [  self.astrometryCmd,
                self.fitsFileName,
                ]
        # this process is hopefully started in parallel on the second core if any.
        try:
            stdo,stde= self.spawnProcess(cmd=cmd, wait=True).communicate()
        except:
            self.logger.error( '{0}: ending, reading from {1} pipe failed'.format(self.scriptName, self.astrometryCmd))
            sys.exit(1)
        if stde:
            self.logger.error( '{0}: message from {1} on stderr:\n{2}'.format(self.scriptName, self.astrometryCmd, stde))
            sys.stderr.write(stde)

        lnstdo= stdo.split('\n')
        for ln in lnstdo:
            self.logger.info( '{0}, {1}: {2}'.format(self.scriptName, self.astrometryCmd, ln))
            if re.search('corrwerr', ln):
                print ln

        self.logger.info( '{0}: ending'.format(self.scriptName))
            

if __name__ == "__main__":

    import os
    import argparse
    from astropy.io.fits import getheader
    from rts2saf.config import Configuration
    from rts2saf.log import Logger

    script=os.path.basename(__file__)
    parser= argparse.ArgumentParser(prog=script, description='rts2asaf online image processing')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--topath', dest='toPath', metavar='PATH', action='store', default='.', help=': %(default)s, write log file to path') # needs a path where it can always write
    parser.add_argument('--logfile',dest='logfile', default='{0}.log'.format(script), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--config', dest='config', action='store', default='/usr/local/etc/rts2/rts2saf/rts2saf.cfg', help=': %(default)s, configuration file path')
    parser.add_argument( metavar='FITS FN', dest='fitsFn', nargs=1, default=None, help=': %(default)s, FITS file name')

    args=parser.parse_args()

    if args.toconsole or args.debug:
        logger= Logger(debug=args.debug, args=args).logger # if you need to chage the log format do it here
    else: 
        # started by IMGP
        import logging
        logging.basicConfig(filename='/var/log/rts2-debug', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
        logger = logging.getLogger()

    # read the run time configuration
    rt=Configuration(logger=logger)
    rt.readConfiguration(fileName=args.config)
    if not rt.checkConfiguration(args=args):
        logger.error('[0]: exiting, check the configuration file: {1}'.format(script, args.config))
        sys.exit(1)

    if not args.fitsFn:
        parser.print_help()
        logger.warn('{0}: no FITS file specified'.format(script))
        sys.exit(1)

    fitsFn=args.fitsFn[0]
    if not os.path.exists(fitsFn):
        logger.error('{0}: file: {1}, does not exists, exiting'.format(script, fitsFn))
        sys.exit(1)

    # check if there is a header element on the list
    # which excludes fitsFn from being processed, e.g. hartmann
    imgp_analysis= ImgpAnalysis(
        scriptName=script, 
        fwhmCmd=rt.cfg['SCRIPT_FWHM'], 
        astrometryCmd=rt.cfg['SCRIPT_ASTROMETRY'], 
        fitsFileName=fitsFn, 
        config=args.config,
        logger=logger)
    # if in config file is nothing specified it is a str
    if isinstance(rt.cfg['FILTERS_TO_EXCLUDE'], dict):
        ftNameS=rt.cfg['FILTERS_TO_EXCLUDE'].keys()

        # fitsHeaderName are in reality names of filter wheels
        # in the notation of CCD driver (FILTA, FILTB, ...)
        hdr = getheader(fitsFn, 0)
        for fitsHeaderName in set(rt.cfg['FILTERS_TO_EXCLUDE'].values()):
            try:
                hdrv = hdr[fitsHeaderName]
            except Exception, e:
                continue

            if hdrv in ftNameS:
                logger.info('{0}: skipping FITS file: {1} due to filter wheel:{2}, contains filter:{3} (see config file)'.format(script, fitsFn, fitsHeaderName, hdrv))
                sys.exit(1)

    imgp_analysis.run()

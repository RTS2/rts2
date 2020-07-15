#!/usr/bin/env python
# (C) 2011-2013, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage: imgp_analysis.py /absolute/path/to/fits 
#   
#   imgp_analysis.py is the script called by IMGP after expusre has been
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

__author__ = 'markus.wildi@one-arcsec.org'

import sys
import subprocess
import re
import logging

logging.basicConfig(filename='/var/log/rts2-debug', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')

class ImgpAnalysis():
    """called by IMGP to to astrometric calibration, and fwhm determination"""

    def __init__(self, scriptName=None, fitsFileName=None):
        # RTS2 has no possibilities to pass arguments to a command, defining the defaults
        self.scriptName= scriptName 
        self.fitsFileName= fitsFileName
        self.astrometryCmd= 'rts2-astrometry.net'
        self.fwhmCmd= 'rts2saf_fwhm.py'
        # this is the default config.
        # specify a different here if necessary
        self.fwhmConfigFile= '/usr/local/etc/rts2/rts2saf/rts2saf.cfg'
        self.fwhmLogFile= '/var/log/rts2-debug'

    def spawnProcess( self, cmd=None, wait=None):
        """Spawn a process and Wait or do not wait for completion"""
        subpr=None
        try:
            if(wait):
                subpr  = subprocess.Popen( cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            else:
                subpr  = subprocess.Popen( cmd)

        except OSError as (errno, strerror):
            logging.error('{0}: returning due to I/O error({1}): {2}\ncommand:{3}'.format(self.scriptName, errno, strerror, cmd))
            return None

        except:
            logging.error('{0}: returning due to: {1} died'.format(self.scriptName, repr(cmd)))
            return None

        return subpr

    def run(self):
        logging.info( '{0}: starting'.format(self.scriptName))

        cmd= [  self.fwhmCmd,
                '--config',
                self.fwhmConfigFile,
                '--logfile',
                self.fwhmLogFile,
                '--fitsFn',
                self.fitsFileName,
                ]
# no time to waste, the decision if a focus run is triggered is done elsewhere
        try:
            fwhmLine= self.spawnProcess(cmd, False)
        except:
            logging.error( '{0}: starting suprocess: {1} failed, continuing with astrometrical calibration'.format(self.scriptName, cmd))

        cmd= [  self.astrometryCmd,
                self.fitsFileName,
                ]
# this process is hopefully started in parallel on the second core if any.
        try:
            stdo,stde= self.spawnProcess(cmd=cmd, wait=True).communicate()
        except:
            logging.error( '{0}: ending, reading from astrometry.net pipe failed'.format(self.scriptName))
            sys.exit(1)
        if stde:
            logging.error( '{0}: message on stderr:\n{1}\nexiting'.format(self.astrometryCmd, stde))
            sys.exit(1)
            
        # ToDo
        lnstdo= stdo.split('\n')
        for ln in lnstdo:
            logging.info( 'rts2-astrometry.net: {0}'.format(ln))
            if re.search('corrwerr', ln):
                print ln

        logging.info( 'imgp_analysis.py: ending')
            

if __name__ == "__main__":

    import os
    head, script=os.path.split(sys.argv[0])
    if( len(sys.argv)== 2):
        imgp_analysis= ImgpAnalysis(scriptName=script, fitsFileName=sys.argv[1])
    else:
        print '{0}: exiting, no fits file name given'.format(script)
        sys.exit(1)

    imgp_analysis.run()

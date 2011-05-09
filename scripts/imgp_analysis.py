#!/usr/bin/python
# (C) 2011, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage: imgp_analysis.py /absolute/path/to/fits 
#   
#   imgp_analysis.py is the script called by IMGP after expusre has been
#   completed.
#   It serves as a wrapper for several tasks to be performed, currently
#   1) do astrometric calibration
#   2) define the FWHM and write it to the log file (rts2-debug)
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
        self.astrometryCmd= '/etc/rts2/bin/img_astrometry.py'
        self.fwhmCmd= 'rts2af_fwhm.py'
        self.fwhmConfigFile= '/etc/rts2/rts2af/rts2af-fwhm.cfg'
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
            logging.error( 'imgp_analysis.py: I/O error({0}): {1}'.format(errno, strerror))
            sys.exit(1)

        except:
            logging.error('imgp_analysis.py: '+ repr(cmd) + ' died')
            sys.exit(1)

        return subpr

    def run(self):
        logging.info( 'imgp_analysis.py: starting')

        cmd= [  self.fwhmCmd,
                '--config',
                self.fwhmConfigFile,
                '--logto',
                self.fwhmLogFile,
                '--reference',
                self.fitsFileName,
                ]
# no time to waste, the decision if a focus run is triggered is done elsewhere
        fwhmLine= self.spawnProcess(cmd, False)
        cmd= [  self.astrometryCmd,
                self.fitsFileName,
                ]
# this process is hopefully started in parallel on the second core if any.
        astrometryLine= self.spawnProcess(cmd, True).communicate()
        astrometryLine= astrometryLine[0].split('\n')
# tell the result IMGP
        print '{0}'.format(astrometryLine[0])
        logging.info( 'imgp_analysis.py: ending')

if __name__ == "__main__":
    if( len(sys.argv)== 2):
        imgp_analysis= ImgpAnalysis(scriptName=sys.argv[0], fitsFileName=sys.argv[1])
    else:
        print 'imgp_analysis.py: exiting, no fits file name given'
        sys.exit(1)

    imgp_analysis.run()

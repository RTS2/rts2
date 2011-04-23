#!/usr/bin/python
# (C) 2011, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rts2af_feed_acquire.py --help
#   
#   not yet see man 1 rts2af_feed_acquire
#
#   Basic usage: rts2af_feed_acquire.py
# 
#   This script feeds rts2af_acquire.py with offline data to test the
#   whole analysis/fitting chain.
#   Attention: this script does nwver end, kill it with CRTL-C.
#
#   Configuration is done in def __init__ below.
#
#   In self.storePath must reside the fits files from a previous focus run.
#   Define in self.referenceFile the fits file which is near the focus.
#
#   Adjust the pathes according to svn checkout path.
#
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


__author__ = 'markus.wildi@one-arcsec.org'

import sys
import subprocess
import glob
import time
import re


class main():
    """Script to  feed rts2af_acquire.py with previously taken fits files."""
    def __init__(self, scriptName='main'):
        self.scriptName= scriptName
        # 2011-04-15-T23:58:06
        self.storePath=[]
        self.storePath.append('/scratch/focus/2011-04-15-T23:58:06/X') # X
        self.storePath.append('/scratch/focus/2011-04-23T00:33:19.510523/H') # H
        self.referenceFile=[]
        self.referenceFile.append('20110415235822-154-RA.fits') # X
        self.referenceFile.append('20110422224736-422-RA.fits') # H
        self.cmd= '/usr/local/src/rts-2-head/scripts/rts2af_acquire.py'
        self.focuser = 'FOC_DMY'  
        #
        self.pexposure= re.compile( r'exposure')

    def ignoreOutput(self, acq):
        while(True):
            time.sleep(.1)
            output= acq.stdout.readline()
            if( self.pexposure.match(output)):
                print 'rts2af_feed_acquire, breaking on exposure output >>{0}<<'.format(output)
                break
            else:
                log_match= re.search( r'log', output)
                if(log_match):
                    print 'rts2af_feed_acquire ignore (only to the first <CR>): >>{0}<<>>'.format(output)

    def main(self):

        fitsFiles=[]

        acquire_cmd= [ self.cmd, 'test']
        acquire= subprocess.Popen( acquire_cmd, stdin=subprocess.PIPE,stdout=subprocess.PIPE)

        # focuser dialog
        output= acquire.stdout.readline()
        print 'rts2af_feed_acquire, output: >>{0}<<, focuser'.format(output, self.focuser)
        acquire.stdin.write('{0}\n'.format(self.focuser))

        for storePath in  self.storePath:
            
            fitsFiles= glob.glob( storePath + '/' + '*fits')
 
            # first file is the reference catalogue
            self.ignoreOutput(acquire)
            time.sleep(3)
            acquire.stdin.write('{0}\n'.format('exposure_end'))
            popoff= self.referenceFile.pop(0)
            print 'rts2af_feed_acquire: poping reference file {0}'.format(popoff)

            acquire.stdin.write('img {0} \n'.format( storePath + '/' + popoff))
            acquire.stdin.flush()

            # acquire dialog
            for fitsFile in fitsFiles:
                #            print 'rts2af_feed_acquire, fits file     {0}'.format(fitsFile)
                # ignore lines until next exposure
                self.ignoreOutput(acquire)
                time.sleep(2)
                # exposure ends
                acquire.stdin.write('{0}\n'.format('exposure_end'))
                acquire.stdin.write('img {0} \n'.format(fitsFile))
                acquire.stdin.flush()
                print 'rts2af_feed_acquire, exposure ends {0}'.format(fitsFile)

            # send a Q
            self.ignoreOutput(acquire)
            time.sleep(3)
            # exposure ends
            acquire.stdin.write('{0}\n'.format('exposure_end'))
            acquire.stdin.write('img /abc/{0}\n'.format('Q'))
            acquire.stdin.flush()

            print 'rts2af_feed_acquire, sent a Q'


        print 'rts2af_feed_acquire ending, reading stdin for ever'
        while(True):
            try:
                print 'rts2af_feed_acquire.py: from rts2af_acquire.py: {0}'.format( acquire.stdout.readline())
            except:
                print 'rts2af_feed_acquire.py: cought exception, breaking'
                break
            time.sleep(1)

if __name__ == '__main__':
    main(sys.argv[0]).main()

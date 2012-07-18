#!/usr/bin/python
# (C) 2011, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rts2af_feed_acquire.py
#   
#   not yet see man 1 rts2af_feed_acquire
#
#   Basic usage: rts2af_feed_acquire.py
# 
#   This script feeds rts2af_acquire.py with offline data to test 
#   the wholet rts2af  analysis/fitting chain.
#   Attention: this script does never end, kill it with CRTL-C.
#
#
#   Configuration is done in def __init__ below.
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

import rts2af

class main():
    """Script to feed rts2af_acquire.py with previously taken fits files."""
    def __init__(self, scriptName='main'):
        self.scriptName= scriptName

        self.verbose= True
        #self.mode= 'feedback' # rts2af_analysis.py is replaced by rts2af_feedback_analysis.py 
        self.mode= 'feed' # rts2af_analysis.py is used and results are fitted (the whole chain)
        self.focuser = 'FOC_DMY'  
        # reference file 
        self.referenceFile='20120324001509-875-RA-reference.fits'
        self.cmd= 'rts2af_acquire.py'
        #
        self.pexposure= re.compile( r'exposure')
        self.filterInUse=None
        self.runTimeConfig= rts2af.runTimeConfig= rts2af.Configuration() # default config
        self.runTimeConfig.readConfiguration('./rts2af-acquire.cfg') # rts2 exe mechanism has no options

        self.fitsHDU= rts2af.FitsHDU( self.referenceFile)

        if(self.fitsHDU.fitsFileName==None):
            sys.exit(1)
            
        if( not self.fitsHDU.headerProperties()):
            sys.exit(1)

        self.runTimePath= rts2af.serviceFileOp.runTimePath

        if(self.fitsHDU.staticHeaderElements['FILTER']== 'UNK'):
            self.filterInUse='NOFILTER'
        else:
            self.filterInUse= self.fitsHDU.staticHeaderElements['FILTER']

    def ignoreOutput(self, acq):
        while(True):
            time.sleep(.1)
            output= acq.stdout.readline().strip()
            if( self.pexposure.match(output)):
                if( self.verbose):
                    print 'rts2af_feed_acquire, breaking on exposure output >>{0}<<'.format(output)
                break
            else:
                if( self.verbose):
                    log_match= re.search( r'log', output)
                    if(log_match):
                        print 'rts2af_feed_acquire ignore (only to the first <CR>): >>{0}<<'.format(output)

    def main(self):


        acquire_cmd= [ self.cmd, self.mode]
        acquire= subprocess.Popen( acquire_cmd, stdin=subprocess.PIPE,stdout=subprocess.PIPE)

        # focuser dialog
        output= acquire.stdout.readline().strip()
        if( self.verbose):
            print 'rts2af_feed_acquire, output: >>{0}<<, focuser'.format(output, self.focuser)
        acquire.stdin.write('{0}\n'.format(self.focuser))

        # filterInUse dialog
        output= acquire.stdout.readline().strip()
        acquire.stdin.write('{0} '.format(self.filterInUse))
        acquire.stdin.write('\n')

        fitsFiles=[]
        fitsFiles= glob.glob( self.runTimePath + '/' + '*fits')
        # first file is the reference catalogue
        self.ignoreOutput(acquire)
            #time.sleep(.1)
        acquire.stdin.write('{0}\n'.format('exposure_end'))
        if( self.verbose):
            print 'rts2af_feed_acquire: reference file {0}'.format(self.referenceFile)

        acquire.stdin.write('img {0} \n'.format( self.runTimePath + '/' + self.referenceFile))
        acquire.stdin.flush()
        
        for fitsFile in fitsFiles:
            if( self.verbose):
                print 'rts2af_feed_acquire, fits file     {0}'.format(fitsFile)

                # acquire dialog
                # ignore lines until next exposure
            self.ignoreOutput(acquire)
                #time.sleep(.1)
                # exposure ends
            acquire.stdin.write('{0}\n'.format('exposure_end'))
            acquire.stdin.write('img {0} \n'.format(fitsFile))
            acquire.stdin.flush()
            print 'rts2af_feed_acquire, exposure ends {0}'.format(fitsFile)

        # send a Q to signal rts2af_analysis.py to begin with the fitting
        self.ignoreOutput(acquire)
        #time.sleep(.1)
        # exposure ends
        acquire.stdin.write('{0}\n'.format('exposure_end'))
        acquire.stdin.write('img /abc/{0}\n'.format('Q'))
        acquire.stdin.flush()

        if( self.verbose):
            print 'rts2af_feed_acquire, sent a Q'

        print 'rts2af_feed_acquire ending, reading stdin for ever'
        while(True):
            try:
                print 'rts2af_feed_acquire.py: from rts2af_acquire.py: {0}'.format( acquire.stdout.readline().strip())
            except:
                print 'rts2af_feed_acquire.py: cought exception, breaking'
                break
            time.sleep(1)

if __name__ == '__main__':
    main(sys.argv[0]).main()

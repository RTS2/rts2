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
#   You must install CERN's root package, recompile and install 
#   RTS2 to get the executable rts2-fit-focus (see contrib within 
#   RTS2 source tree).
#
#   Configuration is done in def __init__ below.
#
#   self.storePath contains the path to the fits files from a previous 
#   focus run. These are retrieved by a glob function.
#   Define in self.referenceFile the fits file which is near the focus.
#
#   Adjust the pathes according to svn checkout path or install the
#   executables e.g. in /usr/local/bin (recommended).
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
        self.scriptPath= '/home/wildi/workspace/rts2-head/scripts/'

        self.storePath=[]
        self.storePath.append('/usr/local/src/rts2af-data/samples/03/') # NOFILTER (AOSTA)
#        self.storePath.append('/scratch/focus/2011-04-15-T23:58:06/X') # X
#        self.storePath.append('/scratch/focus/2011-04-23T00:33:19.510523/H') # H
        self.referenceFile=[]
        self.referenceFile.append('20071205025927-674-RA.fits') # NOFILTER (AOSTA)
#        self.referenceFile.append('20110416000447-370-RA.fits') # X
#        self.referenceFile.append('20110422224736-422-RA.fits') # H
#        self.cmd= self.scriptPath + 'rts2af_acquire.py'
        self.cmd= 'rts2af_acquire.py'
        self.focuser = 'FOC_DMY'  
        self.verbose= True
        #
        self.pexposure= re.compile( r'exposure')
        self.fitsHDUs=[]
        self.filtersInUse=[]
        self.runTimeConfig= rts2af.runTimeConfig= rts2af.Configuration() # default config
        self.runTimeConfig.readConfiguration('/etc/rts2/rts2af/rts2af-acquire.cfg') # rts2 exe mechanism has no options

        i= 0 # ugly
        for storePath in self.storePath:
            self.fitsHDUs.append( rts2af.FitsHDU( storePath + '/' + self.referenceFile[i]))
            self.fitsHDUs[i].headerProperties()
            if(self.fitsHDUs[i].staticHeaderElements['FILTER']== 'UNK'):
                self.filtersInUse.append('NOF')
            else:
                self.filtersInUse.append(self.fitsHDUs[i].staticHeaderElements['FILTER'])
                i += 1
                

    def ignoreOutput(self, acq):
        while(True):
            time.sleep(.1)
            output= acq.stdout.readline()
            if( self.pexposure.match(output)):
                if( self.verbose):
                    print 'rts2af_feed_acquire, breaking on exposure output >>{0}<<'.format(output)
                break
            else:
                if( self.verbose):
                    log_match= re.search( r'log', output)
                    if(log_match):
                        print 'rts2af_feed_acquire ignore (only to the first <CR>): >>{0}<<>>'.format(output)

    def main(self):

        fitsFiles=[]

        acquire_cmd= [ self.cmd, 'test']
        acquire= subprocess.Popen( acquire_cmd, stdin=subprocess.PIPE,stdout=subprocess.PIPE)

        # focuser dialog
        output= acquire.stdout.readline()
        if( self.verbose):
            print 'rts2af_feed_acquire, output: >>{0}<<, focuser'.format(output, self.focuser)
        acquire.stdin.write('{0}\n'.format(self.focuser))

        # filtersInUse dialog
        output= acquire.stdout.readline()
        #print 'rts2af_feed_acquire, filter request {0}'.format(output)
        for filter in self.filtersInUse:
            acquire.stdin.write('{0} '.format(filter))

        acquire.stdin.write('\n')

        # loop over the different focus run directories
        for storePath in  self.storePath:
            
            fitsFiles= glob.glob( storePath + '/' + '*fits')
            # first file is the reference catalogue
            self.ignoreOutput(acquire)
            #time.sleep(.1)
            acquire.stdin.write('{0}\n'.format('exposure_end'))
            popoff= self.referenceFile.pop(0)
            if( self.verbose):
                print 'rts2af_feed_acquire: poping reference file {0}'.format(popoff)

            acquire.stdin.write('img {0} \n'.format( storePath + '/' + popoff))
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
                print 'rts2af_feed_acquire.py: from rts2af_acquire.py: {0}'.format( acquire.stdout.readline())
            except:
                print 'rts2af_feed_acquire.py: cought exception, breaking'
                break
            time.sleep(1)

if __name__ == '__main__':
    main(sys.argv[0]).main()

#!/usr/bin/python
# (C) 2010, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rtsaf_acquire.py --help
#   
#   not yet see man 1 rtsaf_acquire
#
#   Basic usage:
#
#   Then run, e.g.
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
#
# required packages:

__author__ = 'markus.wildi@one-arcsec.org'

import sys
import subprocess
import time
import ConfigParser

import rts2comm
import rts2af 


r2c= rts2comm.Rts2Comm()


class Acquire(rts2af.AFScript):
    """control telescope and CCD, acquire a series of focuser images"""
    def __init__(self, scriptName='main'):
        self.scriptName= scriptName
        self.focuser = r2c.getValue('focuser')
        self.serviceFileOp= rts2af.serviceFileOp= rts2af.ServiceFileOperations()
        self.runTimeConfig= rts2af.runTimeConfig= rts2af.Configuration() # default config
        self.runTimeConfig.readConfiguration('/etc/rts2/rts2af/rts2af-acquire.cfg') # rts2 exe mechanism has options
        self.test= False #True: feed rts2af_acquire.py with rts2af_feed_acquire.py

    def acquireImage(self, focToff=None, exposure=None, filter=None, analysis=None):
        r2c.setValue('exposure', exposure)
        r2c.setValue('FOC_TOFF', focToff, self.focuser)

        r2c.log('I','XX-----starting {0}s exposure on offset {1}'.format(exposure, focToff))
        acquisitionPath = r2c.exposure()

        if( acquisitionPath==None): # used with rts2af_feed_acquire.py
            return

        r2c.log('I','YY-----XSWending {0}s exposure on offset {1}'.format(exposure, focToff))
        storePath=self.serviceFileOp.expandToAcquisitionBasePath(filter) + acquisitionPath.split('/')[-1]
        r2c.log('I','acquired {0} storing at {1}'.format(acquisitionPath, storePath))
        if( not self.test):
            r2c.move(acquisitionPath, storePath)

        try:
            if(self.test):
                analysis.stdin.write(acquisitionPath + '\n')
            else:
                analysis.stdin.write(storePath + '\n')

        except:
            r2c.log('E','could not write to pipe: {0},  {1}'.format(acquisitionPath, storePath))

        return

    def prepareAcquisition(self, nofilter, filter):
        r2c.setValue('FOC_FOFF',   filter.focDef, self.focuser)
        r2c.setValue('FOC_TOFF',               0, self.focuser)
        r2c.setValue('FOC_DEF' , nofilter.focDef, self.focuser)
        r2c.setValue('SHUTTER','LIGHT')
        r2c.setValue('filter' , filter.name)

    def saveState(self):
        return

    def run(self):
        args      = self.arguments()
        logger    = self.configureLogger()


        # start analysis process
        analysis={}

        nofilter= self.runTimeConfig.filterByName( 'NOFILTER')

        for fltName in self.runTimeConfig.filtersInUse:
            filter= self.runTimeConfig.filterByName( fltName)
            r2c.log('I','Filter {0}, offset {1}, expousre {2}'.format( filter.name, filter.focDef, filter.exposure))

            fileName= '/tmp/rts2af-acquire-' + filter.name + '.cfg'
            self.runTimeConfig.writeConfigurationForFilter(fileName, fltName)

            cmd= [ '/usr/local/src/rts-2-head/scripts/rts2af_analysis.py',
                   '--config', fileName
                   ]

            self.prepareAcquisition( nofilter, filter)
            self.serviceFileOp.createAcquisitionBasePath( filter)

            # open the analysis suprocess
            try:
                analysis[filter.name] = subprocess.Popen( cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
                r2c.log('I','Reference catalogue: Filter {0}, position {1}, exposure {2}'.format( filter.name, filter.focDef, filter.exposure))
            except:
                r2c.log('E','could not start {0}: Filter {1}, position {2}, exposure {3}'.format( cmd, filter.name, filter.focDef, filter.exposure))

            
            # create the reference catalogue
            self.acquireImage( 0, filter.exposure, filter, analysis[filter.name]) # exposure will later depend on position
            r2c.log('I','rts2af_acquire: received from pipe: {0}'.format(analysis[filter.name].stdout))


            for setting in filter.settings:
                r2c.log('I','===========Filter {0} offset {1} exposure {2}'.format(filter.name, setting.offset, setting.exposure))
                self.acquireImage( setting.offset, setting.exposure, filter, analysis[filter.name])
                

            r2c.log('I','focusing exposures finished for filter {0}'.format(filter.name))

        # read out the NOFILTER value
#        r2c.log( 'I', 'FOC_POS'.format(analysis['X'].stdout.readline()))

        return


if __name__ == '__main__':
    acq= Acquire(sys.argv[0])
    acq.run()

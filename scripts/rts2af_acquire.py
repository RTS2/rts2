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

import rts2comm
import rts2af 


r2c= rts2comm.Rts2Comm()


class Acquire(rts2af.AFScript):
    """control telescope and CCD, acquire a series of focuser images"""
    def __init__(self, scriptName='main'):
        self.scriptName= scriptName
        self.focuser = r2c.getValue('focuser')

    def acquireImage(self, focToff=None, exposure=None, filter=None, analysis=None):
        r2c.setValue('exposure', exposure)
        r2c.setValue('FOC_TOFF', focToff, self.focuser)

        r2c.log('I','XX-----starting {0}s exposure on offset {1}'.format(exposure, focToff))
        acquisitionPath = r2c.exposure()
        r2c.log('I','YY-----XSWending {0}s exposure on offset {1}'.format(exposure, focToff))
        storePath=rts2af.serviceFileOp.expandToAcquisitionBasePath(filter) + acquisitionPath.split('/')[-1]
        r2c.log('I','acquired {0} storing at {1}'.format(acquisitionPath, storePath))
        r2c.move(acquisitionPath, storePath)
        analysis.stdin.write(storePath + '\n')

        return

    
    def prepareAcquisition(self, filter):
        r2c.setValue('FOC_FOFF',      0, self.focuser)
        r2c.setValue('FOC_TOFF',      0, self.focuser)
        r2c.setValue('FOC_DEF' , filter.focDef, self.focuser)
        r2c.setValue('SHUTTER','LIGHT')
        r2c.setValue('filter' , filter.name)


    def saveState(self):
        return

    def run(self):
        rts2af.serviceFileOp= rts2af.ServiceFileOperations()
        runTimeConfig= rts2af.runTimeConfig = rts2af.Configuration()
        args      = self.arguments()
        logger    = self.configureLogger()
        rts2af.serviceFileOp= rts2af.ServiceFileOperations()

        configFileName='/etc/rts2/rts2af/rts2af-acquire.cfg'

        runTimeConfig.readConfiguration(configFileName)

        # start analysis process
        cmd= [ '/usr/local/src/rts-2/scripts/rts2af_analysis.py',
               '--config', '/usr/local/src/rts-2/scripts/rts2af-analysis.cfg'
             ]
#        cmd= [ '/usr/local/src/rts-2/scripts/test_analysis.py']
        analysis={}

        for fltName in runTimeConfig.filtersInUse:
            filter= runTimeConfig.filterByName( fltName)
            r2c.log('I','Filter {0} expousre {1}'.format( filter.name, filter.focDef))
            self.prepareAcquisition( filter)
            rts2af.serviceFileOp.createAcquisitionBasePath( filter)
#            analysis[filter.name] = subprocess.Popen( cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr= subprocess.PIPE)
            analysis[filter.name] = subprocess.Popen( cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
            # create the reference catalogue
            r2c.log('I','Reference catalogue: Filter {0}, position {1}, exposure {2}'.format( filter.name, filter.focDef, filter.exposure))
            
            self.acquireImage( 0, filter.exposure, filter, analysis[filter.name]) # exposure will later depend on position
#           r2c.log('I','received from pipe: {0}'.format(analysis[filter.name].stdout))
            for setting in filter.settings:
                r2c.log('I','===========Filter {0} offset {1} exposure {2}'.format(filter.name, setting.offset, setting.exposure))
                self.acquireImage( setting.offset, setting.exposure, filter, analysis[filter.name])
                
            r2c.log('I','focusing exposures finished, processing data')

        return


if __name__ == '__main__':
    acq= Acquire(sys.argv[0])
    acq.run()

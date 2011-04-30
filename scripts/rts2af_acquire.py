#!/usr/bin/python
# (C) 2011, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rts2af_acquire.py --help
#   
#   not yet see man 1 rts2af_acquire
#
#   Basic usage: rts2af_acquire.py
#
#   rts2af_acquire.py is called by rts2-executor on a target, e.g. 3 (focus),
#   or by the script rts2af_feed_acquire.py.
#
#   It isn't intended to be used interactively. 
#
#   rts2af_acquire.py's purpose is to acquire the images and then terminate
#   in order rts2-executor can immediately continue with the next target.
#
#   It sets the FOC_DEF value for the clear optical path if it is configured.
#   Clear optical path is defined as being filter named NOFILTER or if the
#   offset in the section [filter properties] is 0 (second element in the 
#   array)
#  
#   Planned: It spawns a subprocess to deal with the setting of the focuse.
#
#   rts2af_acquire.py's stdin, stdout and stderr are read by rts2-executor. Hence
#   logging is done via rts2comm.py.
#
#   The configuration file /etc/rts2/rts2af/rts2af-acquire.cfg is hardwired below
#   because EXEC can't (yet) execute scripts with arguments. 
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
import time
import re
import subprocess

import rts2comm
import rts2af 

r2c= rts2comm.Rts2Comm()

class Acquire(rts2af.AFScript):
    """control telescope and CCD, acquire a series of focuser images and eventually set the focus"""
    def __init__(self, scriptName=None, test=None):
        self.scriptName= scriptName
        self.focuser = r2c.getValue('focuser')
        self.serviceFileOp= rts2af.serviceFileOp= rts2af.ServiceFileOperations()
        self.runTimeConfig= rts2af.runTimeConfig= rts2af.Configuration() # default config
        self.runTimeConfig.readConfiguration('/etc/rts2/rts2af/rts2af-acquire.cfg') # rts2 exe mechanism has no options
        self.testFiltersInUse=[]
        if( test==None):
            self.test= False # normal production 
        else:
            self.test= True  #True: feed rts2af_acquire.py with rts2af_feed_acquire.py
            # retrieve the list of used filters from rts2af_feed_acquire.py
            print 'filtersInUse'
            sys.stdout.flush()
            self.testFiltersInUse= sys.stdin.readline().split()            
            r2c.log('I','rts2af_acquire.py: being in test mode, filters {0}'.format(self.testFiltersInUse))

        self.lowerLimit= self.runTimeConfig.value('FOCUSER_ABSOLUTE_LOWER_LIMIT')
        self.upperLimit= self.runTimeConfig.value('FOCUSER_ABSOLUTE_UPPER_LIMIT')
    
        self.binning= self.runTimeConfig.value(self.runTimeConfig.ccd.binning)
        self.windowOffsetX= self.runTimeConfig.ccd.windowOffsetX
        self.windowOffsetY= self.runTimeConfig.ccd.windowOffsetY
        self.windowWidth= self.runTimeConfig.ccd.windowWidth
        self.windowHeight= self.runTimeConfig.ccd.windowHeight

    def focPosWithinLimits(self, focPos=None):

        if( focPos < self.lowerLimit):
            r2c.log('E','rts2af_acquire: focDef below minimum: {0}, setting it to {1}'.format((focPos), self.lowerLimit)) 
            return False

        if( focPos > self.upperLimit):
            r2c.log('E','rts2af_acquire: focDef above maximum: {0}, setting it to {1}'.format((focPos), self.upperLimit))
            return False

        return True

    def focPosReached(self, focPos=None):
        """Sometimes the FLI focuser does not react, log that ToDo: later apply correction"""
        if(not focPos==None):
            i= 0
            while(True):
                # let the focuser move
                time.sleep(2)
                i += 1
                if(self.test):
                    r2c.log('I','rts2af_acquire: test mode current foc_pos: {0}'.format(focPos))
                    break
                else:
                    cur_foc_pos= r2c.getValueFloat('FOC_POS',self.focuser)
                    if( abs(float(cur_foc_pos-focPos) < self.runTimeConfig.value('FOCUSER_RESOLUTION'))):
                        r2c.log('I','rts2af_acquire: current foc_pos: {0}'.format(cur_foc_pos))
                        break
                    elif( i > 20):
                        r2c.log('E','rts2af_acquire: breaking, could not set {0}, current foc_pos: {1}'.format(focPos, cur_foc_pos))
                        break
        else:
            r2c.log('E','rts2af_acquire: no focuser position given')

    def acquireImage(self, focDef=None, focToff=None, exposure=None, filter=None, analysis=None):

        if( not self.focPosWithinLimits( focDef + focToff + filter.OffsetToClearPath)):
            return False

        r2c.setValue('exposure', exposure)
        r2c.setValue('FOC_TOFF', focToff, self.focuser)
        self.focPosReached( focDef + focToff + filter.OffsetToClearPath)

        acquisitionPath = r2c.exposure()

        # move all fits files of a given filter focus run into a separate directory 
        # in test mode the files are fetched for the original 
        storePath=self.serviceFileOp.expandToAcquisitionBasePath(filter) + acquisitionPath.split('/')[-1]
        if( not self.test):
            r2c.log('I','acquired {0} storing at {1}'.format(acquisitionPath, storePath))
            r2c.move(acquisitionPath, storePath)

        try:
            if(self.test):
                q_match= re.search( r'Q', acquisitionPath)
                analysis.stdin.write(acquisitionPath + '\n')

                if( not q_match==None):
                    return False
                else:
                    return True
            else: # storePath ok
                analysis.stdin.write(storePath + '\n')
                return True

        except:
            if(self.test):
                r2c.log('E','could not write to pipe: {0}'.format(acquisitionPath))
            else:
                r2c.log('E','could not write to pipe: {0}'.format(storePath))
                
            return False

        return False

    def prepareAcquisition(self, focDef, filter):
        r2c.setValue('SHUTTER', 'LIGHT')
        r2c.setValue('binning', self.binning)
        r2c.setValue('WINDOW','%d %d %d %d' % (self.windowOffsetX, self.windowOffsetY, self.windowWidth, self.windowHeight))

        if( filter.name != 'NOFILTER'):
            r2c.setValue('filter' , filter.name)
        # ToDo: verify with limits
        r2c.setValue('FOC_FOFF', filter.OffsetToClearPath, self.focuser)
        r2c.setValue('FOC_TOFF', 0, self.focuser)
        r2c.setValue('FOC_DEF' , focDef, self.focuser)
        self.focPosReached(focDef + filter.OffsetToClearPath)
        

    def saveState(self):
        return

    def run(self):
        # telescope parameter
        telescope=rts2af.Telescope() # take the defaults from the config file or overwrite them here

        # start analysis process
        analysis={}
        nofilter= self.runTimeConfig.filterByName( 'NOFILTER')

        # create the result file for the fitted values, read by rts2af_set_fit_focus.py
        fitResultFileName= self.serviceFileOp.expandToFitResultPath( 'rts2af-acquire-')
        with open( fitResultFileName, 'w') as frfn:
            frfn.write('# {0}, written by rts2af_acquire.py\n#\n'.format(fitResultFileName))
        # rts2af_acquire.py will supply the values asynchronously
        frfn.close()

        filtersInUse=[]
        if(self.test):
            filtersInUse= self.testFiltersInUse
        else:
            filtersInUse= self.runTimeConfig.filtersInUse

        fwhm_foc_pos_cp= -1
        for fltName in filtersInUse:

            filter= self.runTimeConfig.filterByName( fltName)

            if( fwhm_foc_pos_cp > 0):
                focDef= fwhm_foc_pos_cp
                self.prepareAcquisition( focDef, filter) # a previous run was successful
                r2c.log('I','Initial setting: filter {0}, offset {1}, expousre {2} (setting from clear path run)'.format( filter.name, fwhm_foc_pos_cp, filter.exposure))
            else:
                # depends on what one wants
                focDef= nofilter.OffsetToClearPath
                self.prepareAcquisition( focDef, filter) # use the config file value
                r2c.log('I','Initial setting: filter {0}, offset {1}, expousre {2}'.format( filter.name, filter.OffsetToClearPath, filter.exposure))

            configFileName= self.serviceFileOp.expandToTmpConfigurationPath( 'rts2af-acquire-' + filter.name + '-') 

            self.runTimeConfig.writeConfigurationForFilter(configFileName, fltName)
            self.serviceFileOp.createAcquisitionBasePath( filter)

            cmd= [ 'rts2af_analysis.py',
                   '--config', configFileName
                   ]

            
            r2c.log('I','COMMAND {0} {1}'.format(cmd, filter.name))
            # open the analysis suprocess
            try:
                analysis[filter.name] = subprocess.Popen( cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
                #r2c.log('I','Reference catalogue: Filter {0}, position {1}, exposure {2}'.format( filter.name, filter.OffsetToClearPath, filter.exposure))
            except:
                r2c.log('E','rts2af_acquire: exiting, could not start {0}: Filter {1}, position {2}, exposure {3}'.format( cmd, filter.name, filter.OffsetToClearPath, filter.exposure))
                sys.exit(1)

            # create the reference catalogue
            self.acquireImage( focDef, 0, filter.exposure, filter, analysis[filter.name]) # exposure will later depend on position

            if(self.test):
                while( True):
                    r2c.log('I','rts2af_acquire: being in test mode Filter {0} offset {1} exposure {2}'.format(focDef, 1443, 11))
                    if( not self.acquireImage( focDef, 1443, 11, filter, analysis[filter.name])):
                        break
            else:
                # loop over the focuser steps
                for setting in filter.settings:
                    if(( abs(setting.offset) < 410) or ( abs(setting.offset)==720)): # to reduce the time, ToDo: a true general solution
                        exposure=  telescope.linearExposureTimeAtFocPos(setting.exposure, setting.offset)
                        r2c.log('I','rts2af_acquire: Filter {0} offset {1} exposure {2}, true exposure: {3}'.format(filter.name, setting.offset, setting.exposure, exposure))
                        self.acquireImage( focDef, setting.offset, exposure, filter, analysis[filter.name])
                
                # signal rts2af_analysis.py to continue with fitting
                analysis[filter.name].stdin.write('Q\n')

            r2c.log('I','rts2af_acquire: focuser exposures finished for filter {0}'.format(filter.name))

            # set the FOC_DEF for the clear optical path
            if( self.runTimeConfig.value('SET_FOCUS')):
                if((filter.OffsetToClearPath== 0) or ( filter.name=='NOFILTER')):
                    r2c.log('I','rts2af_acquire: waiting for fitted focus position')
                    fwhmFocPosStr= analysis[filter.name].stdout.readline()
                    fwhmFocPosMatch= re.search( r'FOCUS: ([0-9]+)', fwhmFocPosStr)

                    if( not fwhmFocPosMatch == None):
                        fwhmFocPos= int(fwhmFocPosMatch.group(1))
                        r2c.log('I','rts2af_acquire: got fitted focuser position at: {0}'.format(fwhmFocPos))
                        # ToDo: create a method wich verifies FOC_POS
                        if( self.focPosWithinLimits( fwhmFocPos + filter.OffsetToClearPath)):

                            r2c.setValue('FOC_FOFF', 0, self.focuser)
                            time.sleep(2)
                            r2c.setValue('FOC_TOFF', 0, self.focuser)
                            time.sleep(2)
                            r2c.setValue('FOC_DEF', fwhmFocPos, self.focuser)

                            self.focPosReached(fwhmFocPos)
                        else:
                            r2c.log('E','rts2af_acquire: can not set FOC_DEF, out of limits'.format(fwhmFocPos))
                    else:
                        r2c.log('E','rts2af_acquire: no match of string FOCUS: ([0-9]+) on {0}, doing nothing'.format(fwhmFocPos))
                else:
                    r2c.log('I','rts2af_acquire: not waiting for fit results for filter {0}'.format(filter.name))
            else:
                r2c.log('I','rts2af_acquire: not attempting to set focus')

        # completed
        r2c.log('I','rts2af_acquire: focuser exposures finished for all filters, spawning rts2af_set_fit_focus.py')
        # rts2af_set_fit_focus.py sets FOC_DEF and writes the filter offsets
        # will use that after Petr took a look at exe ... with arguments
        #cmd= [ '/usr/local/src/rts-2-head/scripts/rts2af_set_fit_focus.py',
        #       , fitResultFileName, self.runTimeConfig.filtersInUse
        #       ]
        # here we do not care about success
        #subprocess.Popen( cmd)
        
        return

if __name__ == '__main__':
# if any extra argument is present then rts2af_acquire.py is executed in test mode
    if( len(sys.argv)== 2):
        acquire= Acquire(scriptName=sys.argv[0], test=sys.argv[1])
    else:
        acquire= Acquire(scriptName=sys.argv[0])

    acquire.run()

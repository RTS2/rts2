#!/usr/bin/python
# (C) 2011, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rts2af_model.py --help
#   
#   not yet see man 1 rts2af_model
#
#   Basic usage: rts2af_model.py
#
#
#   rts2af_acquire.py's purpose is to acquire the images and then terminate
#   in order rts2-executor can immediately continue with the next target.
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

__author__ = 'markus.wildi@one-arcsec.org'

import sys
import time
import re
import os
import subprocess

import rts2af 

class Model(rts2af.AFScript):
    """Script to extract minimum FWHM, date/time and temperature as input for a focuser temperature model"""
    def __init__(self, scriptName=None):
        self.scriptName= scriptName
        self.serviceFileOp= rts2af.serviceFileOp= rts2af.ServiceFileOperations()
        self.runTimeConfig= rts2af.runTimeConfig= rts2af.Configuration() # default config
        self.runTimeConfig.readConfiguration('/etc/rts2/rts2af/rts2af-model.cfg') 
        self.cmdFwhm= 'rts2af_fwhm_model.py'
        self.cnfFwhm= '/etc/rts2/rts2af/rts2af-fwhm.cfg'

        self.cmdOffline= '/usr/local/src/rts-2-head/scripts/rts2af_offline.py'
        self.cnfOffline= '/etc/rts2/rts2af/rts2af-offline.cfg'

    def run(self):
        filtersInUse= self.runTimeConfig.filtersInUse

        # walk through the base directory and find all directories for a given filter
        for root, dirs, names in os.walk(self.runTimeConfig.value('BASE_DIRECTORY')):
            if(re.search( r'X', root)):
                if(len(names)>= self.runTimeConfig.value('MINIMUM_FOCUSER_POSITIONS')):
                    #print '{0} {1} {2} {3}'.format(len(names), root, dirs, names)
                    # if found: find a file name with reference.fits ending or the fits file with the lowest FWHM 
                    bz2= False
                    referenceFileName= ''
                    for name in names:
                        if(re.search( r'reference.fits', name)):
                            print 'rts2af_model.py: ------------------------'
                            print 'rts2af_model.py: named reference file found: {0}'.format(name)
                            referenceFileName= name
                            break
                    else:
                        # find the one with the smalest FWHM
                        fitsNameFwhm={}
                        for name in names:
                            # skip those having bz2
                            if(re.search( r'bz2', name) ):
                                bz2= True
                                break

                            cmd= [  self.cmdFwhm,
                                    '--config',
                                    self.cnfFwhm,
                                    '--reference',
                                    '{0}/{1}'.format(root, name)
                                    ]
                            try:
                                output = subprocess.Popen( cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0].strip()
                                
                            except OSError as (errno, strerror):
                                logging.error( 'Catalogue.runSExtractor: I/O error({0}): {1}'.format(errno, strerror))
                                sys.exit(1)

                            except:
                                logging.error('Catalogue.runSExtractor: '+ repr(cmd) + ' died')
                                sys.exit(1)

                            fwhmMatch= re.search( r'FWHM:([\.0-9]+)', output)
                            if( not fwhmMatch==None):
                                fwhm= float(fwhmMatch.group(1))
                            else:
                                print 'rts2af_model.py: matching fwhm, something went wrong for file: {0}, reference file name: {1}'.format(name, referenceFileName)

                            fitsNameFwhm[name]=fwhm
                        else:
                            b = dict(map(lambda item: (item[1],item[0]), fitsNameFwhm.items()))

                            try:
                                referenceFileName = b[min(b.keys())]
                            except:
                                print 'rts2af_model.py: finding reference file: {0}, something went wrong'.format(referenceFileName)

                            print 'rts2af_model.py: ------------------------'
                            print 'rts2af_model.py: minimum: fits file name: {0}, FWHM: {1}'.format(referenceFileName, fitsNameFwhm[referenceFileName])
                    # take care later
                    if( bz2):
                        continue

                    # reference file identified, fit it
                    cmd= [  self.cmdOffline,
                            '--config',
                            self.cnfOffline,
                            '--reference',
                            '{0}'.format(referenceFileName)
                            ]
                    try:
                        analysis= subprocess.Popen( cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
                    except:
                        print 'rts2af_model.py: spawning offline analysis, something went wrong: {0}'.format(cmd)
                        sys.exit(1)
                    while True: 
                        try:
                            outputLines= analysis.communicate()
                        except:
                            break

                        for line in outputLines:
                            try:
                                line= line.replace('\n',' ')
                            except:
                                break

                            print 'rts2af_model.py: line -->>{0}<<--'.format(line)
                            try:
                                focPosMatch= re.search( r'FOCUS: ([\.0-9]+)', line)
                            except:
                                print 'rts2af_model.py: something went wrong with reference file: {0}'.format(referenceFileName)
                                break 

                            if( not focPosMatch==None):
                                focPos= float(focPosMatch.group(1))
                                print 'rts2af_model.py:FOCUS: {0}'.format(focPos)
                                break

if __name__ == '__main__':
    model= Model(scriptName=sys.argv[0])
    model.run()

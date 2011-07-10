#!/usr/bin/python
# (C) 2011, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rts2af_model_extract.py --help
#   
#   not yet see man 1 rts2af_model_extract
#
#   Basic usage: rts2af_model_extract.py
#
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
import shutil
import argparse

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
        self.bzip2 = 'bzip2'
        self.trashDirectory= '/scratch/focus-trash'

    def bz2Prepare(self, filePath=None, baseDirectory='/scratch/focus/bz2tmp'):
        targetDirectory= None

        if(os.path.exists(baseDirectory)):
            shutil.rmtree( baseDirectory)

#        sourceDirectory= os.path.dirname(filePath)
        sourceDirectory= filePath

        items= filePath.split('/')
        targetDirectory= baseDirectory + os.sep + items[-2] + os.sep + items[-1]

        print 'bz2Prepare: {0} {1}'.format(sourceDirectory, targetDirectory)

        if(not os.path.exists(targetDirectory)):
            os.makedirs( targetDirectory)

        files = os.listdir(sourceDirectory)

        for file in files:
            path = sourceDirectory + os.sep + file
            shutil.copy (path, targetDirectory + os.sep + file)
#            print 'copy: {0} {1}'.format(path, directory + os.sep + file)

            cmd= [  self.bzip2,
                    '-d',
                    targetDirectory + os.sep + file,
                    ]

            try:
                output = subprocess.Popen( cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0].strip()
                
            except OSError as (errno, strerror):
                logging.error( 'rts2af_model_extract.py: I/O error({0}): {1}'.format(errno, strerror))
                sys.exit(1)
                
            except:
                logging.error('rts2af_model_extract.py: '+ repr(cmd) + ' died')
                sys.exit(1)
#            print 'decompressed: {0}'.format(directory + os.sep + file)

        return targetDirectory

    def run(self):


        args= self.arguments()
        filtersInUse= self.runTimeConfig.filtersInUse

        # ToDo: walk through the base directory and find all directories for a given filter
        # now: X hardcoded
        #
        filterX= 'X'
        date1= '2011-07-04'
        date2= '2011-07-05'
        date3= '2011-07-09'
        for root, dirs, names in os.walk(self.runTimeConfig.value('BASE_DIRECTORY')):

            if(not (re.search( date1, root) or re.search( date1, root) or re.search( date3, root))):
                print '----------{0}'.format(root)
                continue

            if(re.search( filterX, root)): #ToDo: make a configurable variable
                if(len(names)<= self.runTimeConfig.value('MINIMUM_FOCUSER_POSITIONS')):
                    print 'rts2af_model.py: only {0} files, required: {1}, in: {2}'.format(len(names), self.runTimeConfig.value('MINIMUM_FOCUSER_POSITIONS'), root)
                    (runDirectory, seperator, afterSeparator)= root.rpartition( '/' + filterX)
                    print 'rts2af_model.py: moving director: {0} to: {1}'.format( runDirectory, self.trashDirectory)
                    shutil.move(runDirectory, self.trashDirectory)
                    continue
                else:


                    #print '{0} {1} {2} {3}'.format(len(names), root, dirs, names)
                    # if found: find a file name with reference.fits ending or the fits file with the lowest FWHM 
                    targetDirectory=''
                    referenceFileName= ''
                    cont = False
                    for name in names:
                        if(re.search( r'reference.fits', name)):
                            print 'rts2af_model.py: ------------------------'
                            print 'rts2af_model.py: named reference file found: {0}'.format(name)
                            targetDirectory= root
                            referenceFileName= name
                            break
#                        if(re.search( r'reference.fits.bz2', name)):
#                            print 'rts2af_model.py: ------------------------'
#                            print 'rts2af_model.py: named reference file found: {0}'.format(name)
#                            referenceFileName= name.replace('.bz2', '')
#                            targetDirectory=self.bz2Prepare( root)
#                            print 'rts2af_model.py: bz2 target directory: {0}'.format(targetDirectory)
#                            break
                    else:
                        # find the one with the smallest FWHM
                        fitsNameFwhm={}
                        
                        canonicalName= ''
                        namesTmp=[]
                        for name in names:
                            pass
#                            if(re.search( r'fits.bz2', name)):
#                                print 'rts2af_model.py: bz2 file found: {0}'.format(name)
#                                referenceFileName= name
#                                targetDirectory= self.bz2Prepare(root)
#                                namesTmp = os.listdir(targetDirectory)
#                                break
                        else:
                            namesTmp= names
                            targetDirectory= root

                        if(targetDirectory==None):
                            namesTmp=[]
                            cont= True
                            

                        for name in namesTmp:
                            nameTmp= name.split('/')
                            canonicalName= targetDirectory + os.sep +  nameTmp[-1].replace('.bz2', '')

                            cmd= [  self.cmdFwhm,
                                    '--config',
                                    self.cnfFwhm,
                                    '--reference',
                                    canonicalName
                                    ]
                            try:
                                output = subprocess.Popen( cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0].strip()
                                
                            except OSError as (errno, strerror):
                                logging.error( 'Catalogue.runSExtractor: I/O error({0}): {1}'.format(errno, strerror))
                                sys.exit(1)

                            except:
                                logging.error('Catalogue.runSExtractor: '+ repr(cmd) + ' died')
                                sys.exit(1)
 
                            fwhm=''
                            fwhmMatch= re.search( r'FWHM:([\-\.0-9]+)', output) # include -1
                            if( not fwhmMatch==None):
                                fwhm= float(fwhmMatch.group(1))
                            else:
                                print 'rts2af_model.py: matching fwhm, something went wrong for file: {0}, reference file name: {1}'.format(name, referenceFileName)
                                print 'output: len: {0} string: {1}'.format(len(output), output)
                                print 'cmd: {0}'.format( cmd)
                            if( fwhm > 0):
                                fitsNameFwhm[name]=fwhm
                            else:
                                print 'rts2af_model.py: matching fwhm, no objects found in: {0}, reference file name: {1}'.format(name, referenceFileName)

                        else:
                            if( not cont):
                                b = dict(map(lambda item: (item[1],item[0]), fitsNameFwhm.items()))

                                try:
                                    referenceFileName = b[min(b.keys())]
                                    print 'rts2af_model.py: ------------------------'
                                    print 'rts2af_model.py: minimum: fits file name: {0}, FWHM: {1}'.format(referenceFileName, fitsNameFwhm[referenceFileName])
                                except:
                                    cont = True
                                    print 'rts2af_model.py: finding reference in: {0} something went wrong'.format(targetDirectory)


                    if( cont):
                        print 'rts2af_model.py: ------------------------ CONT {0}'.format(referenceFileName)
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
                        print 'rts2af_model_extract.py: spawning offline analysis, something went wrong: {0}'.format(cmd)
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

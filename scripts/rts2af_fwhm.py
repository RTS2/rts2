#!/usr/bin/python
# (C) 2010, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   find /pathe/to/fits -name "*fits" -exec rts2af_fwhm.py  --config rts2-autofocus-offline.cfg --reference {} \; 
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
# wget 'http://argparse.googlecode.com/files/argparse-1.1.zip

__author__ = 'markus.wildi@one-arcsec.org'

import io
import os
import re
import shutil
import string
import sys
import time
from operator import itemgetter, attrgetter

import numpy
import pyfits
import rts2comm 
import rts2af 

class FWHM(rts2af.AFScript):
    """extract the catalgue of an images"""
    def __init__(self):
        rts2af.AFScript.__init__(self)

    def runSex(self,filename):
        """Run sextractor, return catalogue of found sources"""
        self.configureLoggerStdout()
        rts2af.runTimeConfig = rts2af.Configuration()
        rts2af.runTimeConfig.readDefaults()
        rts2af.serviceFileOp = rts2af.ServiceFileOperations()

        # rts2af.runTimeConfig.readConfiguration(rts2af.runTimeConfig.configurationFileName())

        sex = rts2af.SExtractorParams()
        sex.readSExtractorParams()

        hdu = rts2af.FitsHDU(filename)
        hdu.headerProperties()

        paramsSexctractor = rts2af.SExtractorParams()
        paramsSexctractor.readSExtractorParams()

        self.cat = rts2af.ReferenceCatalogue(hdu,paramsSexctractor)
        self.cat.runSExtractor()
        self.cat.createCatalogue()

        return self.cat

    def run(self):
        runTimeConfig= rts2af.runTimeConfig = rts2af.Configuration()
        args      = self.arguments()
        logger    = self.configureLogger()
        rts2af.serviceFileOp= rts2af.ServiceFileOperations()

        configFileName=''
        if( args.fileName):
            configFileName= args.fileName[0]  
        else:
            configFileName= runTimeConfig.configurationFileName()
            cmd= 'logger no config file specified, taking default' + configFileName
            #print cmd 
            os.system( cmd)

        runTimeConfig.readConfiguration(configFileName)

        if( args.referenceFitsFileName):
            referenceFitsFileName = args.referenceFitsFileName[0]
#            if( not rts2af.serviceFileOp.defineRunTimePath(referenceFitsFileName)):
#                logger.error('main: reference file '+ referenceFitsFileName + ' not found in base directory ' + runTimeConfig.value('BASE_DIRECTORY'))
#                sys.exit(1)
# read the SExtractor parameters
        paramsSexctractor= rts2af.SExtractorParams()
        paramsSexctractor.readSExtractorParams()


        if( paramsSexctractor==None):
            print "exiting"
            sys.exit(1)

        hdu= rts2af.FitsHDU( referenceFitsFileName)
        if(hdu.headerProperties()):
            print "append "+ hdu.fitsFileName
        else:
            print "append not "

        cat= rts2af.ReferenceCatalogue(hdu,paramsSexctractor)

        cat.runSExtractor()
        cat.createCatalogue()
        #cat.cleanUp()

        cat.average('FWHM_IMAGE')

if __name__ == '__main__':
    FWHM().run()




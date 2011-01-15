#!/usr/bin/python
# (C) 2010, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rtsaf_offline.py --help
#   
#   see man 1 rts2_catalogue.py
#   see rts2_autofocus_unittest.py for unit tests
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

class main(rts2af.AFScript):
    """extract the catalgue of an images"""
    def __init__(self, scriptName='main'):
        self.scriptName= scriptName

    def main(self):
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
            if( not rts2af.serviceFileOp.defineRunTimePath(referenceFitsFileName)):
                logger.error('main: reference file '+ referenceFitsFileName + 'not found in base directory ' + runTimeConfig.value('BASE_DIRECTORY'))
                sys.exit(1)
# get the list of fits files
        testFitsList=[]
        testFitsList=rts2af.serviceFileOp.fitsFilesInRunTimePath()
# read the SExtractor parameters
        paramsSexctractor= rts2af.SExtractorParams()
        paramsSexctractor.readSExtractorParams()

        if( paramsSexctractor==None):
            print "exiting"
            sys.exit(1)

# create the administrative objects 
        HDUs= rts2af.FitsHDUs()

# load the reference file first
        for fits in testFitsList:
            if( fits.find(referenceFitsFileName) >= 0):
                break
        else:
            logger.error("main: reference file " + referenceFitsFileName + " not found")
            sys.exit(1)

# create the reference catalogue
        hdur= rts2af.FitsHDU(referenceFitsFileName)

        if(hdur.headerProperties()):
            HDUs.fitsHDUsList.append(hdur)
            catr= rts2af.ReferenceCatalogue(hdur,paramsSexctractor)
            catr.runSExtractor()
            catr.createCatalogue()
            catr.cleanUpReference()
            catr.writeCatalogue()
            cats= rts2af.Catalogues(catr)
        else:
            sys.exit(1)
            
# read the files 
        for fits in testFitsList:
            hdu= rts2af.FitsHDU( fits, hdur)
            if(hdu.headerProperties()):
                if(rts2af.verbose):
                    print "append "+ hdu.fitsFileName
                HDUs.fitsHDUsList.append(hdu)

        if( not HDUs.validate()):
            logger.error("main: HDUs are not valid, exiting")
            sys.exit(1)

# loop over hdus (including reference catalog currently)
        for hdu  in HDUs.fitsHDUsList:
            if( rts2af.verbose):
                print '=======' + hdu.headerElements['FILTER'] + '=== valid=' + repr(hdu.isValid) + ' number of files at FOC_POS=%d' % hdu.headerElements['FOC_POS'] + ': %d' % HDUs.fitsHDUsList.count(hdu) + " " + hdu.fitsFileName
            
            cat= rts2af.Catalogue(hdu,paramsSexctractor, catr)
            cat.runSExtractor()
            cat.createCatalogue()
            cat.cleanUp()

            # append the catalogue only if there are more than runTimeConfig.value('MATCHED_RATIO') sxObjects 
            if( cat.matching()):
                print "Added catalogue at FOC_POS=%d" % hdu.headerElements['FOC_POS'] + " file "+ hdu.fitsFileName
                cats.CataloguesList.append(cat)
            else:
                logger.error("main: discarded catalogue at FOC_POS=%d" % hdu.headerElements['FOC_POS'] + " file "+ hdu.fitsFileName)

        if(cats.validate()):
            print "main: catalogues are valid"
        else:
            print "main: catalogues are invalid"

        for cat in sorted(cats.CataloguesList, key=lambda cat: cat.fitsHDU.headerElements['FOC_POS']):
            if(rts2af.verbose):
                print "fits file: "+ cat.fitsHDU.fitsFileName + ", %d " % cat.fitsHDU.headerElements['FOC_POS'] 
            cat.average('FWHM_IMAGE')


        cats.fitTheValues()
        #cats.average()
        #for focPos in sorted(fwhm):
        #    print "average %d %f %f" % (focPos, self.averageFwhm[focPos], self.averageFlux[focPos])

        #cats.printSelectedSXobjects()
        #cats.ds9DisplayCatalogues()
        cats.ds9WriteRegionFiles()
        
        logger.error("THIS IS THE END")
        print "THIS IS THE END"

if __name__ == '__main__':
    main(sys.argv[0]).main()




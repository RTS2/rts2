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
import logging
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
        rts2af.serviceFileOp= rts2af.ServiceFileOperations()

        configFileName='/etc/rts2/rts2af/rts2af-fwhm.cfg'
        if( args.fileName):
            configFileName= args.fileName[0]  
        else:
            configFileName= runTimeConfig.configurationFileName()
            logging.info('rts2af_fwhm.py: no config file specified, taking default: ' + configFileName)

        runTimeConfig.readConfiguration(configFileName)

        if( args.referenceFitsFileName):

            referenceFitsFileName = args.referenceFitsFileName[0]
            if( not rts2af.serviceFileOp.defineRunTimePath(referenceFitsFileName)):
                logging.error('rts2af_fwhm.py: reference file '+ referenceFitsFileName + ' not found in base directory ' + runTimeConfig.value('BASE_DIRECTORY'))
                sys.exit(1)
# read the SExtractor parameters
        paramsSexctractor= rts2af.SExtractorParams()
        paramsSexctractor.readSExtractorParams()


        if( paramsSexctractor==None):
            print "exiting"
            sys.exit(1)

        try:
            hdu= rts2af.FitsHDU( referenceFitsFileName)
        except:
            logging.error('rts2af_fwhm.py: exiting, no reference file given')
            sys.exit(1)

        if(hdu.headerProperties()):
            logging.info('rts2af_fwhm.py: appending: {0}'.format(hdu.fitsFileName))
        else:
            logging.info('rts2af_fwhm.py: exiting due to invalid header properties inf file:{0}'.format(hdu.fitsFileName))

        cat= rts2af.ReferenceCatalogue(hdu,paramsSexctractor)

        cat.runSExtractor()
        cat.createCatalogue()
        cat.cleanUpReference()

        fwhm= cat.average('FWHM_IMAGE')
        logging.info('rts2af_fwhm.py, FWHM:{0}'.format(fwhm))

        threshFwhm= 4.
        if( fwhm > threshFwhm):
            r2c.setValue('next', 5, 'EXEC')
            logging.info('rts2af_fwhm.py: queueing a focus run at EXEC next, fwhm :{0}, threshold: {1}'.format(fwhm, threshFwhm))
        else:
            logging.info('rts2af_fwhm.py: no focus run necessary, fwhm :{0}, threshold: {1}'.format(fwhm, threshFwhm))

        # does not work yet cat.ds9WriteRegionFile(writeSelected=True)
        #cat.displayCatalogue()

if __name__ == '__main__':
    main(sys.argv[0]).main()




#!/usr/bin/python
# (C) 2010, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   find /path/to/fits -name "*fits" -exec rts2af_fwhm.py  --config rts2-autofocus-offline.cfg --reference {} \; 
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
import subprocess
from operator import itemgetter, attrgetter


import numpy
import pyfits
import rts2.scriptcomm 
import rts2af 


r2c= rts2.scriptcomm.Rts2Comm()


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
            logging.debug('rts2af_fwhm.py: no config file specified, taking default: ' + configFileName)

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
            logging.error('rts2af_fwhm.py: exiting, no paramsSexctractor file given')
            sys.exit(1)

        try:
            hdu= rts2af.FitsHDU( referenceFitsFileName)
        except:
            logging.error('rts2af_fwhm.py: exiting, no reference file given')
            sys.exit(1)

        if(hdu.headerProperties()):
            logging.debug('rts2af_fwhm.py: appending: {0}'.format(hdu.fitsFileName))
        else:
            logging.error('rts2af_fwhm.py: exiting due to invalid header properties inf file:{0}'.format(hdu.fitsFileName))
            sys.exit(1)

        cat= rts2af.ReferenceCatalogue(hdu,paramsSexctractor)


        cat.runSExtractor()
        if( not cat.createCatalogue()):
            logging.error('rts2af_fwhm.py: returning due to invalid catalogue')
            return
        numberReferenceObjects=cat.cleanUpReference()
        if(numberReferenceObjects==0):
            logging.error('rts2af_fwhm.py: returning due to no objects found')
            return
                
        fwhm= cat.average('FWHM_IMAGE')
        logging.info('rts2af_fwhm.py: FWHM: {0}, {1}, {2}, {3}, {4}, {5}'.format(cat.fitsHDU.variableHeaderElements['FOC_POS'], fwhm, numberReferenceObjects, cat.fitsHDU.staticHeaderElements['FILTER'], cat.fitsHDU.variableHeaderElements['AMBIENTTEMPERATURE'], referenceFitsFileName))

        # While a focus run is in progress there might still images 
        # be analyzed.
        # No new focus run is then scheduled in rts2af-queue.
        filter= runTimeConfig.filterByName( hdu.staticHeaderElements['FILTER'])
        
        if( filter and filter.OffsetToClearPath== 0): # do focus run only if there is no filter, see filters NOF or X
            threshFwhm=runTimeConfig.value('THRESHOLD')
            if( fwhm > threshFwhm):
                ##r2c.setValue('next', 5, 'EXEC')
                ## plain wrong were are not talking to rts2, use rts2-scriptexec

                cmd= [ 'rts2af-queue',
                       '--user={0}'.format(runTimeConfig.value('USERNAME')),
                       '--password={0}'.format(runTimeConfig.value('PASSWORD')),
                       '--clear',
                       '--queue={0}'.format(runTimeConfig.value('QUEUENAME')),
                       '{0}'.format(runTimeConfig.value('TARGETID'))
                       ]
                fnull = open(os.devnull, 'w')
                proc=subprocess.Popen(cmd, shell=False, stdout = fnull, stderr = fnull)
 
                logging.info('rts2af_fwhm.py: try to queue a focus run at SEL queue: {0}, fwhm: {1}, threshold: {2}, command: {3}, based on reference file {4}'.format(runTimeConfig.value('QUEUENAME'), fwhm, threshFwhm, cmd, referenceFitsFileName))
            else:
                logging.info('rts2af_fwhm.py: no focus run necessary, fwhm: {0}, threshold: {1}, reference file: {2}'.format(fwhm, threshFwhm, referenceFitsFileName))
        else:
            # a focus run sets FOC_DEF and that is without filter
            if( filter):
                logging.info('rts2af_fwhm.py: queueing focus run only for clear path (no filter), used filter: {0}, offset: {1}'.format(filter.name, filter.OffsetToClearPath))
            else:
                logging.info('rts2af_fwhm.py: queueing focus run only for clear path (no filter), no known filter found in headers')


if __name__ == '__main__':
    main(sys.argv[0]).main()

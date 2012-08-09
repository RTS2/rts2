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


class Fwhm(rts2af.AFScript):
    """extract the catalgue of an images"""
    def run(self):
# read the SExtractor parameters
        paramsSexctractor= rts2af.SExtractorParams(paramsFileName=self.rtc.value('SEXREFERENCE_PARAM'))
        paramsSexctractor.readSExtractorParams()

        if( paramsSexctractor==None):
            logging.error('rts2af_fwhm.py: exiting, no paramsSexctractor file given')
            sys.exit(1)

        try:
            hdu= rts2af.FitsHDU( env=self.env, fitsFileName=self.referenceFitsName)
        except:
            logging.error('rts2af_fwhm.py: exiting, no reference file given')
            sys.exit(1)

        if(hdu.headerProperties()):
            logging.debug('rts2af_fwhm.py: appending: {0}'.format(hdu.fitsFileName))
        else:
            logging.error('rts2af_fwhm.py: exiting due to invalid header properties inf file:{0}'.format(hdu.fitsFileName))
            sys.exit(1)

        cat= rts2af.ReferenceCatalogue(env=self.env, fitsHDU=hdu,SExtractorParams=paramsSexctractor)


        cat.runSExtractor()
        if( not cat.createCatalogue()):
            logging.error('rts2af_fwhm.py: returning due to invalid catalogue')
            return
        numberReferenceObjects=cat.cleanUpReference()
        if(numberReferenceObjects==0):
            logging.error('rts2af_fwhm.py: returning due to no objects found')
            return
                
        fwhm= cat.average('FWHM_IMAGE')
        logging.info('rts2af_fwhm.py: FWHM: {0}, {1}, {2}, {3}, {4}, {5}'.format(cat.fitsHDU.variableHeaderElements['FOC_POS'], fwhm, numberReferenceObjects, cat.fitsHDU.staticHeaderElements['FILTER'], cat.fitsHDU.variableHeaderElements['AMBIENTTEMPERATURE'], self.referenceFitsName))

        # While a focus run is in progress there might still images 
        # be analyzed.
        # No new focus run is then scheduled in rts2af-queue.
        filter= self.env.rtc.filterByName( hdu.staticHeaderElements['FILTER'])
        
        if( filter and filter.OffsetToClearPath== 0): # do focus run only if there is no filter, see filters NOF or X
            threshFwhm=self.env.rtc.value('THRESHOLD')
            if( fwhm > threshFwhm):
                ##r2c.setValue('next', 5, 'EXEC')
                ## plain wrong were are not talking to rts2, use rts2-scriptexec

                cmd= [ 'rts2af-queue',
                       '--user={0}'.format(self.env.rtc.value('USERNAME')),
                       '--password={0}'.format(self.env.rtc.value('PASSWORD')),
                       '--clear',
                       '--queue={0}'.format(self.env.rtc.value('QUEUENAME')),
                       '{0}'.format(self.env.rtc.value('TARGETID'))
                       ]
                fnull = open(os.devnull, 'w')
                proc=subprocess.Popen(cmd, shell=False, stdout = fnull, stderr = fnull)
 
                logging.info('rts2af_fwhm.py: try to queue a focus run at SEL queue: {0}, fwhm: {1}, threshold: {2}, command: {3}, based on reference file {4}'.format(self.env.rtc.value('QUEUENAME'), fwhm, threshFwhm, cmd, self.referenceFitsName))
            else:
                logging.info('rts2af_fwhm.py: no focus run necessary, fwhm: {0}, threshold: {1}, reference file: {2}'.format(fwhm, threshFwhm, self.referenceFitsName))
        else:
            # a focus run sets FOC_DEF and that is without filter
            if( filter):
                logging.info('rts2af_fwhm.py: queueing focus run only for clear path (no filter), used filter: {0}, offset: {1}'.format(filter.name, filter.OffsetToClearPath))
            else:
                logging.info('rts2af_fwhm.py: queueing focus run only for clear path (no filter), no known filter found in headers')


if __name__ == '__main__':
    fwhm=Fwhm(sys.argv[0],parser=None,mode=None).run()

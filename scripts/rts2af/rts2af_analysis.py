#!/usr/bin/python
# (C) 2011, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rts2af_analysis.py --help
#   
#
#   rts2af_analysis.py is called by rts2af_acquire.py during an rts2
#   initiated focus run. It is not intended for interactive use.
#   The first fits file is used as reference catalogue and its focuser 
#   position must be as close as possible to the real focus.
#   
#   rts2af_analysis.py has no connection via rts2.scriptcomm.py to rts2-centrald
#   Therefore logging must be done do a separate file.
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

import sys
import re
import os
import logging
import rts2af 

class main(rts2af.AFScript):
    """extract the catalgue of an images"""
    def __init__(self, argv=None):
        self.scriptName= argv[0]
        self.argv=argv
        self.pid= os.getpid()
        

    def main(self):
        logformat= '%(asctime)s %(levelname)s %(message)s'
	logging.basicConfig(filename='/var/log/rts2-debug', level=logging.INFO, format= logformat)

        runTimeConfig= rts2af.runTimeConfig = rts2af.Configuration()
        rts2af.serviceFileOp= rts2af.ServiceFileOperations()
        configFileName=''
        if( self.argv[2]):
            configFileName= self.argv[2]  
        else:
            configFileName= runTimeConfig.configurationFileName()
            logging.info('rts2af_analysis.py: logger no config file specified, taking default ' + configFileName)

        runTimeConfig.readConfiguration(configFileName)
# read the SExtractor parameters
        paramsSexctractor= rts2af.SExtractorParams()
        paramsSexctractor.readSExtractorParams()

        if( paramsSexctractor==None):
            print 'FOCUS: -1'
            sys.stdout.flush()
            
            logging.error( 'rts2af_analysis.py: no paramsSexctractor found, exiting')
            sys.exit(1)

# create the reference catalogue
        referenceFitsFileName = sys.stdin.readline().strip()

        logging.info('rts2af_analysis.py: pid: {0}, starting, reference file: {1}'.format(self.pid, referenceFitsFileName))

        hdur= rts2af.FitsHDU(referenceFitsFileName)

        HDUs= None
        catr= None
        cats= None

        if(hdur.headerProperties()):
            HDUs= rts2af.FitsHDUs(hdur)
            catr= rts2af.ReferenceCatalogue(hdur,paramsSexctractor)
            catr.runSExtractor()
            if(catr.createCatalogue()):

                if(catr.cleanUpReference()==0):
                    logging.error('rts2af_analysis.py: exitinging due to no objects found')
                    print 'FOCUS: -1'
                    sys.stdout.flush()
                    sys.exit(1)

                catr.writeCatalogue()
                cats= rts2af.Catalogues(catr)
            else:
                print 'FOCUS: -1'
                sys.stdout.flush()

                logging.error('rts2af_analysis.py: exiting due to invalid reference catalogue or file not found: {0}'.format(referenceFitsFileName))
                sys.exit(1)

        else:
            print 'FOCUS: -1'
            sys.stdout.flush()

            logging.error('rts2af_analysis.py: exiting due to invalid hdur.headerProperties or file not found: {0}'.format(referenceFitsFileName))
            sys.exit(1)


        if( catr.numberReferenceObjects() < runTimeConfig.value('MINIMUM_OBJECTS')):
            print 'FOCUS: -1'
            sys.stdout.flush()

            logging.error('rts2af_analysis.py: exiting due to too few sxObjects found: {0} of {1}'.format(catr.numberReferenceObjects(), runTimeConfig.value('MINIMUM_OBJECTS')))
            sys.exit(1)
        else:
            logging.info('rts2af_analysis.py: reference catalogue created with: {0} objects'.format(catr.numberReferenceObjects()))
        # is needed!
        print 'info: reference catalogue created'
        sys.stdout.flush()

# read the files sys.stdin.readline() normally rts2af_analysis.py is fed by rts2af_acquire.py
        while(True):

            fits=None
            try:
                fits= sys.stdin.readline().strip()
            except:
                logging.info('rts2af_analysis.py: got EOF, breaking')
                break

            if( fits==None):
                logging.info('rts2af_analysis.py: got None, breaking')
                break
            
            quit_match= re.search( r'^Q', fits)
            if( not quit_match==None): 
                logging.info('rts2af_analysis.py: got Q, breaking')
                break

            if(len(fits) < 10):
                logging.info('rts2af_analysis.py: got short, breaking')
                break

            hdu= rts2af.FitsHDU( fits, hdur)
            if(hdu.headerProperties()):
                if(rts2af.verbose):
                    logging.info('rts2af_analysis.py: append '+ hdu.fitsFileName)

                HDUs.fitsHDUsList.append(hdu)

                cat= rts2af.Catalogue(hdu,paramsSexctractor, catr)
                cat.runSExtractor()
                cat.createCatalogue()
                cat.cleanUp()

                # append the catalogue only if there are more than runTimeConfig.value('MATCHED_RATIO') sxObjects 
                if( cat.matching()):
                    cats.CataloguesList.append(cat)
                else:
                    logging.error('rts2af_analysis.py: discarded catalogue at FOC_POS: {0}, file: {1}'.format(hdu.variableHeaderElements['FOC_POS'], hdu.fitsFileName))
            else:
                logging.error('rts2af_analysis.py: could not analyze file: {0}'.format(fits))

        fitResult= cats.fitTheValues()
        if not (fitResult==None):
            print 'FOCUS: {0}, FWHM: {1}, TEMPERATURE: {2}, OBJECTS: {3} DATAPOINTS: {4}'.format(fitResult.fwhmMinimumFocPos, fitResult.fwhmMinimum, fitResult.temperature, fitResult.objects, fitResult.nrDatapoints)
            sys.stdout.flush()
            logging.info('rts2af_analysis.py: fit result {0}, reference file: {1}'.format(fitResult.fwhmMinimumFocPos, referenceFitsFileName))
            # input format for rts2af_model_analyze.py
            # uncomment that if you need it
            logging.info('rts2af_analysis.py: {0} {1} {2} {3} {4} {5} {6} {7}\n'.format(fitResult.temperature, fitResult.objects, fitResult.fwhmMinimumFocPos, fitResult.fwhmMinimum, fitResult.dateEpoch, fitResult.fwhmWithinBounds, fitResult.referenceFileName, fitResult.nrDatapoints)) # ToDo, fitResult.constants))

        else:
            print 'FOCUS: fit did not converge'
            logging.error('rts2af_analysis.py: fit did not converge, reference file: {0}'.format(referenceFitsFileName))
                

        logging.info('rts2af_analysis.py: pid: {0}, ending, reference file: {1}'.format(self.pid, referenceFitsFileName))

if __name__ == '__main__':
    main(sys.argv).main()

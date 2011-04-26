#!/usr/bin/python
# (C) 2011, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rts2af_analysis.py --help
#   
#   not yet see man 1 rts2af_analysis.py
#
#   rts2af_analysis.py is mainly called by rts2af_acquire.py during an rts2
#   initiated focus run.
#   The first fits file is used as reference catalogue and its focuser 
#   position must be as close as possible to the real focus.
#   
#   rts2af_analysis.py has no connection via rts2comm.py to rts2-centrald
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
import logging
import rts2af 

class main(rts2af.AFScript):
    """extract the catalgue of an images"""
    def __init__(self, scriptName='main'):
        self.scriptName= scriptName
        self.test = False

    def main(self):
        runTimeConfig= rts2af.runTimeConfig = rts2af.Configuration()
        args      = self.arguments()
        rts2af.serviceFileOp= rts2af.ServiceFileOperations()

        configFileName=''
        if( args.fileName):
            configFileName= args.fileName[0]  
        else:
            configFileName= runTimeConfig.configurationFileName()
            logging.error('rts2af_analysis.py: logger no config file specified, taking default' + configFileName)

        runTimeConfig.readConfiguration(configFileName)

        if( args.referenceFitsFileName):
            referenceFitsFileName = args.referenceFitsFileName[0]
            if( not rts2af.serviceFileOp.defineRunTimePath(referenceFitsFileName)):
                logging.error('rts2af_analysis.py: reference file '+ referenceFitsFileName + ' not found in base directory ' + runTimeConfig.value('BASE_DIRECTORY' + ', exiting'))
                sys.exit(1)

# read the SExtractor parameters
        paramsSexctractor= rts2af.SExtractorParams()
        paramsSexctractor.readSExtractorParams()

        if( paramsSexctractor==None):
            print "no paramsSexctractor found, exiting"
            sys.exit(1)

# create the reference catalogue
        referenceFitsFileName = sys.stdin.readline()

        if( self.test== True):
            logging.error("rts2af_analysis.py: got reference file {0}".format(referenceFitsFileName))
        else:
            hdur= rts2af.FitsHDU(referenceFitsFileName)

            HDUs= None
            catr= None
            cats= None

            if(hdur.headerProperties()):
                HDUs= rts2af.FitsHDUs(hdur)
                catr= rts2af.ReferenceCatalogue(hdur,paramsSexctractor)
                catr.runSExtractor()
                if(catr.createCatalogue()):
                    catr.cleanUpReference()
                    catr.writeCatalogue()
                    cats= rts2af.Catalogues(catr)
                else:
                    print 'FOCUS: -1'
                    logging.error('rts2af_analysis.py: exiting due to invalid reference catalogue'.format(referenceFitsFileName))
                    sys.exit(1)

            else:
                print 'FOCUS: -1'
                print 'rts2af_analysis.py: exiting due to no hdur.headerProperties in file'.format(referenceFitsFileName)
                sys.exit(1)

# read the files sys.stdin.readline() normally rts2af_analysis.py is fed by rts2af_acquire.py
        while(True):

            fits=None
            try:
                fits= sys.stdin.readline()
            except:
                logging.info("rts2af_analysis.py: got EOF, breaking")
                break
            if( fits==None):
                logging.info("rts2af_analysis.py: got None, breaking")
                break
            
            quit_match= re.search( r'^Q', fits)
            if( not quit_match==None): 
                logging.info("rts2af_analysis.py: got Q, breaking")
                break

            if(len(fits) < 10):
                logging.info("rts2af_analysis.py: got short, breaking")
                break

            if( self.test== True):
                logging.error("rts2af_analysis.py: got file >>{0}<<".format(fits))
            else:
                hdu= rts2af.FitsHDU( fits, hdur)
                if(hdu.headerProperties()):
                    if(rts2af.verbose):
                        logging.error("rts2af_analysis.py: append "+ hdu.fitsFileName)
                    HDUs.fitsHDUsList.append(hdu)

                    cat= rts2af.Catalogue(hdu,paramsSexctractor, catr)
                    cat.runSExtractor()
                    cat.createCatalogue()
                    cat.cleanUp()

                    # append the catalogue only if there are more than runTimeConfig.value('MATCHED_RATIO') sxObjects 
                    if( cat.matching()):
                        cats.CataloguesList.append(cat)
                    else:
                        logging.error("rts2af_analysis.py: discarded catalogue at FOC_POS=%d" % hdu.variableHeaderElements['FOC_POS'] + " file "+ hdu.fitsFileName)
                else:
                    logging.error("rts2af_analysis.py: could not analyze >>{0}<<".format(fits))

        # needs CERN's root installed and rts2-fit-focus from rts2 svn repository
        if( self.test== True):
            logging.error("rts2af_analysis.py: would fit now {0}".format(referenceFitsFileName))
        else:
            cats.fitTheValues(configFileName) # ToDo: not nice

if __name__ == '__main__':
    main(sys.argv[0]).main()



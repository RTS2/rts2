#!/usr/bin/python
# (C) 2010, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rtsaf_offline.py --help
#   
#   not yet see man 1 rts2_catalogue.py
#   not yet see rts2_autofocus_unittest.py for unit tests
#
#   Basic usage:
#   Create a set of focusing images and define the reference file. The
#   reference file has apparently the smallest FWHM.
#   In your configuration file define the base directory parameter base_directory
#   under which the reference file is found.
#   This can be e.g. /scratch/focusing if the reference file is found at
#   /scratch/focusing/2011-01-16-T17:37:55/X/
#
#   Then run, e.g.
#   rts2af_analysis.py  --config ./rts2-autofocus-offline.cfg 
#
#   The output is mostly written to /var/log/rts2-autofocus
#
#   In the /tmp directory you a lot of output for inspection. Or more
#   conveniently, use executed the file with the ending sh, wich looks like, e.g.
#   /tmp/rts2af-ds9-autofocus-X-2011-01-22T12:12:26.729174.reg.sh
#   and check the results with DS9.
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

        test = False

        configFileName=''
        if( args.fileName):
            configFileName= args.fileName[0]  
        else:
            configFileName= runTimeConfig.configurationFileName()
            logger.error('main: logger no config file specified, taking default' + configFileName)

        runTimeConfig.readConfiguration(configFileName)

        if( args.referenceFitsFileName):
            referenceFitsFileName = args.referenceFitsFileName[0]
            if( not rts2af.serviceFileOp.defineRunTimePath(referenceFitsFileName)):
                logger.error('main: reference file '+ referenceFitsFileName + ' not found in base directory ' + runTimeConfig.value('BASE_DIRECTORY' + 'exiting'))
                sys.exit(1)

# read the SExtractor parameters
        paramsSexctractor= rts2af.SExtractorParams()
        paramsSexctractor.readSExtractorParams()

        if( paramsSexctractor==None):
            print "no paramsSexctractor found, exiting"
            sys.exit(1)

# create the reference catalogue

        referenceFitsFileName = sys.stdin.readline()

        if( test== True):
            logger.error("main: got reference file {0}".format(referenceFitsFileName))
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
                    logger.error('main: exiting due to invalid reference catalogue'.format(referenceFitsFileName))
                    sys.exit(1)

            else:
                print 'FOCUS: -1'
                print 'main: exiting due to no hdur.headerProperties in file'.format(referenceFitsFileName)
                sys.exit(1)
            
# read the files sys.stdin.readline() 
        while(True):

            fits=None
            try:
                fits= sys.stdin.readline()
            except:
# doesn't work
                logger.error("main: got EOF, breaking")
                break
# doesn't work
            if( fits==None):
                logger.error("main: got None, breaking")
                break
            if(len(fits) < 2):
# works but bad
                logger.error("main: got short, breaking")
                break
            if( test== True):
                logger.error("main: got file >>{0}<<".format(fits))
            else:
                hdu= rts2af.FitsHDU( fits, hdur)
                if(hdu.headerProperties()):
                    if(rts2af.verbose):
                        logger.error("main: append "+ hdu.fitsFileName)
                    HDUs.fitsHDUsList.append(hdu)

                    cat= rts2af.Catalogue(hdu,paramsSexctractor, catr)
                    cat.runSExtractor()
                    cat.createCatalogue()
                    cat.cleanUp()

                    # append the catalogue only if there are more than runTimeConfig.value('MATCHED_RATIO') sxObjects 
                    if( cat.matching()):
                        cats.CataloguesList.append(cat)
                    else:
                        logger.error("main: discarded catalogue at FOC_POS=%d" % hdu.headerElements['FOC_POS'] + " file "+ hdu.fitsFileName)
                else:
                    logger.error("main: could not analyze >>{0}<<".format(fits))


        # needs CERN's root installed and rts2-fit-focus from rts2 svn repository
        if( test== True):
            logger.error("main: would fit now {0}".format(referenceFitsFileName))
        else:
            cats.fitTheValues()


if __name__ == '__main__':
    main(sys.argv[0]).main()



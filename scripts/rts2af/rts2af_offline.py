#!/usr/bin/python
# (C) 2010, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rtsaf_offline.py --help
#   
#   not yet see man 1 rts2_/offline.py
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
#   rts2af_offline.py  --config ./rts2af-offline.cfg --reference 20091106180858-517-RA.fits
#
#
#   In the /tmp directory is a lot of output for inspection. Or more
#   conveniently, execute the file with the ending .sh, wich looks like, e.g.
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
import logging
import time
import rts2af 
#import rts2af_meteodb

class main(rts2af.AFScript):
    """extract the catalgue of an images"""
    def __init__(self, scriptName='main'):
        self.scriptName= scriptName

    def main(self):
        logformat= '%(asctime)s %(levelname)s %(message)s'
	logging.basicConfig(filename='/tmp/rts2af-model.log', level=logging.INFO, format= logformat)

        runTimeConfig= rts2af.runTimeConfig = rts2af.Configuration()
        rts2af.serviceFileOp= rts2af.ServiceFileOperations()
        args         = self.arguments()

        configFileName='/etc/rts2/rts2af/rts2af-offline.cfg'
        if( args.fileName):
            configFileName= args.fileName[0]  
        else:
            configFileName= runTimeConfig.configurationFileName()
            logging.info('logger no config file specified, taking default' + configFileName)

        runTimeConfig.readConfiguration(configFileName)

        if( args.referenceFitsFileName):
            referenceFitsFileName = args.referenceFitsFileName[0]
            if( not rts2af.serviceFileOp.defineRunTimePath(referenceFitsFileName)):
                logging.error('rts2af_offline.py reference file '+ referenceFitsFileName + ' not found in base directory ' + runTimeConfig.value('BASE_DIRECTORY'))
                sys.exit(1)
# get the list of fits files
        FitsList=[]
        FitsList=rts2af.serviceFileOp.fitsFilesInRunTimePath()
#        print '{0}'.format(FitsList)

# read the SExtractor parameters
        paramsSexctractor= rts2af.SExtractorParams()
        paramsSexctractor.readSExtractorParams()

        if( paramsSexctractor==None):
            print "exiting"
            sys.exit(1)

# create the reference catalogue
        hdur= rts2af.FitsHDU(referenceFitsFileName)

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
                sys.exit(1)
        else:
            sys.exit(1)
            print 'FOCUS: -1'
            
# read the files 
        for fits in FitsList:
            hdu= rts2af.FitsHDU( fits, hdur)
            if(hdu.headerProperties()):
                if(rts2af.verbose):
                    print "append "+ hdu.fitsFileName
                HDUs.fitsHDUsList.append(hdu)

        if( not HDUs.validate()):
            logging.error("rts2af_offline.py: HDUs are not valid, exiting")
            sys.exit(1)

# loop over hdus, create the catalogues
        for hdu  in HDUs.fitsHDUsList:
            if( rts2af.verbose):
                print '=======' + hdu.variableHeaderElements['FILTER'] + '=== valid=' + repr(hdu.isValid) + ' number of files at FOC_POS=%d' % hdu.variableHeaderElements['FOC_POS'] + ': %d' % HDUs.fitsHDUsList.count(hdu) + " " + hdu.fitsFileName
            
            cat= rts2af.Catalogue(hdu,paramsSexctractor, catr)
            cat.runSExtractor()
            cat.createCatalogue()
            cat.cleanUp()
#            cat.ds9DisplayCatalogue()
            # append the catalogue only if there are more than runTimeConfig.value('MATCHED_RATIO') sxObjects 
            if( cat.matching()):
                #print "Added catalogue at FOC_POS=%d" % hdu.variableHeaderElements['FOC_POS'] + " file "+ hdu.fitsFileName
                cats.CataloguesList.append(cat)
            else:
                logging.error("rts2af_offline.py: discarded catalogue at FOC_POS=%d" % hdu.variableHeaderElements['FOC_POS'] + " file "+ hdu.fitsFileName)

        if(not cats.validate()):
            logging.error("rts2af_offline.py: catalogues are invalid, exiting")
            sys.exit(1)

        # needs CERN's root installed and rts2-fit-focus from rts2 svn repository
        fitResult= cats.fitTheValues()
        if(not fitResult==None):
            if( not fitResult.error):
                print 'FOCUS: {0}, FWHM: {1}, TEMPERATURE: {2}, OBJECTS: {3} DATAPOINTS: {4} {5}'.format(fitResult.minimumFocPos, fitResult.minimumFwhm, fitResult.temperature, fitResult.objects, fitResult.nrDatapoints, fitResult.referenceFileName)


#        if(runTimeConfig.value('WRITE_SUMMARY_FILE')):
#            fitResultSummaryFileName= '/tmp/result-model-analyze.log'
#            if(not fitResult==None):
#                if( not fitResult.error):
                    #dc= rts2af_meteodb.ReadMeteoDB()
                    #(temperatureConsole, temperatureIss)= dc.queryMeteoDb(fitResult.dateEpoch)
#                    temperatureConsole= 10.

#                    with open( fitResultSummaryFileName, 'a') as frs:
#                        frs.write('{0} {1} {2} {3} {4} {5} {6} {7} {8} {9} {10}\n'.format(fitResult.chi2, temperatureConsole, fitResult.temperature, fitResult.objects, fitResult.minimumFocPos, fitResult.minimumFwhm, fitResult.dateEpoch, fitResult.withinBounds, fitResult.referenceFileName, fitResult.nrDatapoints, fitResult.constants))
#                    frs.close()


        # executed the latest /tmp/*.sh file ro see the results with DS9 
        #cats.ds9WriteRegionFiles()

if __name__ == '__main__':
    main(sys.argv[0]).main()



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
#   rts2af_offline.py  --config ./rts2-autofocus-offline.cfg --reference 20091106180858-517-RA.fits
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

        configFileName=''
        if( args.fileName):
            configFileName= args.fileName[0]  
        else:
            configFileName= runTimeConfig.configurationFileName()
            cmd= 'logger no config file specified, taking default' + configFileName
            #print cmd 
            ##os.system( cmd)

        runTimeConfig.readConfiguration(configFileName)


        if( args.referenceFitsFileName):
            referenceFitsFileName = args.referenceFitsFileName[0]
            if( not rts2af.serviceFileOp.defineRunTimePath(referenceFitsFileName)):
                logger.error('main: reference file '+ referenceFitsFileName + ' not found in base directory ' + runTimeConfig.value('BASE_DIRECTORY'))
                sys.exit(1)
# get the list of fits files
        FitsList=[]
        FitsList=rts2af.serviceFileOp.fitsFilesInRunTimePath()

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
            logger.error("main: HDUs are not valid, exiting")
            sys.exit(1)

# loop over hdus, create the catalogues
        for hdu  in HDUs.fitsHDUsList:
            if( rts2af.verbose):
                print '=======' + hdu.headerElements['FILTER'] + '=== valid=' + repr(hdu.isValid) + ' number of files at FOC_POS=%d' % hdu.headerElements['FOC_POS'] + ': %d' % HDUs.fitsHDUsList.count(hdu) + " " + hdu.fitsFileName
            
            cat= rts2af.Catalogue(hdu,paramsSexctractor, catr)
            cat.runSExtractor()
            cat.createCatalogue()
            cat.cleanUp()

            # append the catalogue only if there are more than runTimeConfig.value('MATCHED_RATIO') sxObjects 
            if( cat.matching()):
                #print "Added catalogue at FOC_POS=%d" % hdu.headerElements['FOC_POS'] + " file "+ hdu.fitsFileName
                cats.CataloguesList.append(cat)
            else:
                logger.error("main: discarded catalogue at FOC_POS=%d" % hdu.headerElements['FOC_POS'] + " file "+ hdu.fitsFileName)

        if(not cats.validate()):
            logger.error("main: catalogues are invalid, exiting")
            sys.exit(1)

        # needs CERN's root installed and rts2-fit-focus from rts2 svn repository
        cats.fitTheValues()
        # executed the latest /tmp/*.sh file ro see the results with DS9 
        cats.ds9WriteRegionFiles()


        # Various examples:
        #cats.average()
        #for focPos in sorted(fwhm):
        #    print "average %d %f %f" % (focPos, self.averageFwhm[focPos], self.averageFlux[focPos])

        # Print on terminal
        #cats.printSelectedSXobjects()
        # Live display can take a long time (it is not very useful):
        #cats.ds9DisplayCatalogues()
        #
        #
        # Check properties of the various data sets
        #
        #for cat in sorted(cats.CataloguesList, key=lambda cat: cat.fitsHDU.headerElements['FOC_POS']):
        #    if(rts2af.verbose):
        #        print "fits file: "+ cat.fitsHDU.fitsFileName + ", %d " % cat.fitsHDU.headerElements['FOC_POS'] 
        #    cat.average('FWHM_IMAGE')
        #    cat.averageFWHM("selected")
        #    cat.averageFWHM("matched")
        #    cat.averageFWHM()


if __name__ == '__main__':
    main(sys.argv[0]).main()



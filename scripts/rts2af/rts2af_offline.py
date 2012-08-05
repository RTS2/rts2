#!/usr/bin/python
# (C) 2010-2012, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rtsaf_offline.py --help
#   
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
import rts2af 

class Offline(rts2af.AFScript):
    """Do offline analysis of a set of focus images"""

    def run(self):
# get the list of fits files
        FitsList=[]
        FitsList=self.env.fitsFilesInRunTimePath()
# read the SExtractor parameters
        paramsSexctractor= rts2af.SExtractorParams(paramsFileName=self.rtc.value('SEXREFERENCE_PARAM'))
        paramsSexctractor.readSExtractorParams()

        if paramsSexctractor==None:
            print 'FOCUS: -1'
            self.logger('run: exiting')
            sys.exit(1)

# create the reference catalogue
        hdur= rts2af.FitsHDU(env=self.env, fitsFileName=self.referenceFitsName)

        if hdur.headerProperties():
            HDUs= rts2af.FitsHDUs(minimumFocuserPositions=self.rtc.value('MINIMUM_FOCUSER_POSITIONS'), referenceHDU=hdur)
            catr= rts2af.ReferenceCatalogue(env=self.env, fitsHDU=hdur,SExtractorParams=paramsSexctractor)
            catr.runSExtractor()
            
            if catr.createCatalogue():
                catr.cleanUpReference()
                catr.writeCatalogue()
                cats= rts2af.Catalogues(env=self.env, referenceCatalogue=catr)
            else:
                print 'FOCUS: -1'
                sys.exit(1)
        else:
            sys.exit(1)
            print 'FOCUS: -1'
    
# read the files 
        for fits in FitsList:
            hdu= rts2af.FitsHDU(env= self.env, fitsFileName=fits, referenceFitsHDU=hdur)
            if hdu.headerProperties():
                self.logger.debug("append "+ hdu.fitsFileName)
                    
                HDUs.fitsHDUsList.append(hdu)
            else:
                self.logger.warning( "NOT append "+ str(hdu.fitsFileName))

        if( not HDUs.validate()):
            self.logger.error("rts2af_offline.py: HDUs are not valid, exiting")
            sys.exit(1)

# loop over hdus, create the catalogues
        for hdu  in HDUs.fitsHDUsList:

            cat= rts2af.Catalogue(env=self.env, fitsHDU=hdu,SExtractorParams=paramsSexctractor, referenceCatalogue=catr)
            cat.runSExtractor()
            cat.createCatalogue()
            cat.cleanUp()
#            cat.ds9DisplayCatalogue()
# append the catalogue only if there are more than runTimeConfig.value('MATCHED_RATIO') sxObjects 
            cats.CataloguesAllFocPosList.append(cat)
            if cat.matching():
                cats.CataloguesList.append(cat)
            else:
                self.logger.error("rts2af_offline.py: discarded catalogue at FOC_POS=%d" % hdu.variableHeaderElements['FOC_POS'] + " file "+ hdu.fitsFileName)

        if not cats.validate():
            self.logger.error("rts2af_offline.py: catalogues are invalid, exiting")
            sys.exit(1)

        extreme=False
        fitResult= cats.fitTheValues()
        if fitResult:
            if not fitResult.error:
                # rts2af_offline is often called as a subprocess
                print 'FOCUS: {0}, FWHM: {1}, TEMPERATURE: {2}, OBJECTS: {3} DATAPOINTS: {4} {5}'.format(fitResult.fwhmMinimumFocPos, fitResult.fwhmMinimum, fitResult.temperature, fitResult.objects, fitResult.nrDatapoints, fitResult.referenceFileName)
                self.logger.info('FOCUS: {0}, FWHM: {1}, TEMPERATURE: {2}, OBJECTS: {3} DATAPOINTS: {4} {5}'.format(fitResult.fwhmMinimumFocPos, fitResult.fwhmMinimum, fitResult.temperature, fitResult.objects, fitResult.nrDatapoints, fitResult.referenceFileName))
            else:
                self.logger.error('rts2af_offline.py: fit result is erroneous')
                extreme=True
        else:
            extreme=True

        if extreme:
            self.logger.warning("rts2af_offline.py: no fit result, using either weighted mean or minFocPos or maxFocPos")
            (focpos, extreme)=cats.findExtrema()
            if focpos:
                 print 'FOCUS: {0}, FWHM: {1}, TEMPERATURE: {2}, OBJECTS: {3} DATAPOINTS: {4} {5}'.format(focpos, extreme, fitResult.temperature, -1., -1., fitResult.referenceFileName)
                 self.logger.info('FOCUS: {0}, FWHM: {1}, TEMPERATURE: {2}, OBJECTS: {3} DATAPOINTS: {4} {5}'.format(focpos, extreme, fitResult.temperature, -1., -1., fitResult.referenceFileName))
            else:
                 print 'FOCUS: {0}'.format(-1)

        # executed the latest /tmp/*.sh file ro see the results with DS9 
        #cats.ds9WriteRegionFiles()

if __name__ == '__main__':
    Offline(sys.argv[0],parser=None,mode=None).run()


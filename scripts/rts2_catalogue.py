#!/usr/bin/python
# (C) 2010, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rts_catalogue.py --help
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
        rts2af.sfo= rts2af.ServiceFileOperations()

        configFileName=''
        if( args.fileName):
            configFileName= args.fileName[0]  
        else:
            configFileName= runTimeConfig.configurationFileName()
            cmd= 'logger no config file specified, taking default' + configFileName
            #print cmd 
            os.system( cmd)

        runTimeConfig.readConfiguration(configFileName)

        print "main " + runTimeConfig.value('SEXPRG')
        print "main " + repr(runTimeConfig.value('TAKE_DATA'))


        ffs= rts2af.FitsFiles()
        hdu= rts2af.FitsFile('20100626103442-779-RA.fits', True)
        ffs.append(hdu)
        hdu1= rts2af.FitsFile('20100626103210-295-RA.fits', False)
        ffs.append(hdu1)
        ffs.append(hdu)
        ffs.append(hdu)
        ffs.append(hdu)


        ffs.validate()

        for fits in ffs.fitsFiles():
            print '=======' + fits.filter + '===' + repr(fits.isValid) + '= %d' % ffs.fitsFiles().count(hdu)

# loop over hdus
        cats= rts2af.Catalogues()
        cat1= rts2af.Catalogue(hdu1)
        cats.append(cat1)

        cat1.extractToCatalogue()
        #cat1.createCatalogue()
        #cat1.average('FWHM_IMAGE')


        cat= rts2af.Catalogue(hdu)
        cats.append(cat)



        cats.validate()

if __name__ == '__main__':
    main(sys.argv[0]).main()




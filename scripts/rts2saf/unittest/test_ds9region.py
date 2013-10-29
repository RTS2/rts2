# (C) 2013, Markus Wildi, markus.wildi@bluewin.ch
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

import unittest
from ds9 import *
from rts2saf.config import Configuration 
from rts2saf.data import DataSex
from rts2saf.sextract import Sextract
from rts2saf.ds9region import Ds9Region


import logging
logging.basicConfig(filename='/tmp/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()



# sequence matters
def suite():
    suite = unittest.TestSuite()
    suite.addTest(TestDs9Region('test_displayWithRegion'))

    return suite

#@unittest.skip('class not yet implemented')
class TestDs9Region(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        self.rt = Configuration(logger=logger)
        self.rt.readConfiguration(fileName='../configs/no-filter-wheel/rts2saf.cfg')
        self.dSx=Sextract(debug=False, rt=self.rt, logger=logger).sextract(fitsFn='../samples/20071205025911-725-RA.fits')
        # to see DS9 use
        #  self.dds9=ds9()
        self.dds9=None

    #@unittest.skip('feature not yet implemented')
    def test_displayWithRegion(self):
        
        ds9r=Ds9Region(dataSex=self.dSx, display=self.dds9, logger=logger)

        self.assertTrue( ds9r, 'return value: '.format(type(DataSex)))

if __name__ == '__main__':
    
    unittest.TextTestRunner(verbosity=0).run(suite())

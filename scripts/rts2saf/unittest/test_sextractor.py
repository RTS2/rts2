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
import os

from rts2saf.config import Configuration 
from rts2saf.data import DataSxtr
from rts2saf.sextract import Sextract

import logging
if not os.path.isdir('/tmp/rts2saf_log'):
    os.mkdir('/tmp/rts2saf_log')


logging.basicConfig(filename='/tmp/rts2saf_log/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()

# sequence matters
def suite():
    suite = unittest.TestSuite()
    suite.addTest(TestSextract('test_sextract'))
    suite.addTest(TestSextract('test_sextractFlux'))

    return suite


#@unittest.skip('class not yet implemented')
class TestSextract(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        pass

    #@unittest.skip('feature not yet implemented')
    def test_sextract(self):
        logger.info('== {} =='.format(self._testMethodName))
        rt = Configuration(logger=logger)
        rt.readConfiguration(fileName='./rts2saf-no-filter-wheel.cfg')
        sx = Sextract(debug=False, rt=rt, logger=logger)
        fitsFn='../samples/20071205025911-725-RA.fits'
        dSx=sx.sextract(fitsFn=fitsFn)
        self.assertIs(type(dSx), DataSxtr, 'return object of type: '.format(type(dSx)))
        self.assertEqual(fitsFn, dSx.fitsFn, 'install sample FITS from wget http://azug.minpet.unibas.ch/~wildi/rts2saf-test-focus-2013-09-14.tgz')

    def test_sextractFlux(self):
        logger.info('== {} =='.format(self._testMethodName))
        rt = Configuration(logger=logger)
        rt.readConfiguration(fileName='./rts2saf-flux.cfg')
        sx = Sextract(debug=False, rt=rt, logger=logger)
        fitsFn='../samples/20071205025911-725-RA.fits'
        sx.appendFluxFields()
        dSx=sx.sextract(fitsFn=fitsFn)
        self.assertEqual(dSx.fields[12], 'FLUXERR_APER', 'return value: {}'.format(dSx.fields[12]))


if __name__ == '__main__':
    
    unittest.TextTestRunner(verbosity=0).run(suite())

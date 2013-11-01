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
import glob
import sys # ugly
from rts2saf.config import Configuration 
from rts2saf.analyze import SimpleAnalysis, CatalogAnalysis 
from rts2saf.data import DataSex
from rts2saf.sextract import Sextract
from rts2saf.environ import Environment

from rts2saf.devices import CCD, Focuser, FilterWheel, Filter

import logging
logging.basicConfig(filename='/tmp/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()

# sequence matters
def suite_simple():
    suite = unittest.TestSuite()
    suite.addTest(TestSimpleAnalysis('test_readConfiguration'))
    suite.addTest(TestSimpleAnalysis('test_fitsInBasepath'))
    suite.addTest(TestSimpleAnalysis('test_analyze'))

    return suite

def suite_catalog():
    suite = unittest.TestSuite()
    suite.addTest(TestCatalogAnalysis('test_readConfiguration'))
    suite.addTest(TestCatalogAnalysis('test_fitsInBasepath'))
    suite.addTest(TestCatalogAnalysis('test_selectAndAnalyze'))

    return suite

#@unittest.skip('class not yet implemented')
class TestSimpleAnalysis(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        self.rt = Configuration(logger=logger)
        self.fileName='./rts2saf-no-filter-wheel.cfg'
        self.success=self.rt.readConfiguration(fileName=self.fileName)
        self.ev=Environment(debug=False, rt=self.rt,logger=logger)

    #@unittest.skip('feature not yet implemented')
    def test_readConfiguration(self):
        self.assertTrue(self.success, 'config file: {} faulty or not found, return value: {}'.format(self.fileName, self.success))

    def test_fitsInBasepath(self):
        fitsFns=glob.glob('{0}/{1}'.format('../samples', self.rt.cfg['FILE_GLOB']))
        self.assertEqual(len(fitsFns), 14, 'return value: {}'.format(len(fitsFns)))

    def test_analyze(self):
        fitsFns=glob.glob('{0}/{1}'.format('../samples', self.rt.cfg['FILE_GLOB']))
        dataSex=list()
        for k, fitsFn in enumerate(fitsFns):
        
            logger.info('analyze: processing fits file: {0}'.format(fitsFn))
            sxtr= Sextract(debug=False, rt=self.rt, logger=logger)
            dSx=sxtr.sextract(fitsFn=fitsFn)

            if dSx !=None:
                dataSex.append(dSx)

        self.assertEqual(len(dataSex), 14, 'return value: {}'.format(len(dataSex)))
        an=SimpleAnalysis(debug=False, dataSex=dataSex, Ds9Display=False, FitDisplay=False, focRes=float(self.rt.cfg['FOCUSER_RESOLUTION']), ev=self.ev, logger=logger)
        resultFitFwhm=an.analyze()
        self.assertAlmostEqual(resultFitFwhm.minFitFwhm, 2.24000979001, places=3, msg='return value: {}'.format(resultFitFwhm.minFitFwhm))


#@unittest.skip('class not yet implemented')
class TestCatalogAnalysis(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        self.rt = Configuration(logger=logger)
        self.fileName='./rts2saf-no-filter-wheel.cfg'
        self.success=self.rt.readConfiguration(fileName=self.fileName)
        self.ev=Environment(debug=False, rt=self.rt,logger=logger)

    def test_readConfiguration(self):
        self.assertTrue(self.success, 'config file: {} faulty or not found, return value: {}'.format(self.fileName, self.success))

    def test_fitsInBasepath(self):
        fitsFns=glob.glob('{0}/{1}'.format('../samples', self.rt.cfg['FILE_GLOB']))
        self.assertEqual(len(fitsFns), 14, 'return value: {}'.format(len(fitsFns)))

    def test_selectAndAnalyze(self):
        fitsFns=glob.glob('{0}/{1}'.format('../samples', self.rt.cfg['FILE_GLOB']))
        dataSex=list()
        for k, fitsFn in enumerate(fitsFns):
        
            logger.info('analyze: processing fits file: {0}'.format(fitsFn))
            sxtr= Sextract(debug=False, rt=self.rt, logger=logger)
            dSx=sxtr.sextract(fitsFn=fitsFn)

            if dSx !=None:
                dataSex.append(dSx)

        self.assertEqual(len(dataSex), 14, 'return value: {}'.format(len(dataSex)))
        an=CatalogAnalysis(debug=False, dataSex=dataSex, Ds9Display=False, FitDisplay=False, focRes=float(self.rt.cfg['FOCUSER_RESOLUTION']), moduleName='rts2saf.criteria_radius', ev=self.ev, rt=self.rt, logger=logger)
        accRFt, rejRFt, allrFt=an.selectAndAnalyze()
        self.assertEqual('{0:5.4f}'.format(allrFt.minFitFwhm), '{0:5.4f}'.format(2.24000979001), 'return value: {}'.format(allrFt.minFitFwhm))
        self.assertAlmostEqual(accRFt.minFitFwhm, 2.24000979001, delta=0.05, msg='return value: {}'.format(allrFt.minFitFwhm))

if __name__ == '__main__':
    
    suiteSimple=suite_simple()
    suiteCatalog= suite_catalog()
    alltests = unittest.TestSuite([suiteSimple, suiteCatalog])
    unittest.TextTestRunner(verbosity=0).run(alltests)

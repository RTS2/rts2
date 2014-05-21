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
import sys 
import os

from rts2saf.config import Configuration 
from rts2saf.analyze import SimpleAnalysis, CatalogAnalysis 
from rts2saf.data import DataSxtr
from rts2saf.sextract import Sextract
from rts2saf.environ import Environment

from rts2saf.devices import CCD, Focuser, FilterWheel, Filter

import logging
if not os.path.isdir('/tmp/rts2saf_log'):
    os.mkdir('/tmp/rts2saf_log')


logging.basicConfig(filename='/tmp/rts2saf_log/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
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
        self.fileName='./rts2saf-flux.cfg'
        self.success=self.rt.readConfiguration(fileName=self.fileName)
        self.ev=Environment(debug=False, rt=self.rt,logger=logger)

    #@unittest.skip('feature not yet implemented')
    def test_readConfiguration(self):
        logger.info('== {} =='.format(self._testMethodName))
        self.assertTrue(self.success, 'config file: {} faulty or not found, return value: {}'.format(self.fileName, self.success))

    def test_fitsInBasepath(self):
        logger.info('== {} =='.format(self._testMethodName))
        fitsFns=glob.glob('{0}/{1}'.format('../samples', self.rt.cfg['FILE_GLOB']))
        self.assertEqual(len(fitsFns), 14, 'return value: {}'.format(len(fitsFns)))

    def test_analyze(self):
        logger.info('== {} =='.format(self._testMethodName))
        fitsFns=glob.glob('{0}/{1}'.format('../samples', self.rt.cfg['FILE_GLOB']))
        dataSxtr=list()
        for k, fitsFn in enumerate(fitsFns):
        
            logger.info('analyze: processing fits file: {0}'.format(fitsFn))
            sxtr= Sextract(debug=False, rt=self.rt, logger=logger)
            dSx=sxtr.sextract(fitsFn=fitsFn)

            if dSx:
                dataSxtr.append(dSx)

        self.assertEqual(len(dataSxtr), 14, 'return value: {}'.format(len(dataSxtr)))

        an=SimpleAnalysis(debug=False, dataSxtr=dataSxtr, Ds9Display=False, FitDisplay=False, focRes=float(self.rt.cfg['FOCUSER_RESOLUTION']), ev=self.ev, logger=logger)
        resultFitFwhm, resultMeansFwhm, resultFitFlux, resultMeansFlux=an.analyze()
        self.assertAlmostEqual(resultFitFwhm.extrFitVal, 2.2175214358, places=2, msg='return value: {}'.format(resultFitFwhm.extrFitVal))

        an.display()

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
        logger.info('== {} =='.format(self._testMethodName))
        self.assertTrue(self.success, 'config file: {} faulty or not found, return value: {}'.format(self.fileName, self.success))

    def test_fitsInBasepath(self):
        logger.info('== {} =='.format(self._testMethodName))
        fitsFns=glob.glob('{0}/{1}'.format('../samples', self.rt.cfg['FILE_GLOB']))
        self.assertEqual(len(fitsFns), 14, 'return value: {}'.format(len(fitsFns)))

    #@unittest.skip('feature not yet implemented')
    def test_selectAndAnalyze(self):
        logger.info('== {} =='.format(self._testMethodName))
        fitsFns=glob.glob('{0}/{1}'.format('../samples', self.rt.cfg['FILE_GLOB']))
        dataSxtr=list()
        for k, fitsFn in enumerate(fitsFns):
        
            logger.info('analyze: processing fits file: {0}'.format(fitsFn))
            sxtr= Sextract(debug=False, rt=self.rt, logger=logger)
            dSx=sxtr.sextract(fitsFn=fitsFn)

            if dSx:
                dataSxtr.append(dSx)

        self.assertEqual(len(dataSxtr), 14, 'return value: {}'.format(len(dataSxtr)))
        an=CatalogAnalysis(debug=False, dataSxtr=dataSxtr, Ds9Display=False, FitDisplay=False, focRes=float(self.rt.cfg['FOCUSER_RESOLUTION']), moduleName='rts2saf.criteria_radius', ev=self.ev, rt=self.rt, logger=logger)
        accRFt, rejRFt, allrFt, accRMns, recRMns, allRMns=an.selectAndAnalyze()
        self.assertAlmostEqual(allrFt.extrFitVal, 2.2175214358, delta=0.1, msg='return value: {}'.format(allrFt.extrFitVal))
        self.assertAlmostEqual(accRFt.extrFitVal, 2.24000979001, delta=0.1, msg='return value: {}'.format(allrFt.extrFitVal))



if __name__ == '__main__':
    
    suiteSimple=suite_simple()
    suiteCatalog= suite_catalog()
    alltests = unittest.TestSuite([suiteSimple, suiteCatalog])
    unittest.TextTestRunner(verbosity=0).run(alltests)

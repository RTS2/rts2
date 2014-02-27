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
from rts2saf.devices import CCD, Focuser, FilterWheel, Filter

import logging
if not os.path.isdir('/tmp/rts2saf_log'):
    os.mkdir('/tmp/rts2saf_log')


logging.basicConfig(filename='/tmp/rts2saf_log/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()

# sequence matters
def suiteBootes2():
    suite = unittest.TestSuite()
    suite.addTest(TestConfigurationBootes2('test_readConfiguration'))
    suite.addTest(TestConfigurationBootes2('test_checkConfiguration'))
    suite.addTest(TestConfigurationBootes2('test_filterWheelInUse'))
    suite.addTest(TestConfigurationBootes2('test_writeDefaultConfiguration'))

    return suite

def suiteBootes2Autonomous():
    suite = unittest.TestSuite()
    suite.addTest(TestConfigurationBootes2Autonomous('test_readConfiguration'))
    return suite

def suiteNoFilterWheel():
    suite = unittest.TestSuite()
    suite.addTest(TestConfigurationNoFilterWheel('test_readConfiguration'))
    return suite

def suiteFlux():
    suite = unittest.TestSuite()
    suite.addTest(TestConfigurationFlux('test_readConfiguration'))
    return suite

class Args(object):
    def __init__(self):
        pass


#@unittest.skip('class not yet implemented')
class TestConfigurationBootes2(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        self.rt = Configuration(logger=logger)
        self.fileName='./rts2saf-bootes-2.cfg'
        self.success=self.rt.readConfiguration(fileName=self.fileName)
        self.args=Args()
        self.args.associate = True
        self.args.flux=True

    #@unittest.skip('feature not yet implemented')
    def test_readConfiguration(self):
        logger.info('== {} =='.format(self._testMethodName))
        self.assertTrue(self.success, 'config file: {} faulty or not found'.format(self.fileName))
        self.assertIs(type(self.rt), Configuration)

    def test_checkConfiguration(self):
        logger.info('== {} =='.format(self._testMethodName))
        self.assertTrue(self.success, 'config file: {} faulty or not found'.format(self.fileName))
        result = self.rt.checkConfiguration(args=self.args)
        self.assertTrue(result)

    def test_filterWheelInUse(self):
        logger.info('== {} =='.format(self._testMethodName))
        self.assertEqual(self.rt.cfg['inuse'][0], 'COLWFLT', 'return value: {}'.format(self.rt.cfg['inuse'][0]))

    def test_writeDefaultConfiguration(self):
        logger.info('== {} =='.format(self._testMethodName))
        cfn='./rts2saf-default.cfg'
        result=self.rt.writeDefaultConfiguration(cfn=cfn)
        self.assertEqual(cfn, result, 'return value: {}'.format(result))



#@unittest.skip('class not yet implemented')
class TestConfigurationBootes2Autonomous(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        self.rt = Configuration(logger=logger)
        self.fileName='./rts2saf-bootes-2-autonomous.cfg'
        self.success=self.rt.readConfiguration(fileName=self.fileName)

    #@unittest.skip('feature not yet implemented')
    def test_readConfiguration(self):
        logger.info('== {} =='.format(self._testMethodName))
        self.assertTrue(self.success, 'config file: {} faulty or not found'.format(self.fileName))
        self.assertIs(type(self.rt), Configuration)


#@unittest.skip('class not yet implemented')
class TestConfigurationNoFilterWheel(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        self.rt = Configuration(logger=logger)
        self.fileName='./rts2saf-no-filter-wheel.cfg'
        self.success=self.rt.readConfiguration(fileName=self.fileName)

    #@unittest.skip('feature not yet implemented')
    def test_readConfiguration(self):
        logger.info('== {} =='.format(self._testMethodName))
        self.assertTrue(self.success, 'config file: {} faulty or not found'.format(self.fileName))
        self.assertIs(type(self.rt), Configuration)


#@unittest.skip('class not yet implemented')
class TestConfigurationFlux(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        self.rt = Configuration(logger=logger)
        self.fileName='./rts2saf-flux.cfg'
        self.success=self.rt.readConfiguration(fileName=self.fileName)

    #@unittest.skip('feature not yet implemented')
    def test_readConfiguration(self):
        logger.info('== {} =='.format(self._testMethodName))        
        self.assertEqual(self.rt.cfg[ 'MINIMUM_FOCUSER_POSITIONS'], 5, 'return value: {}, config file: {} faulty or not found'.format(self.rt.cfg[ 'MINIMUM_FOCUSER_POSITIONS'], self.fileName))
        self.assertTrue(self.success, 'config file: {} faulty or not found'.format(self.fileName))
        self.assertIs(type(self.rt), Configuration)




if __name__ == '__main__':
    

    suiteB2    = suiteBootes2()
    suiteB2Aut = suiteBootes2Autonomous()
    suiteNoFtw = suiteNoFilterWheel()
    suiteFlux  = suiteFlux()
    alltests   = unittest.TestSuite([suiteB2, suiteB2Aut, suiteNoFtw, suiteFlux])

    unittest.TextTestRunner(verbosity=0).run(alltests)

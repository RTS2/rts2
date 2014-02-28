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

from rts2saf.config  import Configuration 
from rts2saf.devices import Filter, FilterWheel, Focuser, CCD

import logging
if not os.path.isdir('/tmp/rts2saf_log'):
    os.mkdir('/tmp/rts2saf_log')


logging.basicConfig(filename='/tmp/rts2saf_log/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()

# sequence matters
def suite_devices():
    suite = unittest.TestSuite()
    suite.addTest(TestDevices('test_deviceClasses'))
    return suite

def suite_create_devices():
    suite = unittest.TestSuite()
    suite.addTest(TestCreateDevices('test_createFTs'))
    suite.addTest(TestCreateDevices('test_createFOC'))
    suite.addTest(TestCreateDevices('test_createFTWs'))
    suite.addTest(TestCreateDevices('test_createCCD'))
    return suite

def suite_check_devices():
    suite = unittest.TestSuite()
    suite.addTest(TestCheckDevices('test_checkDevices'))
    return suite

from rts2saf.createdevices import CreateFilters, CreateFilterWheels, CreateFocuser, CreateCCD
#@unittest.skip('class not yet implemented')
class TestDevices(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        pass
    @unittest.skip('feature not yet implemented')
    def test_deviceClasses(self):
        logger.info('== {} =='.format(self._testMethodName))
        # they are harmless
        # ToDo: move write from CheckDevices to these classes
        ft= Filter()
        self.assertIs(type(ft), Filter, 'no object of type: '.format(type(Filter)))
        ftw = FilterWheel()
        self.assertIs(type(ftw), FilterWheel, 'no object of type: '.format(type(FilterWheel)))
        foc = Focuser()
        self.assertIs(type(foc), Focuser, 'no object of type: '.format(type(Focuser)))
        ccd= CCD()
        self.assertIs(type(ccd), CCD, 'no object of type: '.format(type(CCD)))

#@unittest.skip('class not yet implemented')
class TestCreateDevices(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        self.rt = Configuration(logger=logger)
        self.fileName='./rts2saf-no-filter-wheel.cfg'
        self.success=self.rt.readConfiguration(fileName=self.fileName)

    #@unittest.skip('feature not yet implemented')
    def test_createFTs(self):
        logger.info('== {} =='.format(self._testMethodName))
        fts=CreateFilters(debug=False, rt=self.rt, logger=logger).create()
        self.assertIsNotNone(fts)
        self.assertEqual(fts[0].name, 'FAKE_FT')

    #@unittest.skip('feature not yet implemented')
    def test_createFOC(self):
        logger.info('== {} =='.format(self._testMethodName))
        cfoc= CreateFocuser(debug=False, rt=self.rt, logger=logger)
        foc=cfoc.create(focDef=0)
        self.assertEqual(foc.name, 'F0')

    #@unittest.skip('feature not yet implemented')
    def test_createFTWs(self):
        logger.info('== {} =='.format(self._testMethodName))
        fts=CreateFilters(debug=False, rt=self.rt, logger=logger).create()
        foc=CreateFocuser(debug=False, rt=self.rt, logger=logger).create(focDef=0)
        # no connection to real device
        foc.focDef=0
        cftw= CreateFilterWheels(debug=False, rt=self.rt, logger=logger, filters=fts, foc=foc)
        ftw=cftw.create()
        self.assertEqual(ftw[0].name, 'FAKE_FTW')

    #@unittest.skip('feature not yet implemented')
    def test_createCCD(self):
        logger.info('== {} =='.format(self._testMethodName))
        fts=CreateFilters(debug=False, rt=self.rt, logger=logger).create()
        foc=CreateFocuser(debug=False, rt=self.rt, logger=logger).create(focDef=0)
        cftw= CreateFilterWheels(debug=False, rt=self.rt, logger=logger, filters=fts, foc=foc)
        cccd= CreateCCD(debug=False, rt=self.rt, logger=logger, ftws=fts)
        ccd=cccd.create()
        self.assertEqual(ccd.name, 'C0')
        

from rts2saf.checkdevices import CheckDevices
from rts2.json import JSONProxy

@unittest.skip('these tests are done in test_focus')
class TestCheckDevices(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        self.rt = Configuration(logger=logger)
        self.fileName='./rts2saf-bootes-2.cfg'
        self.success=self.rt.readConfiguration(fileName=self.fileName)
        self.proxy=JSONProxy(url=self.rt.cfg['URL'],username=self.rt.cfg['USERNAME'],password=self.rt.cfg['PASSWORD'])
        self.scd= CheckDevices(debug=False, proxy=self.proxy, blind=None, verbose=None, ccd=None, ftws=None, foc=None, logger=logger)



    @unittest.skip('these tests are done in test_focus')
    def test_checkDevices(self):
        logger.info('== {} =='.format(self._testMethodName))


if __name__ == '__main__':

    suiteDevices=suite_devices()
    suiteCreateDevices= suite_create_devices()
    suiteCheckDevices= suite_check_devices()
    alltests = unittest.TestSuite([suiteDevices, suiteCreateDevices, suiteCheckDevices])
    unittest.TextTestRunner(verbosity=0).run(alltests)

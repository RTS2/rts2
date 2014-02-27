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
from rts2saf.environ import Environment

import logging
if not os.path.isdir('/tmp/rts2saf_log'):
    os.mkdir('/tmp/rts2saf_log')


logging.basicConfig(filename='/tmp/rts2saf_log/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()


# sequence matters
def suite():
    suite = unittest.TestSuite()
    suite.addTest(TestEnvironment('test_expandToTmp'))
    suite.addTest(TestEnvironment('test_expandToPlotFileName'))
    suite.addTest(TestEnvironment('test_expandToAcquisitionBasePath'))
    suite.addTest(TestEnvironment('test_createAcquisitionBasePath'))

    return suite

#@unittest.skip('class not yet implemented')
class TestEnvironment(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        self.rt = Configuration(logger=logger)
        self.rt.readConfiguration(fileName='./rts2saf-bootes-2.cfg')

    def test_expandToTmp(self):
        logger.info('== {} =='.format(self._testMethodName))
        ev=Environment(debug=False, rt=self.rt, logger=logger)
        res=ev.expandToTmp(fn='fn')
        self.assertEqual(res,'{}/fn'.format(self.rt.cfg['TEMP_DIRECTORY']),'return value:{}'.format(res))

    def test_expandToPlotFileName(self):
        logger.info('== {} =='.format(self._testMethodName))
        ev=Environment(debug=False, rt=self.rt, logger=logger)
        res=ev.expandToPlotFileName()
        self.assertEqual(res, 'myPlot-{}.png'.format(ev.startTime[0:19]),'return value:{}'.format(res))

    def test_expandToAcquisitionBasePath(self):
        logger.info('== {} =='.format(self._testMethodName))
        ev=Environment(debug=False, rt=self.rt, logger=logger)
        res=ev.expandToAcquisitionBasePath()
        tst='{}/{}'.format(self.rt.cfg['BASE_DIRECTORY'], ev.startTime)
        self.assertEqual( res, '{}/{}/'.format(self.rt.cfg['BASE_DIRECTORY'], ev.startTime),'return value:{}, {}'.format(res, tst))

    #@unittest.skip('feature not yet implemented')
    def test_createAcquisitionBasePath(self):
        logger.info('== {} =='.format(self._testMethodName))
        ev=Environment(debug=False, rt=self.rt, logger=logger)
        res=ev.createAcquisitionBasePath()
        self.assertTrue(res,'return value:{}'.format(res))


if __name__ == '__main__':
    
    unittest.TextTestRunner(verbosity=0).run(suite())

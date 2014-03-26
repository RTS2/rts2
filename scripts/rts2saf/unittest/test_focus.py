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


from rts2_environment import RTS2Environment

import logging
if not os.path.isdir('/tmp/rts2saf_log'):
    os.mkdir('/tmp/rts2saf_log')


logging.basicConfig(filename='/tmp/rts2saf_log/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()

# sequence matters
def suite_focus():
    suite = unittest.TestSuite()
    suite.addTest(TestFocusRuns('test_proxyConnection'))
    suite.addTest(TestFocusRuns('test_focus'))
    suite.addTest(TestFocusRuns('test_focus_blind'))
    return suite


#@unittest.skip('class not yet implemented')
class TestFocusRuns(RTS2Environment):

    #@unittest.skip('feature not yet implemented')
    def test_proxyConnection(self):
        logger.info('== {} =='.format(self._testMethodName))
        self.setupDevices()
        self.proxy.refresh()
        focPos = int(self.proxy.getSingleValue(self.foc.name,'FOC_POS'))
        self.assertEqual( focPos, 0, 'return value:{}'.format(focPos))

    #@unittest.skip('feature not yet implemented')
    def test_focus(self):
        logger.info('== {} =='.format(self._testMethodName))
        self.setupDevices()
        self.rts2safFoc.run()

    #@unittest.skip('feature not yet implemented')
    def test_focus_blind(self):
        logger.info('== {} =='.format(self._testMethodName))
        self.setupDevices(blind=True)
        self.rts2safFoc.run()
        self.args.blind=False

if __name__ == '__main__':

    suiteFocus=suite_focus()

    unittest.TextTestRunner(verbosity=0).run(suiteFocus)

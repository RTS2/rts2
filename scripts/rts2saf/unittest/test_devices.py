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

from rts2saf.config  import Configuration 
from rts2saf.devices import Filter, FilterWheel, Focuser, CCD

import logging
logging.basicConfig(filename='/tmp/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()

# sequence matters
def suite_devices():
    suite = unittest.TestSuite()
    suite.addTest(TestDevices('test_deviceClasses'))

    return suite



def suite_set_check_devices():
    suite = unittest.TestSuite()
    suite.addTest(TestDevices('test_SetCheckDevices'))

    return suite

#@unittest.skip('class not yet implemented')
class TestDevices(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        self.rt = Configuration(logger=logger)
        self.fileName='/usr/local/etc/rts2/rts2saf/rts2saf.cfg'
        self.success=self.rt.readConfiguration(fileName=self.fileName)

    #@unittest.skip('feature not yet implemented')
    def test_deviceClasses(self):
        # they are harmless
        # ToDo: move write from SetCheckDevices to these classes
        ft= Filter()
        self.assertIs(type(ft), Filter, 'no object of type: '.format(type(Filter)))
        ftw = FilterWheel()
        self.assertIs(type(ftw), FilterWheel, 'no object of type: '.format(type(FilterWheel)))
        foc = Focuser()
        self.assertIs(type(foc), Focuser, 'no object of type: '.format(type(Focuser)))
        ccd= CCD()
        self.assertIs(type(ccd), CCD, 'no object of type: '.format(type(CCD)))

import mock
from rts2saf.devices import SetCheckDevices
from rts2.json import JSONProxy

class TestSetCheckDevices(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        self.rt = Configuration(logger=logger)
        self.fileName='/usr/local/etc/rts2/rts2saf/rts2saf.cfg'
        self.success=self.rt.readConfiguration(fileName=self.fileName)
        self.proxy=JSONProxy(url=self.rt.cfg['URL'],username=self.rt.cfg['USERNAME'],password=self.rt.cfg['PASSWORD'])
        self.scd= SetCheckDevices(debug=False, proxy=self.proxy, rangeFocToff=None, blind=None, verbose=None, rt=self.rt, logger=logger)

    #@unittest.skip('feature not yet implemented')
    def test_SetCheckDevices(self):

        with mock.patch('rts2saf.devices.SetCheckDevices.xxfocuser') as focuser:
            focuser.return_value = True
            with mock.patch('rts2saf.devices.SetCheckDevices.xxcamera') as camera:
                camera.return_value = True
                with mock.patch('rts2saf.devices.SetCheckDevices.xxfilterWheels') as filterWheels:
                    filterWheels.return_value = True


                    self.assertTrue(self.scd.statusDevices())

if __name__ == '__main__':

    suiteDevices=suite_devices()
    suiteSetCheckDevices= suite_set_check_devices()

    unittest.TextTestRunner(verbosity=0).run([suiteDevices, suiteSetCheckDevices])

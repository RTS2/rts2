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
from rts2saf.temperaturemodel import TemperatureFocPosModel

import logging
if not os.path.isdir('/tmp/rts2saf_log'):
    os.mkdir('/tmp/rts2saf_log')


logging.basicConfig(filename='/tmp/rts2saf_log/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()

# sequence matters
def suite():
    suite = unittest.TestSuite()
    suite.addTest(TestTemperatureFocPosModel('test_fitData'))
    suite.addTest(TestTemperatureFocPosModel('test_plotData'))
    return suite

#@unittest.skip('class not yet implemented')
class TestTemperatureFocPosModel(unittest.TestCase):

    def tearDown(self):
        try:
            os.unlink(self.plotFnIn)
        except:
            pass

    def setUp(self):
        from rts2saf.data import ResultFit
        import numpy as np
        self.plotFnIn='./test-temperaturemodel-plot.png'
        date = '2013-09-08T09:30:09'
        resultFitFwhm=list()
        for i in range(0,10):
            resultFitFwhm.append(ResultFit(ambientTemp=float(i), extrFitPos=float( 2 * i)))

        self.ft = TemperatureFocPosModel(showPlot=False, date=date,  comment='unittest', plotFn=self.plotFnIn, resultFitFwhm=resultFitFwhm, logger=logger)

    def test_fitData(self):
        par, flag= self.ft.fitData()
        if flag:
            # ToDo might not what I want
            self.assertAlmostEqual(par[0], 8.881784197e-16, places=6, msg='return value: '.format(par[0]))
        else:
            self.assertEqual(0, 1, 'fit failed')
                            
    #@unittest.skip('feature not yet implemented')
    def test_plotData(self):
        par, flag= self.ft.fitData()
        plotFnOut=self.ft.plotData()
        self.assertEqual(self.plotFnIn, plotFnOut, 'return value: {}'.format(plotFnOut))

if __name__ == '__main__':
    
    unittest.TextTestRunner(verbosity=0).run(suite())

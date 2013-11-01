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
from rts2saf.data import ResultFitFwhm

import logging
logging.basicConfig(filename='/tmp/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
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
        from rts2saf.data import DataFitFwhm
        import numpy as np
        # ugly
        self.plotFnIn='./test-temperaturemodel-plot.png'
        date = '2013-09-08T09:30:09'
        resultFitFwhm=list()
        for i in range(0,10):
            resultFitFwhm.append(ResultFitFwhm(ambientTemp=float(i), minFitPos=float( 2 * i)))

        self.ft = TemperatureFocPosModel(showPlot=False, date=date,  comment='unittest', plotFn=self.plotFnIn, resultFitFwhm=resultFitFwhm, logger=logger)

    def test_fitData(self):
        par, flag= self.ft.fitData()
        # ToDo might not what I want
        self.assertAlmostEqual('{0:12.9}'.format(par[0]), '{0:12.9}'.format(8.881784197e-16), 'return value: {}'.format(par[0]))
                                               
    #@unittest.skip('feature not yet implemented')
    def test_plotData(self):
        par, flag= self.ft.fitData()
        plotFnOut=self.ft.plotData()
        self.assertEqual(self.plotFnIn, plotFnOut, 'return value: {}'.format(plotFnOut))

if __name__ == '__main__':
    
    unittest.TextTestRunner(verbosity=0).run(suite())

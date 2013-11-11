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
from rts2saf.fitfunction import FitFunction

import logging
logging.basicConfig(filename='/tmp/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()

# sequence matters
def suite():
    suite = unittest.TestSuite()
    suite.addTest(TestFitFunction('test_fitData'))
    return suite

#@unittest.skip('class not yet implemented')
class TestFitFunction(unittest.TestCase):

    def tearDown(self):
        try:
            os.unlink(self.plotFnIn)
        except:
            pass

    def setUp(self):
        from rts2saf.data import DataFitFwhm, DataFitFlux, DataSxtr
        import numpy as np
        pos       = np.asarray([ 2000., 2100., 2200., 2300., 2400., 2500., 2600., 2700., 2800., 2900., 3000.])
        fwhm      = np.asarray([   40.,   30.,   20.,   15.,   10.,    5.,   10.,   15.,   20.,   30.,   40.])
        stdFocPos = np.asarray([   20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.])
        stdFwhm   = np.asarray([    2.,    2.,    2.,    2.,    2.,    2.,    2.,    2.,    2.,    2.,    2.])
        flux      = np.asarray([   2.2,   3.1,   4.8,   7.9,   10.1, 11.2,  11.1,   8.2,  5.4,   3.2,    2.2])
        stdFlux   = np.asarray([  0.5,    0.5,   0.5,   0.5,   0.5,   0.5,   0.5,   0.5,   0.5,   0.5,   0.5])

        dataSxtr=list()
        for i in range( 0, len(pos)-1):
            dataSxtr.append(
                DataSxtr(
                    focPos=pos[i],
                    stdFocPos=stdFocPos[i],
                    fwhm=fwhm[i],
                    stdFwhm=stdFwhm[i],
                    flux=flux[i],
                    stdFlux=stdFlux[i],
                    catalog=[1, 2, 3]
                    )
                )

        self.plotFnIn='./test-plot.png'
        self.date = '2013-09-08T09:30:09'

        self.dataFitFwhm=DataFitFwhm(
            dataSxtr=dataSxtr,
            ambientTemp='21.3',
            plotFn= self.plotFnIn)

        self.ft = FitFunction(dataFit=self.dataFitFwhm, logger=logger)

    def test_fitData(self):
        min_focpos_fwhm, val_fwhm, par, flag= self.ft.fitData()
        self.assertAlmostEqual(min_focpos_fwhm, 2507.66745331, places=3, msg='return value: {}'.format(min_focpos_fwhm))
                                               

if __name__ == '__main__':
    
    unittest.TextTestRunner(verbosity=0).run(suite())

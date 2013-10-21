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


from rts2saf.fitfwhm import FitFwhm
import unittest


class TestFitFwhm(unittest.TestCase):

    def setUp(self):
        from rts2saf.data import DataFitFwhm
        import numpy as np

        dataFitFwhm=DataFitFwhm(
            pos= np.asarray([ 2000., 2100., 2200., 2300., 2400., 2500., 2600., 2700., 2800., 2900., 3000.]),
            fwhm= np.asarray([ 40.,   30.,   20.,   15.,   10.,    5.,   10.,   15.,   20.,   30.,   40.]),
            errx= np.asarray([ 20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.]),
            stdFwhm= np.asarray([  2.,    2.,    2.,    2.,    2.,    2.,    2.,    2.,    2.,    2.,    2.]),)

        self.ft= FitFwhm(showPlot=True, date='2013-09-08T09:30:09',  comment='unittest', dataFitFwhm=dataFitFwhm, logger=None)

    def tearDown(self):
        pass


    def test_fitData(self):
        self.min_focpos_fwhm, self.val_fwhm, self.par= self.ft.fitData()
        self.assertEqual(self.min_focpos_fwhm, 2499.4659033048924)
                                               

    def test_plotData(self):
        self.ft.plotData()

if __name__ == '__main__':
    
    suite = unittest.TestLoader().loadTestsFromTestCase(TestFitFwhm)
    unittest.TextTestRunner(verbosity=0).run(suite)

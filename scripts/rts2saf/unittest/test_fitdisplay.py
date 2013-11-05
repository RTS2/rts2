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
from rts2saf.fitfunction import  FitFunction
from rts2saf.fitdisplay import FitDisplay
from rts2saf.data import ResultFit

import numpy as np
import logging
logging.basicConfig(filename='/tmp/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()

# sequence matters
def suite():
    suite = unittest.TestSuite()
    suite.addTest(TestFitDisplay('test_FwhmfitDisplay'))
    suite.addTest(TestFitDisplay('test_FluxfitDisplay'))
    return suite

#@unittest.skip('class not yet implemented')
class TestFitDisplay(unittest.TestCase):

    def tearDown(self):
        try:
            os.unlink(self.plotFnIn)
        except:
            pass

    def setUp(self):
        from rts2saf.data import DataFit
        # ugly
        self.plotFnIn='./test-plot.png'
        self.date = '2013-09-08T09:30:09'
        self.dataFitFwhm=DataFit(
            pos= np.asarray([ 2000., 2100., 2200., 2300., 2400., 2500., 2600., 2700., 2800., 2900., 3000.]),
            val= np.asarray([ 40.,   30.,   20.,   15.,   10.,    5.,   10.,   15.,   20.,   30.,   40.]),
            errx= np.asarray([ 20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.]),
            erry= np.asarray([  2.,    2.,    2.,    2.,    2.,    2.,    2.,    2.,    2.,    2.,    2.]),
            ambientTemp='21.3',
            plotFn= self.plotFnIn)

        self.parFwhm= np.array([1., 1., 1., 1.])

        self.dataFitFlux=DataFit(
            pos= np.asarray([ 2000., 2100., 2200., 2300., 2400., 2500., 2600., 2700., 2800., 2900., 3000.]),
            val= np.asarray([   2.2, 3.1,   4.8,   7.9,   10.1, 11.2,  11.1,   8.2,  5.4,   3.2,    2.2]),
            errx= np.asarray([ 20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.]),
            erry= np.asarray([  0.5,    0.5,    0.5,    0.5,    0.5,    0.5,    0.5,    0.5,    0.5,    0.5,    0.5]),
            ambientTemp='21.3',
            plotFn= self.plotFnIn)

        self.ffwhm = lambda x, p: p[0] + p[1] * x + p[2] * (x ** 2)+ p[3] * (x ** 4)  # due to optimize.fminbound
        # ToDO, try old function
        # self.fflux = lambda x, p: p[0] * p[4] / ( p[4] + p[3] * (abs(x - p[1]) ** p[2]))
        # gaussian
        self.fflux = lambda x, p: p[3] + p[0]*np.exp(-(x-p[1])**2/(2*p[2]**2))
        # find minimum
        self.recpFlux = lambda x, p: 1./(p[3] + p[0]*np.exp(-(x-p[1])**2/(2*p[2]**2)))

        
    @unittest.skip('feature not yet implemented')
    def test_FwhmfitDisplay(self):
        ft = FitFunction(dataFit=self.dataFitFwhm, fitFunc=self.ffwhm, logger=logger, par=self.parFwhm)
        min_focpos_fwhm, val_fwhm, par, flag= ft.fitData()

        resultFitFwhm=ResultFit(
            ambientTemp='20.1', 
            ftName='UNK',
            extrFitPos=min_focpos_fwhm, 
            extrFitVal=val_fwhm, 
            fitPar=par,
            fitFlag=flag
            )

        self.fd = FitDisplay(date=self.date, comment='unittest', logger=logger)
        self.fd.fitDisplay(dataFit=self.dataFitFwhm, resultFit=resultFitFwhm, fitFunc=self.ffwhm)

    #@unittest.skip('feature not yet implemented')
    def test_FluxfitDisplay(self):
        ft = FitFunction(dataFit=self.dataFitFwhm, fitFunc=self.ffwhm, par=self.parFwhm, logger=logger)
        min_focpos_fwhm, val_fwhm, par, flag= ft.fitData()

        if flag!=None:
            resultFitFwhm=ResultFit(
                ambientTemp='20.1', 
                ftName='UNK',
                extrFitPos=min_focpos_fwhm, 
                extrFitVal=val_fwhm, 
                fitPar=par,
                fitFlag=flag,
                color='blue',
                ylabel='FWHM [px]: blue'
                )

            fd = FitDisplay(date=self.date, comment='unittest', logger=logger)
            fd.fitDisplay(dataFit=self.dataFitFwhm, resultFit=resultFitFwhm, fitFunc=self.ffwhm)

        x=np.array([p for p in self.dataFitFlux.pos])
        y=np.array([v for v in self.dataFitFlux.val])
        wmean= np.average(a=x, weights=y) 
        xy= zip(x,y)
        wstd = np.std(a=xy) 
        # scale the values [a.u.]
        sv = [max(self.dataFitFwhm.val) / max(self.dataFitFlux.val) * x for x in self.dataFitFlux.val]
        self.dataFitFlux.val=sv

        # gaussian
        self.parFlux= np.array([ 10., wmean, wstd/4., 2.])
        ft = FitFunction(dataFit=self.dataFitFlux, fitFunc=self.fflux, par=self.parFlux, recpFunc=self.recpFlux, logger=logger)
        max_focpos_flux, val_flux, par, flag= ft.fitData()
        print max_focpos_flux, val_flux, par, flag
        if flag!=None:
            resultFitFlux=ResultFit(
                ambientTemp='20.1', 
                ftName='UNK',
                extrFitPos=max_focpos_flux, 
                extrFitVal=val_flux, 
                fitPar=par,
                fitFlag=flag,
                color='red',
                ylabel='FWHM [px]: blue, Flux [a.u.]:red'
                )

            fd.fitDisplay(dataFit=self.dataFitFlux, resultFit=resultFitFlux, fitFunc=self.fflux, display=True)

if __name__ == '__main__':
    
    unittest.TextTestRunner(verbosity=0).run(suite())

#!/usr/bin/python
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

__author__ = 'markus.wildi@bluewin.ch'

import os
import sys
import matplotlib
try:
    os.environ['DISPLAY']
    NODISPLAY=False
except:
    matplotlib.use('Agg')    
    NODISPLAY=True

import matplotlib.pyplot as plt
import numpy as np
from scipy import optimize
try:
    import lib.data as dtf
except:
    import data as dtf

class FitFwhm(object):
    """ Fit FWHM data and find the minimum"""
    def __init__(self, showPlot=False, filterName=None, ambientTemp=None, date=None,  comment=None, pltFile=None, dataFitFwhm=None, logger=None):

        self.showPlot=showPlot
        self.filterName=filterName
        self.ambientTemp=ambientTemp
        self.date=date
        self.dataFitFwhm=dataFitFwhm
        self.logger=logger
        self.comment=comment
        self.pltFile=pltFile
        self.par=None
        self.flag=None
        self.min_focpos_fwhm=None
        self.fitfunc_fwhm = lambda p, x: p[0] + p[1] * x + p[2] * (x ** 2)+ p[3] * (x ** 4)
        self.errfunc_fwhm = lambda p, x, y, res, err: (y - self.fitfunc_fwhm(p, x)) / (res * err) # ToDo why err
        self.fitfunc_r_fwhm = lambda x, p0, p1, p2, p3, p4: p0 + p1 * x + p2 * (x ** 2) + p3 * (x ** 4) 
        # ToDO I want this:
        #self.fitfunc_r_fwhm = lambda p, x: p[0] + p[1] * x + p[2] * (x ** 2)+ p[3] * (x ** 4) 
        # resp:
        # self.fitfunc_r_fwhm==self.fitfunc_fwhm
    def fitData(self):
        self.par= np.array([1., 1., 1., 1., 1.])
        try:
            self.par, self.flag  = optimize.leastsq(self.errfunc_fwhm, self.par, args=(self.dataFitFwhm.pos, self.dataFitFwhm.fwhm, self.dataFitFwhm.errx, self.dataFitFwhm.stdFwhm))
        except Exception, e:
            self.logger.error('fitfwhm: fitData: failed fitting FWHM:\nnumpy error message:\n{0}'.format(e))                
            return None, None
        # ToDo lazy
        posS=sorted(self.dataFitFwhm.pos)
        step= posS[1]-posS[0]
        try:
            self.min_focpos_fwhm = optimize.fminbound(self.fitfunc_r_fwhm,min(self.dataFitFwhm.pos)-2 * step, max(self.dataFitFwhm.pos)+2 * step,args=(self.par), disp=0)
        except Exception, e:
            self.logger.error('fitfwhm: fitData: failed finding minimum FWHM:\nnumpy error message:\n{0}'.format(e))                
            return None, None

        val_fwhm= self.fitfunc_fwhm( self.par, self.min_focpos_fwhm) 
        # a decision is done in Analyze
        return self.min_focpos_fwhm, val_fwhm

    def plotData(self):

        try:
            x_fwhm = np.linspace(self.dataFitFwhm.pos.min(), self.dataFitFwhm.pos.max())
        except Exception, e:
            self.logger.error('fitfwhm: numpy error:\n{0}'.format(e))                
            return
        plt.plot(self.dataFitFwhm.pos, self.dataFitFwhm.fwhm, 'ro', color='blue')
        plt.errorbar(self.dataFitFwhm.pos, self.dataFitFwhm.fwhm, xerr=self.dataFitFwhm.errx, yerr=self.dataFitFwhm.stdFwhm, ecolor='black', fmt=None)

        if self.flag:
            plt.plot(x_fwhm, self.fitfunc_fwhm(self.par, x_fwhm), 'r-', color='blue')
            
        if self.ambientTemp and self.comment:
            plt.title('rts2saf, {0},{1},{2},min:{3:.0f},{4}'.format(self.date, self.filterName, self.ambientTemp, float(self.min_focpos_fwhm), self.comment), fontsize=12)
        elif self.ambientTemp:
            plt.title('rts2saf, {0},{1},{2},min:{3:.0f}'.format(self.date, self.filterName, self.ambientTemp, float(self.min_focpos_fwhm)), fontsize=12)
        elif self.comment:
            plt.title('rts2saf, {0},{1},min:{2:.0f},{3}'.format(self.date, self.filterName, float(self.min_focpos_fwhm), self.comment), fontsize=12)
        else:
            plt.title('rts2saf, {0},{1},min:{2:.0f}'.format(self.date, self.filterName, float(self.min_focpos_fwhm)), fontsize=12)

        plt.xlabel('FOC_POS [tick]')
        plt.ylabel('FWHM [px]')
        plt.grid(True)

        try:
            plt.savefig(self.pltFile)
        except:
            self.logger.error('fitfwhm: can not save plot to: {0}'.format(self.pltFile))                
            
        if self.showPlot:
            if NODISPLAY:
                self.logger.warn('fitfwhm: NO $DISPLAY no plot')                
                return
            else:
                plt.show()


if __name__ == '__main__':

    dataFitFwhm=dtf.DataFitFwhm(
        pos= np.asarray([ 2000., 2100., 2200., 2300., 2400., 2500., 2600., 2700., 2800., 2900., 3000.]),
        fwhm= np.asarray([ 40.,   30.,   20.,   15.,   10.,    5.,   10.,   15.,   20.,   30.,   40.]),
        errx= np.asarray([ 20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.,   20.]),
        stdFwhm= np.asarray([  2.,    2.,    2.,    2.,    2.,    2.,    2.,    2.,    2.,    2.,    2.]),)
 

    fit=FitFwhm(showPlot=True, filterName='U', ambientTemp=20., date='2013-09-08T09:30:09', comment='Test fitfwhm', pltFile='./test-fit.png', dataFitFwhm=dataFitFwhm)
    fit.fitData()
    fit.plotData()

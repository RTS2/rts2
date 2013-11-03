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
import math
import matplotlib.pyplot as plt
import numpy as np
from scipy import optimize
import rts2saf.data as dtf

class FitFwhm(object):
    """ Fit FWHM data and find the minimum"""
    def __init__(self, showPlot=False, date=None,  comment=None, dataFitFwhm=None, logger=None):

        self.showPlot=showPlot
        self.date=date
        self.dataFitFwhm=dataFitFwhm
        self.logger=logger
        self.comment=comment
        self.par=None
        self.flag=None
        self.min_focpos_fwhm=float('nan')
        self.f = lambda x, p: p[0] + p[1] * x + p[2] * (x ** 2)+ p[3] * (x ** 4)
        self.e = lambda p, x, y, res, err: (y - self.f(x, p)) / (res * err) 
        self.val_fwhm=None

    def fitData(self):
        self.par= np.array([1., 1., 1., 1.])
        try:
            self.par, self.flag  = optimize.leastsq(self.e, self.par, args=(self.dataFitFwhm.pos, self.dataFitFwhm.fwhm, self.dataFitFwhm.errx, self.dataFitFwhm.stdFwhm))
        except Exception, e:
            self.logger.error('fitfwhm: fitData: failed fitting FWHM:\nnumpy error message:\n{0}'.format(e))                
            return None, None, None

        # ToDo lazy
        posS=sorted(self.dataFitFwhm.pos)
        step= posS[1]-posS[0]
        try:
            self.min_focpos_fwhm = optimize.fminbound(self.f,x1=min(self.dataFitFwhm.pos)-2 * step, x2=max(self.dataFitFwhm.pos)+2 * step, args=[self.par])
        except Exception, e:
            self.logger.error('fitfwhm: fitData: failed finding minimum FWHM:\nnumpy error message:\n{0}'.format(e))                
            return None, None, None

        self.val_fwhm= self.f(x=self.min_focpos_fwhm, p=[self.par[0], self.par[1],self.par[2],self.par[3]]) 
        # a decision is done in Analyze
        return self.min_focpos_fwhm, self.val_fwhm, self.par, self.flag

    def plotData(self):
        #if not self.min_focpos_fwhm:
        #    return
        try:
            x_fwhm = np.linspace(self.dataFitFwhm.pos.min(), self.dataFitFwhm.pos.max())
        except Exception, e:
            self.logger.error('fitfwhm: numpy error:\n{0}'.format(e))                
            return e

        plt.plot(self.dataFitFwhm.pos, self.dataFitFwhm.fwhm, 'ro', color='blue')
        plt.errorbar(self.dataFitFwhm.pos, self.dataFitFwhm.fwhm, xerr=self.dataFitFwhm.errx, yerr=self.dataFitFwhm.stdFwhm, ecolor='black', fmt=None)

        if self.flag:
            plt.plot(x_fwhm, self.f(x_fwhm, p=[self.par[0], self.par[1],self.par[2],self.par[3]]), 'r-', color='blue')
            
        if self.comment:
            plt.title('rts2saf, {0},{1},{2}C,min:{3:.0f},{4}'.format(self.date, self.dataFitFwhm.ftName, self.dataFitFwhm.ambientTemp, float(self.min_focpos_fwhm), self.comment), fontsize=12)
        else:
            plt.title('rts2saf, {0},{1},{2}C,min:{3:.0f}'.format(self.date, self.dataFitFwhm.ftName, self.dataFitFwhm.ambientTemp, float(self.min_focpos_fwhm)), fontsize=12)

        plt.xlabel('FOC_POS [tick]')
        plt.ylabel('FWHM [px]')
        plt.grid(True)

            
        if self.showPlot:
            if NODISPLAY:
                self.logger.warn('fitfwhm: NO $DISPLAY no plot')                
                return
            else:
                plt.show()

        try:
            plt.savefig(self.dataFitFwhm.plotFn)
            return self.dataFitFwhm.plotFn
        except Exception, e:
            self.logger.error('fitfwhm: can not save plot to: {0}'.format(self.dataFitFwhm.plotFn))
            return e

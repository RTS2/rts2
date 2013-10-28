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

class TemperatureFocPosModel(object):
    """ Fit minimum focuser positions as function of temperature"""
    def __init__(self, showPlot=False, date=None,  comment=None, plotFn=None, resultFitFwhm=None, logger=None):

        self.showPlot=showPlot
        self.date=date
        self.logger=logger
        self.comment=comment
        self.plotFn=plotFn
        self.resultFitFwhm=resultFitFwhm
        self.fitfunc = lambda p, x: p[0] + p[1] * x 
        self.errfunc = lambda p, x, y, res, err: (y - self.fitfunc(p, x))/(res * err)
        self.temp=list() 
        self.tempErr=list()
        self.minFitPos=list()
        self.minFitPosErr=list()

    def __prepare(self):

        for rFF in self.resultFitFwhm:
            try:
                self.temp.append(float(rFF.ambientTemp))
            except:
                self.temp.append(rFF.ambientTemp)
            self.tempErr.append(0.01)
            self.minFitPos.append(float(rFF.minFitPos))
            self.minFitPosErr.append(2.5) 

    def fitData(self):
        self.__prepare()
        self.par= np.array([1., 1.])
        try:
            self.par, self.flag  = optimize.leastsq(self.errfunc, self.par, args=(np.asarray(self.temp), np.asarray(self.minFitPos),np.asarray(self.tempErr),np.asarray(self.minFitPosErr)))
        except Exception, e:
            self.logger.error('temperaturemodel: fitData: failed fitting model:\nnumpy error message:\n{0}'.format(e))                
            return None, None

        return self.par, self.flag

    def plotData(self):
        try:
            x_temp = np.linspace(min(self.temp), max(self.temp))
        except Exception, e:
            self.logger.error('temperaturemodel: numpy error:\n{0}'.format(e))                
            return e

        plt.plot(self.temp,self.minFitPos, 'ro', color='blue')
        plt.errorbar(self.temp,self.minFitPos, xerr=self.tempErr, yerr=self.minFitPosErr, ecolor='blue', fmt=None)

        if self.flag:
            plt.plot(x_temp, self.fitfunc(self.par, x_temp), 'r-', color='red')
            
        if self.comment:
            plt.title('rts2saf {0}, temperature model, {1}'.format(self.date, self.comment), fontsize=12)
        else:
            plt.title('rts2saf {0}, temperature model'.format(self.date), fontsize=12)

        plt.xlabel('ambient temperature [degC]')
        plt.ylabel('FOC_POS(min. FWHM) [ticks]')
        plt.grid(True)

        if self.showPlot:
            if NODISPLAY:
                self.logger.warn('temperaturemodel: NO $DISPLAY no plot')                
                return
            else:
                plt.show()

        try:
            plt.savefig(self.plotFn)
            return self.plotFn
        except Exception, e:
            self.logger.error('temperaturemodel: can not save plot to: {0}'.format(self.plotFn))                
            return e


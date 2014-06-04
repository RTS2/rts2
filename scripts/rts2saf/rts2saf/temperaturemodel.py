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
"""
"""

__author__ = 'markus.wildi@bluewin.ch'

import sys

if 'matplotlib' not in sys.modules: 
    import matplotlib
    matplotlib.use('Agg')    

import matplotlib.pyplot as plt
import numpy as np
from scipy import optimize


class TemperatureFocPosModel(object):
    """ Fit minimum focuser positions as function of temperature

        :var showPlot: if True show plot
        :var date: date when fit has been carried out 
        :var comment: optional comment
        :var plotFn: file name where the plot is stored as PNG
        :var resultFitFwhm: :py:mod:`rts2saf.data.ResultFwhm`
        :var logger:  :py:mod:`rts2saf.log`

    """
    def __init__(self, showPlot=False, date=None,  comment=None, plotFn=None, resultFitFwhm=None, logger=None):

        self.showPlot=showPlot
        self.date=date
        self.logger=logger
        self.comment=comment
        self.plotFn=plotFn
        self.resultFitFwhm=resultFitFwhm
        self.fitfunc = lambda p, x: p[0] + p[1] * x 
        self.errfunc = lambda p, x, y, res, err: (y - self.fitfunc(p, x))
        self.temp=list() 
        self.tempErr=list()
        self.minFitPos=list()
        self.minFitPosErr=list()
        self.fig = None
        self.ax1 = None
        self.fig = plt.figure()
        self.ax1 = self.fig.add_subplot(111)

    def fitData(self):
        """Fit function using  optimize.leastsq().

        :return par, flag:  fit parameters, non zero if successful

        """
        for rFF in self.resultFitFwhm:
            try:
                self.temp.append(float(rFF.ambientTemp))
            except Exception, e:
                self.temp.append(rFF.ambientTemp)
                if self.debug: self.logger.debug('temperaturemodel: new list created, error: {}'.format(e))                

            self.tempErr.append(0.01)
            try:
                self.minFitPos.append(float(rFF.extrFitPos))
            except Exception, e:
                self.logger.error('temperaturemodel: serious error: could not append type: {}, error: {}'.format(type(rFF.extrFitPos), e))
                
            self.minFitPosErr.append(2.5) 

        par= np.array([1., 1.])
        self.flag=None
        try:
            self.par, self.flag  = optimize.leastsq(self.errfunc, par, args=(np.asarray(self.temp), np.asarray(self.minFitPos),np.asarray(self.tempErr),np.asarray(self.minFitPosErr)))
        except Exception, e:
            self.logger.error('temperaturemodel: fit failed:\n{0}'.format(e))                
            return None, None


        return self.par, self.flag

    def plotData(self):
        """Display fit using matplotlib

        :return:  :py:mod:`rts2saf.data.DataFit`.plotFn

        """

        try:
            x_temp = np.linspace(min(self.temp), max(self.temp))
        except Exception, e:
            self.logger.error('temperaturemodel: numpy error:\n{0}'.format(e))                
            return e

        self.ax1.plot(self.temp,self.minFitPos, 'ro', color='blue')
        self.ax1.errorbar(self.temp,self.minFitPos, xerr=self.tempErr, yerr=self.minFitPosErr, ecolor='blue', fmt=None)

        if self.flag:
            self.ax1.plot(x_temp, self.fitfunc(self.par, x_temp), 'r-', color='red')
            
        if self.comment:
            self.ax1.set_title('rts2saf {0}, temperature model, {1}'.format(self.date, self.comment), fontsize=12)
        else:
            self.ax1.set_title('rts2saf {0}, temperature model'.format(self.date), fontsize=12)

        self.ax1.set_xlabel('ambient temperature [degC]')
        self.ax1.set_ylabel('FOC_POS(min. FWHM) [ticks]')
        self.ax1.grid(True)

        if self.showPlot:
            plt.show()
        else:
            self.logger.warn('temperaturemodel: NO $DISPLAY no plot')                
            # no return here

        try:
            self.fig.savefig(self.plotFn)
            return self.plotFn
        except Exception, e:
            self.logger.error('temperaturemodel: can not save plot to: {0}, matplotlib msg: {1}'.format(self.plotFn, e))                
            return e


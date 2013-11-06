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
import rts2saf.data as dtf

class  FitDisplay(object):
    """Display a fit"""
    def __init__(self, date=None,  comment=None, logger=None):

        self.date=date
        self.logger=logger
        self.comment=comment
        self.fig=None
        self.ax1=None
        self.fig = plt.figure()
        self.ax1 = self.fig.add_subplot(111)

    def fitDisplay(self, dataFit=None, resultFit=None, display=False):

        try:
            x_pos = np.linspace(dataFit.pos.min(), dataFit.pos.max())
        except Exception, e:
            self.logger.error('fitDisplay: numpy error:\n{0}'.format(e))                
            return e

        self.ax1.plot(dataFit.pos, dataFit.val, 'ro', color=resultFit.color)
        self.ax1.errorbar(dataFit.pos, dataFit.val, xerr=dataFit.errx, yerr=dataFit.erry, ecolor='black', fmt=None)

        if resultFit.fitFlag:
            line, = self.ax1.plot(x_pos, dataFit.fitFunc(x_pos, p=[ x for x in resultFit.fitPar]), 'r-', color=resultFit.color)
            
            if self.comment:
                self.ax1.set_title('rts2saf, {0},{1},{2}C,{3},{4}'.format(self.date, dataFit.ftName, dataFit.ambientTemp, resultFit.titleResult, self.comment), fontsize=12)
            else:
                self.ax1.set_title('rts2saf, {0},{1},{2}C,{3}'.format(self.date, dataFit.ftName, dataFit.ambientTemp, resultFit.titleResult), fontsize=12)

        self.ax1.set_xlabel('FOC_POS [tick]')
        self.ax1.set_ylabel(resultFit.ylabel)
        self.ax1.grid(True)


        if display:
            if NODISPLAY:
                self.logger.warn('fitDisplay: NO $DISPLAY no plot')                
                return
            else:
                #NO: self.fig.show()
                plt.show()
        try:
            fig.savefig(dataFit.plotFn)
            return dataFit.plotFn
        except Exception, e:
            self.logger.error('fitDisplay: can not save plot to: {0}'.format(dataFit.plotFn))
            return e

        return self.fig
            

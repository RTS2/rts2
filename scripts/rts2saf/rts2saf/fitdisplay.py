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

import sys
import os
import psutil
import matplotlib

# ToDo Ad hoc:
# if executed through RTS2 it has as DISPLAY localhost:10
# plt.figure() fails
#
# http://stackoverflow.com/questions/1027894/detect-if-x11-is-available-python                                                                                    

#from subprocess import Popen, PIPE
#p = Popen(["xset", "-q"], stdout=PIPE, stderr=PIPE)
#p.communicate()
#if p.returncode == 0:
#    NODISPLAY=False
#else:
#    matplotlib.use('Agg')    
#    NODISPLAY=True

pnm=psutil.Process(psutil.Process(os.getpid()).parent.pid).name
if 'init' in pnm or 'rts2-executor' in pnm:
    matplotlib.use('Agg')    
    DISPLAY=False
else:
    from subprocess import Popen, PIPE
    p = Popen(["xset", "-q"], stdout=PIPE, stderr=PIPE)
    p.communicate()
    if p.returncode == 0:
        DISPLAY=True
    else:
        matplotlib.use('Agg')    
        DISPLAY=False




import matplotlib.pyplot as plt

import numpy as np
import rts2saf.data as dtf

class  FitDisplay(object):
    """Display a fit with matplotlib

    :var date: date when focus run started
    :var comment: optional comment
    :var logger: :py:mod:`rts2saf.log`

    """

    def __init__(self, date=None,  comment=None, logger=None):

        self.date=date
        self.logger=logger
        self.comment=comment
        self.fig=None
        self.ax1=None
        self.fig = plt.figure()
        self.ax1 = self.fig.add_subplot(111)

    def fitDisplay(self, dataFit=None, resultFit=None, display=False):
        """Display fit using matplotlib

        :param dataFit: :py:mod:`rts2saf.data.DataFit`
        :param resultFit: :py:mod:`rts2saf.data.ResultFit`
        :param display: if True display and save plot to file, False save only

        :return:  :py:mod:`rts2saf.data.DataFit`.plotFn

        """
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
            if DISPLAY:
                #NO: self.fig.show()
                plt.show()
            else:
                self.logger.warn('fitDisplay: NO $DISPLAY no plot')                
                # no return here
        
        try:
            self.fig.savefig(dataFit.plotFn)
            return dataFit.plotFn
        except Exception, e:
            self.logger.error('fitDisplay: can not save plot to: {0}, matplotlib error:\n{1}'.format(dataFit.plotFn,e))
            return e


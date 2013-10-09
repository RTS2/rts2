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
    def __init__(self, showPlot=False, date=None,  comment=None, pltFile=None, resultFitFwhm=None, logger=None):

        self.showPlot=showPlot
        self.date=date
        self.logger=logger
        self.comment=comment
        self.pltFile=pltFile
        self.resultFitFwhm=resultFitFwhm
        self.fitfunc = lambda p, x: p[0] + p[1] * x 
        self.errfunc = lambda p, x, y, res, err: (y - self.fitfunc(p, x))/(res * err)
        self.temp=list() 
        self.tempErr=list()
        self.minFitPos=list()
        self.minFitPosErr=list()

    def __prepare(self):

        for rFF in self.resultFitFwhm:
            self.temp.append(float(rFF.ambientTemp))
            self.tempErr.append(0.01)
            self.minFitPos.append(float(rFF.minFitPos))
            self.minFitPosErr.append(2.5) 

    def fitData(self):
        self.__prepare()
        self.par= np.array([1., 1.])
        try:
            self.par, self.flag  = optimize.leastsq(self.errfunc, self.par, args=(np.asarray(self.temp), np.asarray(self.minFitPos),np.asarray(self.tempErr),np.asarray(self.minFitPosErr)))
        except Exception, e:
            self.logger.error('fitfwhm: fitData: failed fitting FWHM:\nnumpy error message:\n{0}'.format(e))                
            return None, None

    def plotData(self):
        try:
            x_temp = np.linspace(min(self.temp), max(self.temp))
        except Exception, e:
            self.logger.error('temperaturemodel: numpy error:\n{0}'.format(e))                
            return

        plt.plot(self.temp,self.minFitPos, 'ro', color='blue')
        plt.errorbar(self.temp,self.minFitPos, xerr=self.tempErr, yerr=self.minFitPosErr, ecolor='blue', fmt=None)

        if self.flag:
            plt.plot(x_temp, self.fitfunc(self.par, x_temp), 'r-', color='red')
            
        if self.comment:
            plt.title('rts2saf {0}, temperature model, {1}'.format(self.date, self.comment), fontsize=12)
        else:
            plt.title('rts2saf {0}, temperature model'.format(self.date), fontsize=12)

        plt.xlabel('ambient temperature [degC]')
        plt.ylabel('FOC_POS(min. FWHM)   [ticks]')
        plt.grid(True)

        try:
            plt.savefig(self.pltFile)
        except:
            self.logger.error('fitfwhm: can not save plot to: {0}'.format(self.pltFile))                
            
        if self.showPlot:
            if NODISPLAY:
                self.logger.warn('temperaturemodel: NO $DISPLAY no plot')                
                return
            else:
                plt.show()
if __name__ == '__main__':
    import argparse

    try:
        import lib.config as cfgd
    except:
        import config as cfgd

    try:
        import lib.environ as env
    except:
        import environ as env

    try:
        import lib.log as lg
    except:
        import log as lg
    try:
        import lib.data as dt
    except:
        import data as dt

    import re
    
    prg= re.split('/', sys.argv[0])[-1]
    parser= argparse.ArgumentParser(prog=prg, description='rts2asaf analysis')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--logfile',dest='logfile', default='{0}.log'.format(prg), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--config', dest='config', action='store', default='/etc/rts2/rts2saf/rts2saf.cfg', help=': %(default)s, configuration file path')
    args=parser.parse_args()

    # logger
    lgd= lg.Logger(debug=args.debug, args=args) # if you need to chage the log format do it here
    logger= lgd.logger 
    # config
    rt=cfgd.Configuration(logger=logger)
    rt.readConfiguration(fileName=args.config)
    rt.checkConfiguration()
    # environment
    ev=env.Environment(debug=args.debug, rt=rt,logger=logger)
    ev.createAcquisitionBasePath(ftwName=None, ftName=None)




    rFF=list()
    for i in range(0,10):
        rFF.append(dt.ResultFitFwhm(ambientTemp=float(i), minFitPos=float( 2 * i)))

    tm= TemperatureFocPosModel(showPlot=True,date=None, comment=None, pltFile='test.png', resultFitFwhm=rFF, logger=logger)
    tm.fitData()
    tm.plotData()



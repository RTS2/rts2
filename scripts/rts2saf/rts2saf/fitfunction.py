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

from scipy import optimize

class FitFunction(object):
    """ Fit function to data and find extreme

    :var dataFit: :py:mod:`rts2saf.data.DataFit`
    :var logger:  :py:mod:`rts2saf.log`

    """
    def __init__(self, dataFit=None, logger=None):

        self.dataFit=dataFit
        self.fitFunc=self.dataFit.fitFunc
        self.errorFunc = lambda p, x, y, res, err: (y - self.fitFunc(x, p))
        self.par= self.dataFit.par
        self.recpFunc=self.dataFit.recpFunc
        self.logger=logger
        
    def fitData(self):
        """Fit function using  optimize.leastsq().

        :return extr_focpos, val_func, par, flag:  focuser position, value at position, fit parameters, non zero if successful

        """

        try:
            par, flag  = optimize.leastsq(self.errorFunc, self.par, args=(self.dataFit.pos, self.dataFit.val, self.dataFit.errx, self.dataFit.erry))
        except Exception, e:
            self.logger.error('fitData: failed fitting:\nnumpy error message:\n{0}'.format(e))
            return None, None, None, None

        # ToDo lazy
        # posS=sorted(self.dataFit.pos)
        # step= posS[1]-posS[0]
        extr_focpos=float('nan')
        try:
            if self.recpFunc==None:
                fitFunc=self.fitFunc
            else:
                fitFunc=self.recpFunc

            # extr_focpos = optimize.fminbound(fitFunc,x1=min(self.dataFit.pos)-2 * step, x2=max(self.dataFit.pos)+2 * step, args=[par])
            extr_focpos = optimize.fminbound(fitFunc,x1=min(self.dataFit.pos), x2=max(self.dataFit.pos), args=[par])

        except Exception, e:
            self.logger.error('fitdata: failed finding extreme:\nnumpy error message:\n{0}'.format(e))                
            return None, None, None, None

        val_func= self.fitFunc(x=extr_focpos, p=[x for x in par]) 
        # a decision is done in Analyze
        return extr_focpos, val_func, par, flag


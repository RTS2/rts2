#!/usr/bin/python
# (C) 2010, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rtsaf_model_analyze.py
#   
#   rts2af_model_analyze.py is the nucleus of modelling the
#   temperature dependency of the focus of a telescope.
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

__author__ = 'markus.wildi@one-arcsec.org'


import sys
import time
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
from pylab import *
from scipy import optimize
import collections

fitfunc = lambda p, x: p[0] + p[1]*x # Target function
errfunc = lambda p, x, y: fitfunc(p, x) - y # Distance to the target function


class FitResult():
    """holds the fit results"""
    def __init__(self, p, resid=None, chisq=None, dof=None, rmse=None):
        self.p=p
        self.resid=resid
        self.chisq=chisq
        self.dof=dof
        self.rmse=rmse


class main():
    """fit and plot data"""
    def __init__(self, scriptName='main'):
        self.scriptName= scriptName
        self.nrObjects= 10.
        self.valChi2= 5.
        self.valMinimumFwhm= 4.
        self.valFocPos= 2400. 
#        self.valStartDate= 0. # time.mktime(time.strptime('2009-12-01T00:00:00.0', '%Y-%m-%dT%H:%M:%S.%f'))
        self.valStartDate= time.mktime(time.strptime('2010-12-01T00:00:00.0', '%Y-%m-%dT%H:%M:%S.%f'))
        self.valEndDate= time.mktime(time.strptime('2011-05-31T00:00:00.0', '%Y-%m-%dT%H:%M:%S.%f'))
        print 'valStartDate:{0}'.format(self.valStartDate)
        print 'valEndDate:{0}'.format(self.valEndDate)
        self.aagFocPos = []
        self.issFocPos = []
        self.consFocPos = []
        self.aagTemp= []
        self.issTemp= []
        self.consTemp= []
        self.ll= 000
        self.ul= 4600
        self.llt=-10
        self.ult= 30


    def readData(self):
        with open( 'result.log', 'r') as frfn:
            i= 0
            for line in frfn:
                line.strip()
                
# 0       1        2    3     4  5           6        7
#3.475742 18.997C 19.18 19.32 50 3933.049742 3.453984 1306374986.0 /scratch/focus/2011-05-26T03:56:26.736741/X/20110526015711-255-RA-reference.fits

                items= line.split()
                try:
                    chi2= float(items[0])
                except:
                    continue
                try:
                    objects= float(items[4])
                except:
                    continue
                try:
                    minimumFwhm= float(items[6])
                except:
                    continue
                try:
                    time= float(items[7])
                except:
                    continue
                try:
                    sfocPos= float(items[5])
                except:
                    continue
                try:
                    date= float(items[7])
                except:
                    continue

                if((chi2 < self.valChi2) and ( chi2 >0.) and (objects > self.nrObjects) and (minimumFwhm < self.valMinimumFwhm) and (sfocPos > self.valFocPos) and (( date > self.valStartDate) and ( date < self.valEndDate))):

                    if((float(items[5]) < self.ll) or (float(items[5]) > self.ul)):
##                            print '=======================discarding focus pos: {0}'.format(line)
                        continue

                    aagT=0.
                    try:
                        aagT= float(items[1].replace('C', ''))
                        self.aagFocPos.append(sfocPos)
                        self.aagTemp.append(aagT)
                    except:
                        print '======================= aagTemp NoTemp: {0}'.format(line)
                        pass


                    self.consFocPos.append(sfocPos)
                    self.consTemp.append(float(items[2]))
                    self.issFocPos.append(sfocPos)
                    self.issTemp.append(float(items[3]))

                    if((sfocPos < self.ll) or (sfocPos > self.ul)):
                        print '===================================== {0} {1} {2} {3}'.format(self.focPos[i], self.aagTemp[i], self.consTemp[i],  self.issTemp[i])
                        #print '----{0}, {1}, {2} {3}'.format(i, self.focPos[i], float(items[6]), self.issTemp[i])
                    i += 1
                else:
                    line.strip()
                    line.replace( '\n', '')
                    if( not ((chi2 < self.valChi2) and ( chi2 >0.))):
                        print 'chi2 discarding: {0}'.format(chi2)
                        
                    if( not (objects > self.nrObjects)):
                        print 'objects discarding: {0}'.format(objects)
                        
                    if( not (minimumFwhm < self.valMinimumFwhm)):
                        print 'FWHM discarding: {0}'.format(minimumFwhm)
                    if( not (sfocPos > self.valFocPos)):
                        print 'focuser position discarding: {0}'.format(sfocPos)
                        
                    if(not (( date > self.valStartDate) and( date < self.valEndDate))):
                        print 'date discarding: {0}'.format(date)

                #    print 'discarding: {0}'.format(line)
        frfn.close()
        return i


    def fillArray(self, listTmp=None, arrayTmp=None):
        i=0
        for val in listTmp:
           arrayTmp[i]=val
           i += 1
# http://stackoverflow.com/questions/4520785/how-to-get-rmse-from-scipy-optimize-leastsq-module
    def fit(self, focPosNP=None, tempNP=None):

        Param=collections.namedtuple('Param','p0 p1')
        p_guess=Param(p0=0.,p1=1.)
        p,cov,infodict,mesg,ier = optimize.leastsq(errfunc, p_guess, args=( focPosNP, tempNP), full_output=1, warning=True )
        p=Param(*p)
        #resid=errfunc(p,self.issFocPosNP,self.issTempNP)
        #print(resid)
        #print(infodict['fvec'])
        resid= infodict['fvec']
        chisq=(infodict['fvec']**2).sum()
        dof=len(focPosNP)-len(p)
        rmse=np.sqrt(chisq/dof)
        print(rmse)
        
        return   FitResult(p, resid, chisq, dof, rmse)


    def main(self):
        number= self.readData()

        fig01 = plt.figure(1, figsize=(17.5,11.0))
        print '================= length {0} {1} {2}'.format(len(self.aagFocPos), len(self.issFocPos), len(self.consFocPos))

        title= "Temperature model for AP Astrofire F/7, objs>{0}, chi2<{1}, minFwhm<{2}".format(self.nrObjects, self.valChi2, self.valMinimumFwhm)
        fig01.suptitle(title)
        ax0 = fig01.add_subplot(2, 3, 1)

#        ax0.scatter(self.issFocPos, self.issTemp)
        ax0.set_title('Davis outdoor: iss, runs: {0}'.format(len(self.issFocPos)))
        ax0.set_xlabel('FOC_POS [ticks]')
        ax0.set_ylabel('Temperature [C]')
        ax0.grid(True,linestyle='-',color='0.35')
#        ax0.axis([self.ll,self.ul,self.llt,self.ult])

        ax1 = fig01.add_subplot(2, 3, 2)
        ax1.scatter(self.consFocPos, self.consTemp)
        ax1.set_title('Davis cupola: cons, runs: {0}'.format(len(self.consFocPos)))
        ax1.set_xlabel('FOC_POS [ticks]')
        ax1.set_ylabel('Temperature [C]')
        ax1.grid(True,linestyle='-',color='0.35')
#        ax1.axis([self.ll,self.ul,self.llt,self.ult])
        
        ax2 = fig01.add_subplot(2, 3, 3)
#        ax2.scatter(self.aagFocPos, self.aagTemp)
        ax2.set_title('AAG cloud sensor: irs, runs: {0}'.format(len(self.aagFocPos)))
        ax2.set_xlabel('FOC_POS [ticks]')
        ax2.set_ylabel('Temperature [C]')
        ax2.grid(True,linestyle='-',color='0.35')
#        ax2.axis([self.ll,self.ul,self.llt,self.ult])


        ax4 = fig01.add_subplot(2, 3, 4)
        ax4.set_title('Davis outdoor: residuals')
        ax4.set_xlabel('residual [a.u.]')
        ax4.set_ylabel('[a.u.]')
        ax4.grid(True,linestyle='-',color='0.35')

        ax5 = fig01.add_subplot(2, 3, 5)
        ax5.set_title('Davis cupola: residuals')
        ax5.set_xlabel('residual [a.u.]')
        ax5.set_ylabel('[a.u.]')
        ax5.grid(True,linestyle='-',color='0.35')

        ax6 = fig01.add_subplot(2, 3, 6)
        ax6.set_title('AAG cloud sensor: residuals')
        ax6.set_xlabel('residual [a.u.]')
        ax6.set_ylabel('[a.u.]')
        ax6.grid(True,linestyle='-',color='0.35')


        self.issFocPosNP= np.arange(len(self.issFocPos), dtype=float)
        self.issTempNP  = np.arange(len(self.issTemp), dtype=float)

        self.consFocPosNP= np.arange(len(self.consFocPos), dtype=float)
        self.consTempNP= np.arange(len(self.consTemp), dtype=float)

        self.aagFocPosNP= np.arange(len(self.aagFocPos), dtype=float)
        self.aagTempNP  = np.arange(len(self.aagTemp), dtype=float)


        self.fillArray(self.issFocPos, self.issFocPosNP)
        self.fillArray(self.issTemp, self.issTempNP)

        self.fillArray(self.consFocPos, self.consFocPosNP)
        self.fillArray(self.consTemp, self.consTempNP)

        self.fillArray(self.aagFocPos, self.aagFocPosNP)
        self.fillArray(self.aagTemp, self.aagTempNP)

        
        result0= self.fit(self.issFocPosNP, self.issTempNP)
        time = linspace(self.issFocPosNP.min(), self.issFocPosNP.max(), 100)
        ax0.annotate('p0={0:.5}, p1={1:.3}'.format(result0.p[0], result0.p[1]), xy=(2800., 15.), xytext=(2800, 15.))
        ax0.plot(self.issFocPosNP, self.issTempNP, "ro", time, fitfunc(result0.p, time), "r-") # Plot of the data and the fit

        result1= self.fit(self.consFocPosNP, self.consTempNP)
        time = linspace(self.consFocPosNP.min(), self.consFocPosNP.max(), 100)
        ax1.annotate('p0={0:.5}, p1={1:.3}'.format(result1.p[0], result1.p[1]), xy=(2800., 20.), xytext=(2800, 20.))
        ax1.plot(self.consFocPosNP, self.consTempNP, "ro", time, fitfunc(result1.p, time), "r-") 

        result2= self.fit(self.aagFocPosNP, self.aagTempNP)
        time = linspace(self.aagFocPosNP.min(), self.aagFocPosNP.max(), 100)
        ax2.annotate('p0={0:.5}, p1={1:.3}'.format(result2.p[0], result2.p[1]), xy=(2800., 15.), xytext=(2800, 15.))
        ax2.plot(self.aagFocPosNP, self.aagTempNP, "ro", time, fitfunc(result2.p, time), "r-") 


        print 'focuser position minimum: {0}, maximum: {1}'.format(self.issFocPosNP.min(), self.issFocPosNP.max())
        print 'aag temperature  minimum: {0}, maximum: {1}'.format(self.aagTempNP.min(), self.aagTempNP.max())
        print 'iss temperature  minimum: {0}, maximum: {1}'.format(self.issTempNP.min(), self.issTempNP.max())
        print 'cons temperature minimum: {0}, maximum: {1}'.format(self.consTempNP.min(), self.consTempNP.max())

        print 'Davis iss : parameters: p0={0}, p1={1}'.format(result0.p[0], result0.p[1])
        print 'Davis cons: parameters: p0={0}, p1={1}'.format(result1.p[0], result1.p[1])
        print 'AAG irs   : parameters: p0={0}, p1={1}'.format(result2.p[0], result2.p[1])

        n, bins, patches = ax4.hist(result0.resid, 12, normed=1, facecolor='blue', alpha=0.75)
        n, bins, patches = ax5.hist(result1.resid, 12, normed=1, facecolor='blue', alpha=0.75)
        n, bins, patches = ax6.hist(result2.resid, 12, normed=1, facecolor='blue', alpha=0.75)

        
        
        plt.show()
if __name__ == '__main__':
    main(sys.argv[0]).main()

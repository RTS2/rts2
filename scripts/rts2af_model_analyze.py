#!/usr/bin/python
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
from optparse import OptionParser

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
    def __init__(self, scriptName='main', debug=None, fileName=None, startDate=None, endDate=None, minNumberOfObjects=None, maxNumberOfObjects=None, lowerChi2=None, upperChi2=None, minFwhmLower=None, minFwhmUpper=None):
        self.scriptName= scriptName
        self.fileName= fileName
        self.minNrObjects= minNumberOfObjects
        self.maxNrObjects= maxNumberOfObjects
        self.lowerChi2= lowerChi2
        self.upperChi2= upperChi2
        self.minimumFwhmUpper= minFwhmLower
        self.minimumFwhmLower= minFwhmUpper
        self.valStartDate= time.mktime(time.strptime(startDate, '%Y-%m-%dT%H:%M:%S.%f'))
        self.valEndDate= time.mktime(time.strptime(endDate, '%Y-%m-%dT%H:%M:%S.%f'))
        self.debug= debug
        self.FocPos = []
        self.Temp= []
        self.ll= 0
        self.ul= 10000000


    def readData(self):
        with open( self.fileName, 'r') as frfn:
            i= 0
            for line in frfn:
                line.strip()
# 0       1        2  3           4        5            6
#3.475742 18.997   50 3933.049742 3.453984 1306374986.0 /scratch/focus/2011-05-26T03:56:26.736741/X/20110526015711-255-RA-reference.fits

                items= line.split(',')
                try:
                    chi2= float(items[0])
                except:
                    continue
                try:
                    objects= float(items[2])
                except:
                    continue
                try:
                    minimumFwhm= float(items[4])
                except:
                    continue
                try:
                    sfocPos= float(items[3])
                except:
                    continue
                try:
                    date= float(items[5])
                except:
                    continue

                if((chi2 < self.upperChi2) and ( chi2 > self.lowerChi2)  and ((objects > self.minNrObjects) and(objects < self.maxNrObjects)) and (minimumFwhm > self.minimumFwhmUpper) and (minimumFwhm < self.minimumFwhmLower) and (( date > self.valStartDate) and ( date < self.valEndDate))):

                    if((float(items[3]) < self.ll) or (float(items[3]) > self.ul)):
                        continue

                    self.FocPos.append(sfocPos)
                    self.Temp.append(float(items[1]))

                    if((sfocPos < self.ll) or (sfocPos > self.ul)):
                        print 'Out of focuser bounds: {0} {1} {2} {3}'.format(self.focPos[i], self.aagTemp[i], self.Temp[i],  self.issTemp[i])
                        #print '----{0}, {1}, {2} {3}'.format(i, self.focPos[i], float(items[4]), self.issTemp[i])
                    i += 1
                else:
                    if( self.debug):
                        line.strip()
                        line.replace( '\n', '')
                        if( not ((chi2 < self.upperChi2) and ( chi2 > self.lowerChi2))):
                            print 'chi2 discarding: {0}, {1}'.format(chi2, line)
                        
                        if( not (objects > self.nrObjects)):
                            print 'objects discarding: {0}, {1}'.format(objects, line)
                        
                        if( not (minimumFwhm < self.minimumFwhmUpper)):
                            print 'FWHM Upper discarding: {0}, {1}'.format(minimumFwhm,line)

                        if( not (minimumFwhm > self.minimumFwhmLower)):
                            print 'FWHM Lower discarding: {0}, {1}'.format(minimumFwhm, line)
                        
                        if(not (( date > self.valStartDate) and( date < self.valEndDate))):
                            print 'date discarding: {0} {1}'.format(date, line)

        frfn.close()
        return i


    def fillArray(self, listTmp=None, arrayTmp=None):
        i=0
        for val in listTmp:
           arrayTmp[i]=val
           i += 1
# http://stackoverflow.com/questions/4520785/how-to-get-rmse-from-scipy-optimize-leastsq-module
    def fit(self, tempNP=None, focPosNP=None):

        Param=collections.namedtuple('Param','p0 p1')
        p_guess=Param(p0=1.0,p1=1.)
        p,cov,infodict,mesg,ier = optimize.leastsq(errfunc, p_guess, args=( tempNP, focPosNP), full_output=1, warning=True )
        p=Param(*p)

        resid= infodict['fvec']
        chisq=(infodict['fvec']**2).sum()
        dof=len(focPosNP)-len(p)
        rmse=np.sqrt(chisq/dof)
        return   FitResult(p, resid, chisq, dof, rmse)


    def main(self):
        points= self.readData()

        fig01 = plt.figure(1, figsize=(17.5, 5.5))

        title= "Temperature model for AP Astrofire F/7, {0}<objs<{1}, {2}<chi2<{3}, {4}<minFwhm<{5}, points: {6}".format(self.minNrObjects, self.maxNrObjects, self.lowerChi2, self.upperChi2, self.minimumFwhmUpper, self.minimumFwhmLower, len(self.FocPos))
        fig01.suptitle(title)

        ax1 = fig01.add_subplot(1, 3, 1)
        ax1.scatter(self.Temp, self.FocPos)
        ax1.set_xlabel('Temperature [degC]')
        ax1.set_ylabel('FOC_POS [ticks]')
        ax1.grid(True,linestyle='-',color='0.35')
#        ax1.axis([-5,28., 2600,4500,])
        
        ax2 = fig01.add_subplot(1, 3, 2)
        ax2.set_xlabel('residuals temperature [degC]')
        ax2.set_ylabel('[a.u.]')
        ax2.grid(True,linestyle='-',color='0.35')
        #ax2.axis([-200., 200., 0., .008])

        ax3 = fig01.add_subplot(1, 3, 3)
        ax3.set_xlabel('residuals FOC_POS [ticks]')
        ax3.grid(True,linestyle='-',color='0.35')
        ax3.set_ylabel('[a.u.]')

        self.FocPosNP= np.arange(len(self.FocPos), dtype=float)
        self.TempNP= np.arange(len(self.Temp), dtype=float)

        self.fillArray(self.FocPos, self.FocPosNP)
        self.fillArray(self.Temp, self.TempNP)

        result= self.fit( self.TempNP, self.FocPosNP)
        temp = linspace(self.TempNP.min(), self.TempNP.max(), 100)
        ax1.annotate('p0={0:.4}, p1={1:.3}'.format(result.p[0], result.p[1]), xy=( -4., 4000), xytext=( -4., 4000.))
        ax1.plot(self.TempNP,  self.FocPosNP, "ro", temp, fitfunc(result.p, temp), "r-") 

        n, bins, patches = ax2.hist(result.resid/ result.p[1], 20., normed=1, facecolor='blue', alpha=0.75)
        n, bins, patches = ax3.hist(result.resid, 10, normed=1, facecolor='blue', alpha=0.75)

        print 'Temperature std.: {0}, mean:{1}'.format(np.std(result.resid/ result.p[1]) * 2.35, np.mean(result.resid/ result.p[1]))
        print 'FWHM        std.: {0}, mean:{1}'.format(np.std(result.resid) *2.35, np.mean(result.resid))
        print 'RMSe            : {0}, chi2:{1}, dof: {2}'.format(result.rmse, result.chisq, result.dof)
        print 'P0              : {0}, P1:{1}'.format(result.p[0], result.p[1])
        print 'Data points     : {0}'.format(points)

        plt.show()

if __name__ == '__main__':

    debug=None
    fileName=None
    startDate=None
    endDate=None
    minNumberOfObjects=None
    maxNumberOfObjects=None
    lowerChi2=None
    upperChi2=None
    minFwhmLower=None
    minFwhmUpper=None

    parser = OptionParser()
    parser.add_option('--debug',help='add more output',action='store_true',default=False)
    parser.add_option('--fit-results',help='file with results (FWHM, temperature)',action='store',dest='fileName',default='result1.log')
    parser.add_option('--start-date',help='analyze after',action='store',dest='startDate',default='2010-12-01T00:00:00.0')
    parser.add_option('--end-date',help='analyze before',action='store',dest='endDate',default='2099-07-31T00:00:00.0')
    parser.add_option('--min-objects',help='min. number of objects',action='store',dest='minNumberOfObjects',default=5)
    parser.add_option('--max-objects',help='max. number of objects',action='store',dest='maxNumberOfObjects',default=5000)
    parser.add_option('--min-chi2',help='minimum of chi2',action='store',dest='lowerChi2',default=.1)
    parser.add_option('--max-chi2',help='maximum of chi2',action='store',dest='upperChi2',default=75.)
    parser.add_option('--min-fwhm-lower',help='analyze above lower FWHM boundary',action='store',dest='minFwhmLower',default=0.5)
    parser.add_option('--min-fwhm-upper',help='analyze below upper FWHM boundary',action='store',dest='minFwhmUpper',default=4.)

    (options,args) = parser.parse_args()

    if(options.debug):
        debug=True
    else:
        debug= False

    if(options.fileName):
        fileName=options.fileName

    if(options.startDate):
        startDate=options.startDate

    if(options.endDate):
        endDate=options.endDate

    if(options.minNumberOfObjects):
        minNumberOfObjects=options.minNumberOfObjects

    if(options.maxNumberOfObjects):
        maxNumberOfObjects=options.maxNumberOfObjects

    if(options.lowerChi2):
        lowerChi2=options.lowerChi2

    if(options.upperChi2):
        upperChi2=options.upperChi2

    if(options.minFwhmLower):
        minFwhmLower=options.minFwhmLower

    if(options.minFwhmUpper):
        minFwhmUpper=options.minFwhmUpper

    main(sys.argv[0], debug, fileName, startDate, endDate, minNumberOfObjects, maxNumberOfObjects, lowerChi2, upperChi2, minFwhmLower, minFwhmUpper).main()
 

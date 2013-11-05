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
import numpy as np
import copy

from ds9 import *

#import rts2saf.data as dt
#import rts2saf.ds9region as ds9r
#import rts2saf.fitfwhm as ft

from rts2saf.fitfunction import  FitFunction
from rts2saf.fitdisplay import FitDisplay
from rts2saf.data import DataFit,ResultFit


class SimpleAnalysis(object):
    """SimpleAnalysis a set of FITS"""
    def __init__(self, debug=False, dataSex=None, Ds9Display=False, FitDisplay=False, ftwName=None, ftName=None, dryFits=False, focRes=None, ev=None, logger=None):
        self.debug=debug
        self.dataSex=dataSex
        self.Ds9Display=Ds9Display
        self.FitDisplay=FitDisplay
        self.ftwName=ftwName
        self.ftName=ftName
        self.dryFits=dryFits
        self.focRes=focRes
        self.ev=ev
        self.logger=logger
        self.dataFitFwhm=None
        self.resultFitFwhm=None
        # ToDo must reside outside
        self.parFwhm= np.array([1., 1., 1., 1.])
        self.ffwhm = lambda x, p: p[0] + p[1] * x + p[2] * (x ** 2)+ p[3] * (x ** 4)  # due to optimize.fminbound
        #
        self.dataFitFlux=None
        self.resultFitFlux=None
        # ToDo must reside outside
        self.parFlux= np.array([1., 1., 1., 1.])
        self.fflux = lambda x, p: p[3] + p[0]*np.exp(-(x-p[1])**2/(2*p[2]**2))
        self.recpFlux = lambda x, p: 1./(p[3] + p[0]*np.exp(-(x-p[1])**2/(2*p[2]**2)))
        self.fd=None

    def __weightedMeansFwhm(self):
        nObjsC  = self.dataFitFwhm.nObjs[:]
        posC    = self.dataFitFwhm.pos[:]
        fwhmC   = self.dataFitFwhm.val[:]
        stdFwhmC= self.dataFitFwhm.erry[:]
        while True:
            try:
                ind=fwhmC.index(0.)
            except:
                break
            del nObjsC[ind] # not strictly necessary
            del posC[ind]
            del fwhmC[ind]
            del stdFwhmC[ind]

        # Weighted mean based on number of extracted objects (stars)
        weightedMeanObjects=None
        try:
            weightedMeanObjects= np.average(a=dFwhm.pos, axis=0, weights=nObjsC)
        except Exception, e:
            self.logger.warn('analyze: can not calculate weightedMeanObjects:\n{0}'.format(e))

        try:
            self.logger.info('analyze: FOC_DEF: {0:5d} : weighted mean derived from sextracted objects'.format(int(weightedMeanObjects)))
        except Exception, e:
            self.logger.warn('analyze: can not convert weightedMeanObjects:\n{0}'.format(e))
        # Weighted mean based on median FWHM
        weightedMeanFwhm=None
        try:
            weightedMeanFwhm= np.average(a=posC, axis=0, weights=map( lambda x: 1./x, fwhmC)) 
        except Exception, e:
            self.logger.warn('analyze: can not calculate weightedMeanFwhm:\n{0}'.format(e))

        try:
            self.logger.info('analyze: FOC_DEF: {0:5d} : weighted mean derived from FWHM'.format(int(weightedMeanFwhm)))
        except Exception, e:
            self.logger.warn('analyze: can not convert weightedMeanFwhm:\n{0}'.format(e))
        # Weighted mean based on median std(FWHM)
        weightedMeanStdFwhm=None
        try:
            weightedMeanStdFwhm= np.average(a=posC, axis=0, weights=map( lambda x: 1./x, stdFwhmC)) 
        except Exception, e:
            self.logger.warn('analyze: can not calculate weightedMeanStdFwhm:\n{0}'.format(e))

        try:
            self.logger.info('analyze: FOC_DEF: {0:5d} : weighted mean derived from std(FWHM)'.format(int(weightedMeanStdFwhm)))
        except Exception, e:
            self.logger.warn('analyze: can not convert weightedMeanStdFwhm:\n{0}'.format(e))
        # Weighted mean based on a combination of variables
        weightedMeanCombined=None
        combined=list()
        for i, v in enumerate(nObjsC):
            combined.append( nObjsC[i]/(stdFwhmC[i] * fwhmC[i]))

        try:
            weightedMeanCombined= np.average(a=posC, axis=0, weights=combined)
        except Exception, e:
            self.logger.warn('analyze: can not calculate weightedMeanCombined:\n{0}'.format(e))

        try:
            self.logger.info('analyze: FOC_DEF: {0:5d} : weighted mean derived from Combined'.format(int(weightedMeanCombined)))
        except Exception, e:
            self.logger.warn('analyze: can not convert weightedMeanCombined:\n{0}'.format(e))


    def __fitFwhm(self):
        # ToDo make an option
        comment=None
        if self.dryFits:
            comment='dryFits'
        
        minFitPos, minFitFwhm, fitPar, fitFlag= FitFunction(
            dataFit=self.dataFitFwhm, 
            fitFunc=self.ffwhm, 
            logger=self.logger, 
            par=self.parFwhm).fitData()

        if minFitPos:
            self.logger.info('analyze: FOC_DEF: {0:5d} : fitted minimum position, {1:4.1f}px FWHM, {2} ambient temperature'.format(int(minFitPos), minFitFwhm, self.dataFitFwhm.ambientTemp))
        else:
            self.logger.warn('analyze: fit failed')

        self.resultFitFwhm=ResultFit(
            ambientTemp=self.dataFitFwhm.ambientTemp, 
            ftName=self.dataFitFwhm.ftName,
            extrFitPos=minFitPos, 
            extrFitVal=minFitFwhm, 
            fitPar=fitPar,
            fitFlag=fitFlag,
            color='blue',
            ylabel='FWHM [px]: blue'
            )

        self.date='2013'
        self.fd = FitDisplay(date=self.date, comment='unittest', logger=self.logger)
        self.fd.fitDisplay(dataFit=self.dataFitFwhm, resultFit=self.resultFitFwhm, fitFunc=self.ffwhm)


    def __fitFlux(self):
        i_flux = self.dataSex[0].fields.index('FLUX_MAX')
        # very ugly
        pos=list()
        flux=list()
        errx=list()
        stdFlux=list()
        nObjs=list()
        for dSx in self.dataSex:
            fluxv = map(lambda x:x[i_flux], dSx.catalog)
            dSx.flux=numpy.median(fluxv)
            dSx.stdFlux= numpy.std(flux)
            pos.append(dSx.focPos)
            flux.append(dSx.flux)
            errx.append(self.focRes)
            stdFlux.append(dSx.stdFlux/10.) #ToDO


        bPth,fn=os.path.split(self.dataSex[0].fitsFn)
        ftName=self.dataSex[0].ftName
        plotFn=self.ev.expandToPlotFileName(plotFn='{0}/min-fwhm-{1}.png'.format(bPth,ftName))
        self.dataFitFlux= DataFit(
            plotFn=plotFn,
            ambientTemp=self.dataSex[0].ambientTemp, 
            ftName=self.dataSex[0].ftName, 
            pos=np.asarray(pos),
            val=np.asarray(flux),
            errx=np.asarray(errx),
            erry=np.asarray(stdFlux), 
            nObjs=np.asarray(nObjs))
        x=np.array([p for p in self.dataFitFlux.pos])
        y=np.array([v for v in self.dataFitFlux.val])

        wmean= np.average(a=x, weights=y) 
        xy= zip(x,y)
        wstd = np.std(a=xy) 
        # scale the values [a.u.]
        sv = [max(self.dataFitFwhm.val) / max(self.dataFitFlux.val) * x for x in self.dataFitFlux.val]
        sstd = [max(self.dataFitFwhm.val) / max(self.dataFitFlux.val) * x for x in self.dataFitFlux.erry]
        self.dataFitFlux.val=sv
        self.dataFitFlux.erry=sstd
        # gaussian
        self.parFlux= np.array([ 10., wmean, wstd/40., 2.])
        maxFitPos, maxFitFlux, fitPar, fitFlag= FitFunction(
            dataFit=self.dataFitFlux, 
            fitFunc=self.fflux, 
            recpFunc=self.recpFlux,
            logger=self.logger, 
            par=self.parFlux).fitData()

        if fitFlag:
            self.logger.info('analyze: FOC_DEF: {0:5d} : fitted maximum position, {1:4.1f}px Flux, {2} ambient temperature'.format(int(maxFitPos), maxFitFlux, self.dataFitFlux.ambientTemp))
        else:
            self.logger.warn('analyze: fit flux failed')

        self.resultFitFlux=ResultFit(
            ambientTemp=self.dataFitFlux.ambientTemp, 
            ftName=self.dataFitFlux.ftName,
            extrFitPos=maxFitPos, 
            extrFitVal=maxFitFlux, 
            fitPar=fitPar,
            fitFlag=fitFlag,
            color='red',
            ylabel='FWHM [px]: blue Flux [a.u.]: red'
            )

        self.date='2013'
        #fd = FitDisplay(date=self.date, comment='unittest', logger=self.logger)
        self.fd.fitDisplay(dataFit=self.dataFitFlux, resultFit=self.resultFitFlux, fitFunc=self.fflux, display=True)

    def analyze(self):
        # ToDo lazy                        !!!!!!!!!!
        # create an average and std 
        # ToDo decide wich ftName from which ftw!!
        bPth,fn=os.path.split(self.dataSex[0].fitsFn)
        ftName=self.dataSex[0].ftName
        plotFn=self.ev.expandToPlotFileName(plotFn='{0}/min-fwhm-{1}.png'.format(bPth,ftName))
        if self.FitDisplay:
            self.logger.info('analyze: storing plot file: {0}'.format(plotFn))

        # very ugly
        pos=list()
        fwhm=list()
        errx=list()
        stdFwhm=list()
        nObjs=list()
        for dSx in self.dataSex:
            # all sextracted objects
            no= len(dSx.catalog)
            if self.debug: self.logger.debug('analyze: {0:5.0f}, sextracted objects: {1:5d}, filtered: {2:5d}'.format(dSx.focPos, no, dSx.nstars))
            #
            pos.append(dSx.focPos)
            fwhm.append(dSx.fwhm)
            errx.append(self.focRes)
            stdFwhm.append(dSx.stdFwhm)
            nObjs.append(len(dSx.catalog))

        self.dataFitFwhm= DataFit(
            plotFn=plotFn,
            ambientTemp=self.dataSex[0].ambientTemp, 
            ftName=self.dataSex[0].ftName, 
            pos=np.asarray(pos),
            val=np.asarray(fwhm),
            errx=np.asarray(errx),
            erry=np.asarray(stdFwhm), 
            nObjs=np.asarray(nObjs))

        self.__weightedMeansFwhm()
        self.__fitFwhm()
        self.__fitFlux()

        return self.resultFitFwhm


    def display(self):
        # ToDo ugly here
        DISPLAY=False
        if self.FitDisplay or self.Ds9Display:
            try:
                os.environ['DISPLAY']
                DISPLAY=True
            except:
                self.logger.warn('analyze: no X-Window DISPLAY, do not plot with mathplotlib and/or ds9')

        if self.FitDisplay:
            ft=FitDisplay(date=None, logger=self.logger)
            ft.fitDisplay(dataFit=self.dataFitFwhm, resultFit=self.resultFitFwhm, fitFunc=self.ffwhm, display=DISPLAY)

            # plot them through ds9
        if self.Ds9Display:
            try:
                dds9=ds9()
            except Exception, e:
                self.logger.error('analyze: OOOOOOOOPS, no ds9 display available')
                return 

            # ToDo cretae new list
            self.dataSex.sort(key=lambda x: int(x.focPos))

            for dSx in self.dataSex:
                if dSx.fitsFn:
                    dr=ds9r.Ds9Region( dataSex=dSx, display=dds9, logger=self.logger)
                    if not dr.displayWithRegion():
                        break # something went wrong
                    time.sleep(1.)
                else:
                    self.logger.warn('analyze: OOOOOOOOPS, no file name for fits image number: {0:3d}'.format(dSx.fitsFn))

import numpy
from itertools import ifilterfalse
from itertools import ifilter
# ToDo at the moment this method is an demonstrator
class CatalogAnalysis(object):
    """CatalogAnalysis a set of FITS"""
    def __init__(self, debug=False, dataSex=None, Ds9Display=False, FitDisplay=False, ftwName=None, ftName=None, dryFits=False, focRes=None, moduleName=None, ev=None, rt=None, logger=None):
        self.debug=debug
        self.dataSex=dataSex
        self.Ds9Display=Ds9Display
        self.FitDisplay=FitDisplay
        self.ftwName=ftwName
        self.ftName=ftName
        self.dryFits=dryFits
        self.focRes=focRes
        self.moduleName=moduleName
        self.ev=ev
        self.rt=rt
        self.logger=logger
        self.criteriaModule=None
        self.cr=None


    def __loadCriteria(self):
        # http://stackoverflow.com/questions/951124/dynamic-loading-of-python-modules
        # Giorgio Gelardi ["*"]!
        self.criteriaModule=__import__(self.moduleName, fromlist=["*"])
        self.cr=self.criteriaModule.Criteria(dataSex=self.dataSex, rt=self.rt)

    def selectAndAnalyze(self):

        self.__loadCriteria()
        # ToDo glitch
        i_f = self.dataSex[0].fields.index('FWHM_IMAGE')
        acceptedDataSex=list()
        rejectedDataSex=list()
        for dSx in self.dataSex:
            adSx=copy.deepcopy(dSx)
            acceptedDataSex.append(adSx)
            adSx.catalog= list(ifilter(self.cr.decide, dSx.catalog))
            nsFwhm=np.asarray(map(lambda x: x[i_f], adSx.catalog))
            adSx.fwhm=numpy.median(nsFwhm)
            adSx.stdFwhm=numpy.std(nsFwhm)

            rdSx=copy.deepcopy(dSx)
            rejectedDataSex.append(rdSx)
            rdSx.catalog=  list(ifilterfalse(self.cr.decide, dSx.catalog))

            nsFwhm=np.asarray(map(lambda x: x[i_f], rdSx.catalog))
            rdSx.fwhm=numpy.median(nsFwhm)
            rdSx.stdFwhm=numpy.std(nsFwhm)

        # 
        an=SimpleAnalysis(debug=self.debug, dataSex=acceptedDataSex, Ds9Display=self.Ds9Display, FitDisplay=self.FitDisplay, focRes=self.focRes, ev=self.ev, logger=self.logger)
        accRFt=an.analyze()
        try:
            self.logger.debug( 'ACCEPTED: weightedMeanObjects: {0:5.1f}, weightedMeanCombined: {1:5.1f}, minFitPos: {2:5.1f}, minFitFwhm: {0:5.1f}'.format(accRFt.weightedMeanObjects, accRFt.weightedMeanCombined, accRFt.minFitPos, accRFt.minFitFwhm))
        except:
            self.logger.debug('ACCEPTED: fit and calculation failed')
            
        if self.Ds9Display or self.FitDisplay:
            an.display()
        #
        an=SimpleAnalysis(debug=self.debug, dataSex=rejectedDataSex, Ds9Display=self.Ds9Display, FitDisplay=self.FitDisplay, focRes=self.focRes, ev=self.ev, logger=self.logger)
        rejRFt=an.analyze()
        try:
            self.logger.debug( 'REJECTED: weightedMeanObjects: {0:5.1f}, weightedMeanCombined: {1:5.1f}, minFitPos: {2:5.1f}, minFitFwhm: {3:5.1f}'.format(rejRFt.weightedMeanObjects, rejRFt.weightedMeanCombined, rejRFt.minFitPos, rejRFt.minFitFwhm))
        except:
            self.logger.debug('REJECTED: fit and calculation failed')

        if self.Ds9Display or self.FitDisplay:
            an.display()
        # 
        an=SimpleAnalysis(debug=self.debug, dataSex=self.dataSex, Ds9Display=self.Ds9Display, FitDisplay=self.FitDisplay, focRes=self.focRes, ev=self.ev, logger=self.logger)
        allrFt=an.analyze()
        try:
            self.logger.debug( 'ALL    : weightedMeanObjects: {0:5.1f}, weightedMeanCombined: {1:5.1f}, minFitPos: {2:5.1f}, minFitFwhm: {3:5.1f}'.format(allRFt.weightedMeanObjects, allRFt.weightedMeanCombined, allRFt.minFitPos, allRFt.minFitFwhm))
        except:
            self.logger.debug('ALL     : fit and calculation failed')


        if self.Ds9Display or self.FitDisplay:
            an.display()
        # ToDo here are three objects
        return accRFt, rejRFt, allrFt

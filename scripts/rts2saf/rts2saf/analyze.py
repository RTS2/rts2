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
from rts2saf.data import DataFitFwhm,DataFitFlux,ResultFit


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
        #
        self.dataFitFlux=None
        self.resultFitFlux=None
        # ToDo must reside outside
        self.fd=None
        self.i_flux=None

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

        minFitPos, minFitFwhm, fitPar, fitFlag= FitFunction(
            dataFit=self.dataFitFwhm, 
            logger=self.logger, 
            ).fitData()

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
            ylabel='FWHM [px]: blue',
            titleResult='fwhm:{0:5d}'.format(int(minFitPos))
            )

    def __fitFlux(self):

        maxFitPos, maxFitFlux, fitPar, fitFlag= FitFunction(
            dataFit=self.dataFitFlux, 
            logger=self.logger, 
            ).fitData()

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
            ylabel='FWHM [px]: blue Flux [a.u.]: red',
            titleResult='fwhm:{0:5d}, flux: {1:5d}'.format(int(self.resultFitFwhm.extrFitPos), int(maxFitPos))
            )

    def analyze(self):
        # ToDo lazy                        !!!!!!!!!!
        # create an average and std 
        # ToDo decide wich ftName from which ftw!!
        bPth,fn=os.path.split(self.dataSex[0].fitsFn)
        ftName=self.dataSex[0].ftName
        plotFn=self.ev.expandToPlotFileName(plotFn='{0}/min-fwhm-{1}.png'.format(bPth,ftName))

        self.logger.info('analyze: storing plot file: {0}'.format(plotFn))

        # fwhm
        self.dataFitFwhm= DataFitFwhm(
            dataSex=self.dataSex,
            plotFn=plotFn,
            ambientTemp=self.dataSex[0].ambientTemp, 
            ftName=self.dataSex[0].ftName,
            )
        self.__fitFwhm()

        try:
            self.i_flux = self.dataSex[0].fields.index('FLUX_MAX')
        except:
            pass

        if self.i_flux!=None:
            self.dataFitFlux= DataFitFlux(
                dataSex=self.dataSex,
                dataFitFwhm=self.dataFitFwhm,
                plotFn=plotFn,
                ambientTemp=self.dataSex[0].ambientTemp, 
                ftName=self.dataSex[0].ftName 
                )

            self.__fitFlux()

        # 
        self.__weightedMeansFwhm()
        # ToDo
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

        
        ft=FitDisplay(date=None, logger=self.logger)

        if self.i_flux==None:
            ft.fitDisplay(dataFit=self.dataFitFwhm, resultFit=self.resultFitFwhm, display=self.FitDisplay)
        else:
            # plot but don't show
            ft.fitDisplay(dataFit=self.dataFitFwhm, resultFit=self.resultFitFwhm, display=False)
            ft.fitDisplay(dataFit=self.dataFitFlux, resultFit=self.resultFitFlux, display=self.FitDisplay)

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

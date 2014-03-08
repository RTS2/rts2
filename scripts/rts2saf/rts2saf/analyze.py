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

"""Analysis defines the extremes of FWHM and optionally flux. 
"""

__author__ = 'markus.wildi@bluewin.ch'

import os
import psutil
import numpy as np
import copy

from ds9 import *


from rts2saf.fitfunction import  FitFunction
from rts2saf.fitdisplay import FitDisplay
from rts2saf.data import DataFitFwhm,DataFitFlux,ResultFit, ResultMeans
from rts2saf.ds9region import Ds9Region

class SimpleAnalysis(object):
    """Analysis of extremes of FWHM and optionally of flux.

    :var debug: enable more debug output with --debug and --level
    :var date: date string as it appears on plot 
    :var dataSxtr: list of :py:mod:`rts2saf.data.DataSxtr`
    :var Ds9Display: start ``DS9`` and display FITS files with regions 
    :var FitDisplay: display fit result
    :var ftwName: filter wheel name
    :var ftName: filter name
    :var focRes:  focuser resolution as given in FOCUSER_RESOLUTION  
    :var rt: run time configuration,  :py:mod:`rts2saf.config.Configuration`, usually read from /usr/local/etc/rts2/rts2saf/rts2saf.cfg
    :var ev: helper module for house keeping, :py:mod:`rts2saf.environ.Environment`
    :var logger:  :py:mod:`rts2saf.log`

"""

    def __init__(self, 
                 debug=False, 
                 date=None, 
                 dataSxtr=None, 
                 Ds9Display=False, 
                 FitDisplay=False, 
                 xdisplay = None,
                 ftwName=None, 
                 ftName=None, 
                 focRes=None, 
                 ev=None, 
                 logger=None):
        self.debug=debug
        self.date=date
        self.dataSxtr=dataSxtr
        self.Ds9Display=Ds9Display
        self.FitDisplay=FitDisplay
        self.xdisplay = xdisplay
        self.ftwName=ftwName
        self.ftName=ftName
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

    def _fitFwhm(self):

        minFitPos, minFitFwhm, fitPar, fitFlag= FitFunction(
            dataFit=self.dataFitFwhm, 
            logger=self.logger, 
            ).fitData()

        if minFitPos:
            self.logger.info('analyze: FWHM FOC_DEF: {0:5d} : fitted minimum position, {1:4.1f}px FWHM, {2} ambient temperature'.format(int(minFitPos), minFitFwhm, self.dataFitFwhm.ambientTemp))
        else:
            self.logger.warn('analyze: fit failed')
            self.resultFitFwhm=ResultFit()
            return 

        self.resultFitFwhm=ResultFit(
            ambientTemp = self.dataFitFwhm.ambientTemp, 
            ftName = self.dataFitFwhm.ftName,
            extrFitPos = minFitPos, 
            extrFitVal = minFitFwhm, 
            fitPar = fitPar,
            fitFlag = fitFlag,
            color = 'blue',
            ylabel = 'FWHM [px]: blue',
            titleResult = 'fwhm:{0:5d}'.format(int(minFitPos))
            )

    def _fitFlux(self):

        maxFitPos, maxFitFlux, fitPar, fitFlag = FitFunction(
            dataFit = self.dataFitFlux, 
            logger = self.logger, 
            ).fitData()

        if fitFlag:
            self.logger.info('analyze: Flux FOC_DEF: {0:5d} : fitted maximum position, {1:4.1f}[a.u.] Flux, {2} ambient temperature'.format(int(maxFitPos), maxFitFlux, self.dataFitFlux.ambientTemp))
        else:
            self.logger.warn('analyze: fit flux failed')
            self.resultFitFlux=ResultFit()
            return 

        self.resultFitFlux=ResultFit(
            ambientTemp = self.dataFitFlux.ambientTemp, 
            ftName = self.dataFitFlux.ftName,
            extrFitPos = maxFitPos, 
            extrFitVal=  maxFitFlux, 
            fitPar = fitPar,
            fitFlag = fitFlag,
            color = 'red',
            ylabel = 'FWHM [px]: blue Flux [a.u.]: red',
            titleResult = 'fwhm:{0:5d}, flux: {1:5d}'
            .format(int(self.resultFitFwhm.extrFitPos), int(maxFitPos))
            )

    def analyze(self):
        """Fit function to data and calculate weighted means.

        :return:  :py:mod:`rts2saf.data.ResultFit`, :py:mod:`rts2saf.data.ResultMeans`

        """
        # ToDo lazy                        !!!!!!!!!!
        # create an average and std 
        # ToDo decide which ftName from which ftw!!
        if len(self.dataSxtr)>0:
            bPth,fn=os.path.split(self.dataSxtr[0].fitsFn)
            ftName=self.dataSxtr[0].ftName
        else:
            bPth='/tmp'
            ftName='noFtName'

        if len(self.dataSxtr)>0:
            ambientTemp = self.dataSxtr[0].ambientTemp
        else:
            ambientTemp='noAmbientTemp'

        plotFn = self.ev.expandToPlotFileName(plotFn='{0}/{1}.png'.format(bPth,ftName))
        self.logger.info('analyze: storing plot file: {0}'.format(plotFn))
        # fwhm
        if len(self.dataSxtr)>0:
            ftName = self.dataSxtr[0].ftName
        else:
            ftName = 'NoFtname'

        self.dataFitFwhm = DataFitFwhm(
            dataSxtr = self.dataSxtr,
            plotFn = plotFn,
            ambientTemp = ambientTemp, 
            ftName = ftName,
            )
        self._fitFwhm()
        # weighted means
        self.resultMeansFwhm = ResultMeans(dataFit=self.dataFitFwhm, logger=self.logger)
        self.resultMeansFwhm.calculate(var='FWHM')

        try:
            self.i_flux = self.dataSxtr[0].fields.index('FLUX_MAX')
        except:
            pass

        if self.i_flux!=None:
            self.dataFitFlux= DataFitFlux(
                dataSxtr=self.dataSxtr,
                dataFitFwhm=self.dataFitFwhm,
                plotFn=plotFn,
                ambientTemp=self.dataSxtr[0].ambientTemp, 
                ftName=self.dataSxtr[0].ftName 
                )

            self._fitFlux()
            # weighted means
            self.resultMeansFlux=ResultMeans(dataFit=self.dataFitFlux, logger=self.logger)
            self.resultMeansFlux.calculate(var='Flux')

        # ToDo make a sensible decision
        return self.resultFitFwhm, self.resultMeansFwhm

    def display(self):
        """Plot data, fitted function for FWHM and optionally flux.

        """
        ft=FitDisplay(date = self.date, logger=self.logger)

        if self.i_flux is None:
            ft.fitDisplay(dataFit=self.dataFitFwhm, resultFit=self.resultFitFwhm, display=self.FitDisplay, xdisplay = self.xdisplay)
        else:
            # plot FWHM but don't show
            ft.fitDisplay(dataFit=self.dataFitFwhm, resultFit=self.resultFitFwhm, show=False, display=self.FitDisplay, xdisplay = self.xdisplay)
            ft.fitDisplay(dataFit=self.dataFitFlux, resultFit=self.resultFitFlux, display=self.FitDisplay, xdisplay = self.xdisplay)
        # very important (otherwise all plots show up in next show())
        ft.ax1=None
        # http://stackoverflow.com/questions/741877/how-do-i-tell-matplotlib-that-i-am-done-with-a-plot
        ft.fig.clf()
        ft.fig=None
        ft=None
        # plot them through ds9
        if self.Ds9Display and self.xdisplay:
            dds9=ds9()

            # ToDo create new list
            self.dataSxtr.sort(key=lambda x: int(x.focPos))

            for dSx in self.dataSxtr:
                if dSx.fitsFn:
                    dr=Ds9Region( dataSxtr=dSx, display=dds9, logger=self.logger)
                    if not dr.displayWithRegion():
                        break # something went wrong
                    time.sleep(1.)
                else:
                    self.logger.warn('analyze: OOOOOOOOPS, no file name for fits image number: {0:3d}'.format(dSx.fitsFn))
        elif self.Ds9Display and not self.xdisplay:
            self.logger.warn('analyze: OOOOOOOOPS, no ds9 display available')


import numpy
from itertools import ifilterfalse
from itertools import ifilter

# ToDo at the moment this method is an demonstrator
class CatalogAnalysis(object):
    """Analysis of extremes of FWHM and optionally of flux restricted to additional criteria based on SExtractor parameters.

    :var debug: enable more debug output with --debug and --level
    :var date: date string as it appears on plot 
    :var dataSxtr: list of :py:mod:`rts2saf.data.DataSxtr`
    :var Ds9Display: start ``DS9`` and display FITS files with regions 
    :var FitDisplay: display fit result
    :var ftwName: filter wheel name
    :var ftName: filter name
    :var moduleName: name of the module of type :py:mod:`rts2saf.criteria_radius.Criteria` or similar
    :var focRes:  focuser resolution as given in FOCUSER_RESOLUTION  
    :var rt: run time configuration,  :py:mod:`rts2saf.config.Configuration`, usually read from /usr/local/etc/rts2/rts2saf/rts2saf.cfg
    :var ev: helper module for house keeping, :py:mod:`rts2saf.environ.Environment`
    :var logger:  :py:mod:`rts2saf.log`

    """
    def __init__(self, 
                 debug=False, 
                 date = None, 
                 dataSxtr=None, 
                 Ds9Display=False, 
                 FitDisplay=False, 
                 xdisplay = None,
                 ftwName=None, 
                 ftName=None, 
                 focRes=None, 
                 moduleName=None, 
                 ev=None, 
                 rt=None, 
                 logger=None):

        self.debug=debug
        self.date = date
        self.dataSxtr=dataSxtr
        self.Ds9Display=Ds9Display
        self.FitDisplay=FitDisplay
        self.xdisplay = xdisplay
        self.ftwName=ftwName
        self.ftName=ftName
        self.focRes=focRes
        self.moduleName=moduleName
        self.ev=ev
        self.rt=rt
        self.logger=logger
        self.criteriaModule=None
        self.cr=None


    def _loadCriteria(self):
        # http://stackoverflow.com/questions/951124/dynamic-loading-of-python-modules
        # Giorgio Gelardi ["*"]!
        self.criteriaModule=__import__(self.moduleName, fromlist=["*"])
        self.cr=self.criteriaModule.Criteria(dataSxtr=self.dataSxtr, rt=self.rt)

    def selectAndAnalyze(self):
        """Fit function for accepted, rejected and all data  and calculate resp. weighted means .

        :return:  :py:mod:`rts2saf.data.ResultFit` for accepted, rejected and all data, resp. :py:mod:`rts2saf.data.ResultMeans` 

        """

        self._loadCriteria()
        # ToDo glitch
        i_f = self.dataSxtr[0].fields.index('FWHM_IMAGE')
        acceptedDataSxtr=list()
        rejectedDataSxtr=list()
        for dSx in self.dataSxtr:
            adSx=copy.deepcopy(dSx)
            acceptedDataSxtr.append(adSx)
            adSx.catalog= list(ifilter(self.cr.decide, dSx.catalog))
            nsFwhm=np.asarray([x[i_f] for x in adSx.catalog])
            adSx.fwhm=numpy.median(nsFwhm)
            adSx.stdFwhm=numpy.std(nsFwhm)

            rdSx=copy.deepcopy(dSx)
            rejectedDataSxtr.append(rdSx)
            rdSx.catalog=  list(ifilterfalse(self.cr.decide, dSx.catalog))

            nsFwhm=np.asarray([ x[i_f] for x in  rdSx.catalog])
            rdSx.fwhm=numpy.median(nsFwhm)
            rdSx.stdFwhm=numpy.std(nsFwhm)

        # 
        an=SimpleAnalysis(
            debug=self.debug, 
            date=self.date, 
            dataSxtr=acceptedDataSxtr, 
            Ds9Display=self.Ds9Display, 
            FitDisplay=self.FitDisplay, 
            xdisplay = self.xdisplay,
            focRes=self.focRes, 
            ev=self.ev, 
            logger=self.logger)

        accRFt, accRMns=an.analyze()
        try:
            self.logger.debug( 'ACCEPTED: weightedMeanObjects: {0:5.1f}, weightedMeanCombined: {1:5.1f}, minFitPos: {2:5.1f}, minFitFwhm: {0:5.1f}'.format(accRFt.weightedMeanObjects, accRFt.weightedMeanCombined, accRFt.minFitPos, accRFt.minFitFwhm))
        except:
            self.logger.debug('ACCEPTED: fit and calculation failed')
            
        if self.Ds9Display or self.FitDisplay:
            if accRFt.fitFlag:
                an.display()
        #
        an=SimpleAnalysis(
            debug=self.debug, 
            date=self.date, 
            dataSxtr=rejectedDataSxtr, 
            Ds9Display=self.Ds9Display, 
            FitDisplay=self.FitDisplay, 
            focRes=self.focRes, 
            ev=self.ev, 
            logger=self.logger)

        rejRFt, recRMns=an.analyze()
        try:
            self.logger.debug( 'REJECTED: weightedMeanObjects: {0:5.1f}, weightedMeanCombined: {1:5.1f}, minFitPos: {2:5.1f}, minFitFwhm: {3:5.1f}'.format(rejRFt.weightedMeanObjects, rejRFt.weightedMeanCombined, rejRFt.minFitPos, rejRFt.minFitFwhm))
        except:
            self.logger.debug('REJECTED: fit and calculation failed')

        if self.Ds9Display or self.FitDisplay:
            if accRFt.fitFlag:
                an.display()
        # 
        an=SimpleAnalysis(
            debug=self.debug, 
            date=self.date, 
            dataSxtr=self.dataSxtr, 
            Ds9Display=self.Ds9Display, 
            FitDisplay=self.FitDisplay, 
            focRes=self.focRes, 
            ev=self.ev, 
            logger=self.logger)

        allRFt, allRMns=an.analyze()
        try:
            self.logger.debug( 'ALL    : weightedMeanObjects: {0:5.1f}, weightedMeanCombined: {1:5.1f}, minFitPos: {2:5.1f}, minFitFwhm: {3:5.1f}'.format(allRFt.weightedMeanObjects, allRFt.weightedMeanCombined, allRFt.minFitPos, allRFt.minFitFwhm))
        except:
            self.logger.debug('ALL     : fit and calculation failed')


        if self.Ds9Display or self.FitDisplay:
            if accRFt.fitFlag:
                an.display()
        # ToDo here are three objects
        return accRFt, rejRFt, allRFt, accRMns, recRMns, allRMns

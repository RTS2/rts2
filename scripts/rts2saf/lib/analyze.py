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

try:
    import lib.data as dt
except:
    import data as dt
try:
    import lib.ds9region as ds9r
except:
    import ds9region as ds9r
try:
    import lib.fitfwhm as ft
except:
    import fitfwhm as ft

class SimpleAnalysis(object):
    """SimpleAnalysis a set of FITS"""
    def __init__(self, debug=False, dataSex=None, displayDs9=False, displayFit=False, ftwName=None, ftName=None, dryFits=False, focRes=None, ev=None, logger=None):
        self.debug=debug
        self.dataSex=dataSex
        self.displayDs9=displayDs9
        self.displayFit=displayFit
        self.ftwName=ftwName
        self.ftName=ftName
        self.dryFits=dryFits
        self.focRes=focRes
        self.ev=ev
        self.logger=logger
        self.fit=None

    def __fit(self, dFwhm=None):
        # ToDo make an option
        comment=None
        if self.dryFits:
            comment='dryFits'

        # Fit median FWHM data
        self.fit=ft.FitFwhm(
            showPlot=self.displayFit, 
            date=self.ev.startTime[0:19], 
            comment=comment,  # ToDo, define a sensible value
            dataFitFwhm=dFwhm, 
            logger=self.logger)

        return self.fit.fitData()

    def __analyze(self, dFwhm=None):

        nObjsC  = dFwhm.nObjs[:]
        posC    = dFwhm.pos[:]
        fwhmC   = dFwhm.fwhm[:]
        stdFwhmC= dFwhm.stdFwhm[:]
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
            if self.debug: self.logger.debug('analyze: {0:5d}: weighted mean derived from sextracted objects'.format(int(weightedMeanObjects)))
        except Exception, e:
            self.logger.warn('analyze: can not convert weightedMeanObjects:\n{0}'.format(e))
        # Weighted mean based on median FWHM
        weightedMeanFwhm=None
        try:
            weightedMeanFwhm= np.average(a=posC, axis=0, weights=map( lambda x: 1./x, fwhmC)) 
        except Exception, e:
            self.logger.warn('analyze: can not calculate weightedMeanFwhm:\n{0}'.format(e))

        try:
            self.logger.debug('analyze: {0:5d}: weighted mean derived from FWHM'.format(int(weightedMeanFwhm)))
        except Exception, e:
            self.logger.warn('analyze: can not convert weightedMeanFwhm:\n{0}'.format(e))
        # Weighted mean based on median std(FWHM)
        weightedMeanStdFwhm=None
        try:
            weightedMeanStdFwhm= np.average(a=posC, axis=0, weights=map( lambda x: 1./x, stdFwhmC)) 
        except Exception, e:
            self.logger.warn('analyze: can not calculate weightedMeanStdFwhm:\n{0}'.format(e))

        try:
            self.logger.debug('analyze: {0:5d}: weighted mean derived from std(FWHM)'.format(int(weightedMeanStdFwhm)))
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
            self.logger.debug('analyze: {0:5d}: weighted mean derived from Combined'.format(int(weightedMeanCombined)))
        except Exception, e:
            self.logger.warn('analyze: can not convert weightedMeanCombined:\n{0}'.format(e))
        
        minFitPos, minFitFwhm, fitPar= self.__fit(dFwhm=dFwhm)

        if minFitPos:
            if self.debug: self.logger.debug('analyze: {0:5d}: fitted minimum position, {1:4.1f}px FWHM, {2} ambient temperature'.format(int(minFitPos), minFitFwhm, dFwhm.ambientTemp))
        else:
            self.logger.warn('analyze: fit failed')

        return dt.ResultFitFwhm(
            ambientTemp=dFwhm.ambientTemp, 
            ftName=dFwhm.ftName,
            minFitPos=minFitPos, 
            minFitFwhm=minFitFwhm, 
            weightedMeanObjects=weightedMeanObjects, 
            weightedMeanFwhm=weightedMeanFwhm, 
            weightedMeanStdFwhm=weightedMeanStdFwhm, 
            weightedMeanCombined=weightedMeanCombined,
            fitPar=fitPar
            )

    def analyze(self):
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
        # ToDo lazy                        !!!!!!!!!!
        # create an average and std 
        # ToDo decide wich ftName from which ftw!!
        bPth,fn=os.path.split(self.dataSex[0].fitsFn)
        plotFn=self.ev.expandToPlotFileName(plotFn='{}/min-fwhm.png'.format(bPth))
        self.logger.info('analyze: storing plot file at: {}'.format(plotFn))
        df=dt.DataFitFwhm(plotFn=plotFn,ambientTemp=self.dataSex[0].ambientTemp, ftName=self.dataSex[0].ftName, pos=np.asarray(pos),fwhm=np.asarray(fwhm),errx=np.asarray(errx),stdFwhm=np.asarray(stdFwhm), nObjs=np.asarray(nObjs))
        return self.__analyze(dFwhm=df)

    def display(self):
        # ToDo ugly here
        DISPLAY=False
        if self.displayFit or self.displayDs9:
            try:
                os.environ['DISPLAY']
                DISPLAY=True
            except:
                self.logger.warn('analyze: no X-Window DISPLAY, do not plot with mathplotlib and/or ds9')

        if DISPLAY:
            if self.displayFit:
                self.fit.plotData()
            # plot them through ds9
            if self.displayDs9:
                try:
                    dds9=ds9()
                except Exception, e:
                    self.logger.error('analyze: OOOOOOOOPS, no ds9 display available')
                    return 

                #ToDo cretae new list
                self.dataSex.sort(key=lambda x: int(x.focPos))

                for dSx in self.dataSex:
                    print dSx.focPos
                    if dSx.fitsFn:
                        dr=ds9r.Ds9Region( dataSex=dSx, display=dds9, logger=self.logger)
                        if not dr.displayWithRegion():
                            break # something went wrong
                        time.sleep(1.)
                    else:
                        self.logger.warn('analyze: OOOOOOOOPS, no file name for fits image number: {0:3d}'.format(dSx.fitsFn))

import numpy
import math
from itertools import ifilterfalse
from itertools import ifilter
# ToDo at the moment this method is an demonstrator
class CatalogAnalysis(object):
    """CatalogAnalysis a set of FITS"""
    def __init__(self, debug=False, dataSex=None, displayDs9=False, displayFit=False, ftwName=None, ftName=None, dryFits=False, focRes=None, ev=None, rt=None, logger=None):
        self.debug=debug
        self.dataSex=dataSex
        self.displayDs9=displayDs9
        self.displayFit=displayFit
        self.ftwName=ftwName
        self.ftName=ftName
        self.dryFits=dryFits
        self.focRes=focRes
        self.ev=ev
        self.rt=rt
        self.logger=logger
        self.i_x = self.dataSex[0].fields.index('X_IMAGE')
        self.i_y = self.dataSex[0].fields.index('Y_IMAGE')

        self.center=[ self.dataSex[0].naxis1/2.,self.dataSex[0].naxis2/2. ] 
        
        rds= self.rt.cfg['RADIUS'] 
        if self.dataSex[0].binning:
            self.radius= rds/self.dataSex[0].binning 
        elif self.dataSex[0].binningXY:
            # ToDo (bigger): only x value is used
            self.radius= rds/self.dataSex[0].binningXY[0] 
        else:
            # everything should come
            self.radius=pow(self.dataSex[0].naxis1, 2) + pow(self.dataSex[0].naxis2, 2)

    def __criteria(self, ce=None):

        rd= math.sqrt(pow(ce[self.i_x]-self.center[0],2)+ pow(ce[self.i_y]-self.center[1],2))
        if rd < self.radius:
            return True
        else:
            return False

    def selectAndAnalyze(self):
        acceptedDataSex=dict()
        rejectedDataSex=dict()
        for cnt in self.dataSex.keys():
            acceptedDataSex[cnt]=copy.deepcopy(self.dataSex[cnt])
            acceptedDataSex[cnt].catalog= list(ifilter(self.__criteria, self.dataSex[cnt].catalog))

            i_f = self.dataSex[0].fields.index('FWHM_IMAGE')
            nsFwhm=np.asarray(map(lambda x: x[i_f], acceptedDataSex[cnt].catalog))
            acceptedDataSex[cnt].fwhm=numpy.median(nsFwhm)
            acceptedDataSex[cnt].stdFwhm=numpy.std(nsFwhm)


            rejectedDataSex[cnt]=copy.deepcopy(self.dataSex[cnt])
            rejectedDataSex[cnt].catalog=  list(ifilterfalse(self.__criteria, self.dataSex[cnt].catalog))
            nsFwhm=np.asarray(map(lambda x: x[i_f], rejectedDataSex[cnt].catalog))

            rejectedDataSex[cnt].fwhm=numpy.median(nsFwhm)
            rejectedDataSex[cnt].stdFwhm=numpy.std(nsFwhm)
        # 
        an=SimpleAnalysis(debug=self.debug, dataSex=acceptedDataSex, displayDs9=self.displayDs9, displayFit=self.displayFit, focRes=self.rt.foc.resolution, ev=self.ev, logger=self.logger)
        rFt=an.analyze()
        print
        print rFt.weightedMeanObjects, rFt.weightedMeanCombined, rFt.minFitPos, rFt.minFitFwhm
        if self.displayDs9 or self.displayFit:
            an.display()
        #
        an=SimpleAnalysis(debug=self.debug, dataSex=rejectedDataSex, displayDs9=self.displayDs9, displayFit=self.displayFit, focRes=self.rt.foc.resolution, ev=self.ev, logger=self.logger)
        rFt=an.analyze()
        print
        print rFt.weightedMeanObjects, rFt.weightedMeanCombined, rFt.minFitPos, rFt.minFitFwhm
        if self.displayDs9 or self.displayFit:
            an.display()
        # 
        an=SimpleAnalysis(debug=self.debug, dataSex=self.dataSex, displayDs9=self.displayDs9, displayFit=self.displayFit, focRes=self.rt.foc.resolution, ev=self.ev, logger=self.logger)
        rFt=an.analyze()

        if self.displayDs9 or self.displayFit:
            an.display()
        print
        print rFt.weightedMeanObjects, rFt.weightedMeanCombined, rFt.minFitPos, rFt.minFitFwhm
        # ToDo here are three objects
        return rFt


if __name__ == '__main__':

    import argparse
    import sys
    import logging
    import os
    import glob
    import re

    try:
        import lib.config as cfgd
    except:
        import config as cfgd
    try:
        import lib.sextract as sx
    except:
        import sextract as sx

    try:
        import lib.environ as env
    except:
        import environ as env

    try:
        import lib.log as lg
    except:
        import log as lg

    prg= re.split('/', sys.argv[0])[-1]
    parser= argparse.ArgumentParser(prog=prg, description='rts2asaf analysis')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--debugSex', dest='debugSex', action='store_true', default=False, help=': %(default)s,add more output on SExtract')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--topath', dest='toPath', metavar='PATH', action='store', default='.', help=': %(default)s, write log file to path')
    parser.add_argument('--logfile',dest='logfile', default='{0}.log'.format(prg), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--config', dest='config', action='store', default='/etc/rts2/rts2saf/rts2saf.cfg', help=': %(default)s, configuration file path')
    parser.add_argument('--basepath', dest='basePath', action='store', default=None, help=': %(default)s, directory where FITS images from possibly many focus runs are stored')
#ToDo    parser.add_argument('--ds9region', dest='ds9region', action='store_true', default=False, help=': %(default)s, create ds9 region files')
    parser.add_argument('--displayds9', dest='displayDs9', action='store_true', default=False, help=': %(default)s, display fits images and region files')
    parser.add_argument('--displayfit', dest='displayFit', action='store_true', default=False, help=': %(default)s, display fit')
    parser.add_argument('--cataloganalysis', dest='catalogAnalysis', action='store_true', default=False, help=': %(default)s, ananlys is done with CatalogAnalysis')

    args=parser.parse_args()

    lgd= lg.Logger(debug=args.debug, args=args) # if you need to chage the log format do it here
    logger= lgd.logger 

    rt=cfgd.Configuration(logger=logger)
    rt.readConfiguration(fileName=args.config)

    # get the environment
    ev=env.Environment(debug=args.debug, rt=rt,logger=logger)
    ev.createAcquisitionBasePath(ftwName=None, ftName=None)
    
    fitsFns=glob.glob('{0}/{1}'.format(args.basePath, rt.cfg['FILE_GLOB']))

    if len(fitsFns)==0:
        logger.error('analyze: no FITS files found in:{}'.format(args.basePath))
        logger.info('analyze: set --basepath or'.format(args.basePath))
        logger.info('analyze: download a sample from wget http://azug.minpet.unibas.ch/~wildi/rts2saf-test-focus-2013-09-14.tgz')
        logger.info('analyze: and store it in directory: {0}'.format(args.basePath))
        sys.exit(1)

    dataSex=dict()
    for k, fitsFn in enumerate(fitsFns):
        
        logger.info('analyze: processing fits file: {0}'.format(fitsFn))
        rsx= sx.Sextract(debug=args.debugSex, rt=rt, logger=logger)
        dataSex[k]=rsx.sextract(fitsFn=fitsFn) 

    if args.catalogAnalysis:
        an=CatalogAnalysis(debug=args.debug, dataSex=dataSex, displayDs9=args.displayDs9, displayFit=args.displayFit, focRes=rt.foc.resolution, ev=ev, rt=rt, logger=logger)
        resultFitFwhm=an.selectAndAnalyze()
    else:
        an=SimpleAnalysis(debug=args.debug, dataSex=dataSex, displayDs9=args.displayDs9, displayFit=args.displayFit, focRes=rt.foc.resolution, ev=ev, logger=logger)
        resultFitFwhm=an.analyze()
        if args.displayDs9 or args.displayFit:
            an.display()

    logger.info('analyze: result: weightedMeanObjects: {0}, weightedMeanFwhm: {1}, minFitPos: {2}, fwhm: {3}'.format(resultFitFwhm.weightedMeanObjects, resultFitFwhm.weightedMeanFwhm, resultFitFwhm.minFitPos, resultFitFwhm.fitFwhm))


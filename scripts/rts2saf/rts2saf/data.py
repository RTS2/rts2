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

import numpy as np
import math

class DataFit(object):
    def __init__(self, plotFn=None, ambientTemp=None, ftName=None, pos=list(), val=list(), errx=list(), erry=list(), nObjs=None, par=None):
        self.plotFn=plotFn
        self.ambientTemp=ambientTemp
        self.ftName=ftName
        self.pos=pos
        self.val=val
        self.errx=errx
        self.erry=erry
        self.nObjs=nObjs
        self.par=par


class DataFitFwhm(DataFit):
    def __init__( self, dataSex=None, *args, **kw ):
        super(  DataFitFwhm, self ).__init__( *args, **kw )
        self.dataSex=dataSex
        self.pos =np.asarray([x.focPos for x in dataSex])
        self.val =np.asarray([x.fwhm for x in dataSex])
        self.errx=np.asarray([x.stdFocPos for x in dataSex])
        self.erry=np.asarray([x.stdFwhm for x in dataSex])  
        self.nObjs=[len(x.catalog) for x in dataSex]
        # ToDo must reside outside
        self.par= np.array([1., 1., 1., 1.])
        self.fitFunc = lambda x, p: p[0] + p[1] * x + p[2] * (x ** 2)+ p[3] * (x ** 4)  # due to optimize.fminbound
        self.recpFunc = None

class DataFitFlux(DataFit):
    def __init__( self, dataSex=None, dataFitFwhm=None, i_flux=None, *args, **kw ):
        super(  DataFitFlux, self ).__init__( *args, **kw )
        self.dataSex=dataSex
        self.i_flux=i_flux
        self.dataFitFwhm=dataFitFwhm
        self.pos =np.asarray([x.focPos for x in dataSex])
        self.val =np.asarray([x.flux for x in dataSex])
        self.errx=np.asarray([x.stdFocPos for x in dataSex])
        self.erry=np.asarray([x.stdFlux for x in dataSex])  
        self.nObjs=[len(x.catalog) for x in dataSex]
        self.par= None # see below
        self.fitFunc = lambda x, p: p[3] + p[0]*np.exp(-(x-p[1])**2/(2*p[2]**2))
        self.recpFunc = lambda x, p: 1./(p[3] + p[0]*np.exp(-(x-p[1])**2/(2*p[2]**2)))

        # scale the values [a.u.]
        mfw=max(self.dataFitFwhm.val)
        mfl=max(self.val)
        sv =   [mfw/mfl * x for x in self.val]
        sstd = [mfw/mfl * x for x in self.erry]
        self.val=sv
        self.erry=sstd
        # start values for fit
        x=np.array([p for p in self.pos])
        y=np.array([v for v in self.val])

        wmean= np.average(a=x, weights=y) 
        xy= zip(x,y)
        wstd = np.std(a=xy) 
        # fit gaussian
        self.par= np.array([ 10., wmean, wstd/40., 2.]) # ToDo !!!!!!!!!!!!!!!!!!!!!!!!!!!!

class ResultMeans(object):
    def __init__(self, dataFit=None, logger=None):
        self.dataFit=dataFit
        self.logger=logger
        self.objects=None
        self.val=None
        self.stdVal=None
        self.combined=None

        self.nObjsC = self.dataFit.nObjs[:]
        self.posC   = self.dataFit.pos[:]
        self.valC   = self.dataFit.val[:]
        self.stdValC= self.dataFit.erry[:]
        # remove elements with val=0
        while True:
            try:
                ind=valC.index(0.)
            except:
                break # ToDo what happens here really
            del self.nObjsC[ind] # not strictly necessary
            del self.posC[ind]
            del self.valC[ind]
            del self.stdValC[ind]

    def calculate(self, var=None):
        # Weighted mean based on number of extracted objects (stars)
        try:
            self.objects= np.average(a=self.posC, axis=0, weights=self.nObjsC)
        except Exception, e:
            self.logger.warn('ResultMeans: can not calculate weightedMeanObjects:\n{0}'.format(e))

        try:
            self.logger.info('ResultMeans: FOC_DEF: {0:5d} : weighted mean derived from sextracted objects'.format(int(self.objects)))
        except Exception, e:
            self.logger.warn('ResultMeans: can not convert weightedMeanObjects:\n{0}'.format(e))
        # Weighted mean based on median FWHM, Flux
        if var in 'FWHM':
            wght= [ 1./x for x in  self.valC ]
        else:
            wght= [ x for x in  self.valC ]

        try:
            self.val= np.average(a=self.posC, axis=0, weights=wght) 
        except Exception, e:
            self.logger.warn('ResultMeans: can not calculate weightedMean{0}:\n{0}'.format(var,e))

        try:
            self.logger.info('ResultMeans: FOC_DEF: {0:5d} : weighted mean derived from {1}'.format(int(self.val), var))
        except Exception, e:
            self.logger.warn('ResultMeans: can not convert weightedMean{0}:\n{1}'.format(var,e))
        # Weighted mean based on median std(FWHM, Flux)
        try:
            self.stdVal= np.average(a=self.posC, axis=0, weights=[ 1./x for x in  self.stdValC]) 
        except Exception, e:
            self.logger.warn('ResultMeans: can not calculate weightedMeanStd{0}:\n{1}'.format(var,e))

        try:
            self.logger.info('ResultMeans: FOC_DEF: {0:5d} : weighted mean derived from std({1})'.format(int(self.stdVal), var))
        except Exception, e:
            self.logger.warn('ResultMeans: can not convert weightedMeanStd{0}:\n{1}'.format(var,e))
        # Weighted mean based on a combination of variables
        combined=list()
        for i, v in enumerate(self.nObjsC):
            combined.append( self.nObjsC[i]/(self.stdValC[i] * self.valC[i]))

        try:
            self.combined= np.average(a=self.posC, axis=0, weights=combined)
        except Exception, e:
            self.logger.warn('ResultMeans: can not calculate weightedMeanCombined{0}:\n{1}'.format(var,e))

        try:
            self.logger.info('ResultMeans: FOC_DEF: {0:5d} : weighted mean derived from Combined{1}'.format(int(self.combined),var))
        except Exception, e:
            self.logger.warn('ResultMeans: can not convert weightedMeanCombined{0}:\n{1}'.format(var, e))


class ResultFit(object):
    def __init__(self, ambientTemp=None, ftName=None, extrFitPos=None, extrFitVal=None, fitPar=None, fitFlag=None, color=None, ylabel=None, titleResult=None):
        self.ambientTemp=ambientTemp
        self.ftName=ftName
        self.extrFitPos=extrFitPos
        self.extrFitVal=extrFitVal
        self.fitPar=fitPar
        self.fitFlag=fitFlag
        self.color=color
        self.ylabel=ylabel
        self.titleResult=titleResult

class DataSex(object):
    def __init__(self, fitsFn=None, focPos=None, stdFocPos=None, fwhm=None, stdFwhm=None, flux=None, stdFlux=None, nstars=None, ambientTemp=None, catalog=None, binning=None, binningXY=None, naxis1=None, naxis2=None, fields=None, ftName=None, ftAName=None, ftBName=None, ftCName=None, assocFn=None):
        self.fitsFn=fitsFn
        self.focPos=focPos
        self.stdFocPos=stdFocPos
        self.fwhm=fwhm
        self.stdFwhm=stdFwhm
        self.nstars=nstars
        self.ambientTemp=ambientTemp
        self.catalog=catalog
        # ToDo ugly
        try:
            self.nObjs=len(self.catalog)
        except:
            pass
        self.fields=fields
        self.binning=binning 
        self.binningXY=binningXY 
        self.naxis1=naxis1
        self.naxis2=naxis2
        self.ftName=ftName
        self.ftAName=ftAName
        self.ftBName=ftBName
        self.ftCName=ftCName
        # ToDo: make a theod for fwhm and flux, source self.catalog
        # filled in analyze 
        self.flux=flux # unittest!
        self.stdFlux=stdFlux
        self.assocFn=assocFn
        self.assocCount=0
        self.rawCatalog=list()

    def fillFlux(self, i_flux=None):
        fluxv = [x[i_flux] for x in  self.catalog]
        self.flux=np.median(fluxv)
        self.stdFlux= np.average([ math.sqrt(x) for x in fluxv])

    def fillData(self, i_fwhm=None, i_flux=None):
        fwhmv = [x[i_fwhm] for x in  self.catalog]
        self.fwhm=np.median(fwhmv)
        self.stdFwhm= np.std(fwhmv)

        if i_flux !=None:
            fluxv = [x[i_flux] for x in  self.catalog]
            self.flux=np.median(fluxv)
            self.stdFlux= np.average([ math.sqrt(x) for x in fluxv]) 


    def toRawCatalog(self):
        self.rawCatalog=self.catalog[:]
        self.catalog=list()

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
    def __init__(self, ambientTemp=None, ftName=None, weightedMeanObjects=None, weightedMeanFwhm=None, weightedMeanStdFwhm=None, weightedMeanCombined=None):

        self.ambientTemp=ambientTemp
        self.ftName=ftName
        self.weightedMeanObjects=weightedMeanObjects
        self.weightedMeanFwhm=weightedMeanFwhm
        self.weightedMeanStdFwhm=weightedMeanStdFwhm
        self.weightedMeanCombined=weightedMeanCombined


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
    def __init__(self, fitsFn=None, focPos=None, stdFocPos=None, fwhm=None, stdFwhm=None, flux=None, stdFlux=None, nstars=None, ambientTemp=None, catalog=None, binning=None, binningXY=None, naxis1=None, naxis2=None, fields=None, ftName=None, ftAName=None, ftBName=None, ftCName=None):
        self.fitsFn=fitsFn
        self.focPos=focPos
        self.stdFocPos=stdFocPos
        self.fwhm=fwhm
        self.stdFwhm=stdFwhm
        self.nstars=nstars
        self.ambientTemp=ambientTemp
        self.catalog=catalog
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


    def fillFlux(self, i_flux=None):
        fluxv = [x[i_flux] for x in  self.catalog]
        self.flux=np.median(fluxv)
        self.stdFlux= np.std(fluxv)/10. # ToDo !!!!!!!!!!!!!!!!!!!!!

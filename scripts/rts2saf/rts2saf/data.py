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

class DataFit(object):
    def __init__(self, plotFn=None, ambientTemp=None, ftName=None, pos=None, val=None, errx=None, erry=None, nObjs=None):
        self.plotFn=plotFn
        self.ambientTemp=ambientTemp
        self.ftName=ftName
        self.pos=pos
        self.val=val
        self.errx=errx
        self.erry=erry
        self.nObjs=nObjs

class ResultMeans(object):
    def __init__(self, ambientTemp=None, ftName=None, weightedMeanObjects=None, weightedMeanFwhm=None, weightedMeanStdFwhm=None, weightedMeanCombined=None):

        self.ambientTemp=ambientTemp
        self.ftName=ftName
        self.weightedMeanObjects=weightedMeanObjects
        self.weightedMeanFwhm=weightedMeanFwhm
        self.weightedMeanStdFwhm=weightedMeanStdFwhm
        self.weightedMeanCombined=weightedMeanCombined


class ResultFit(object):
    def __init__(self, ambientTemp=None, ftName=None, extrFitPos=None, extrFitVal=None, fitPar=None, fitFlag=None, color=None, ylabel=None):
        self.ambientTemp=ambientTemp
        self.ftName=ftName
        self.extrFitPos=extrFitPos
        self.extrFitVal=extrFitVal
        self.fitPar=fitPar
        self.fitFlag=fitFlag
        self.color=color
        self.ylabel=ylabel

class DataSex(object):
    def __init__(self, fitsFn=None, focPos=None, fwhm=None, stdFwhm=None, nstars=None, ambientTemp=None, catalog=None, binning=None, binningXY=None, naxis1=None, naxis2=None, fields=None, ftName=None, ftAName=None, ftBName=None, ftCName=None):
        self.fitsFn=fitsFn
        self.focPos=focPos
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
        self.flux=None
        self.stdFlux=None

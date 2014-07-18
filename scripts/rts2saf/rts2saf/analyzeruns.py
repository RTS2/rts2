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

""" AnalyzeRuns performs the offline analysis of many runs.
"""

__author__ = 'markus.wildi@bluewin.ch'


import fnmatch
import os
import re
# frosted:                     this is used
from sympy import Symbol,diff, exp

from rts2saf.sextract import Sextract
from rts2saf.analyze import SimpleAnalysis, CatalogAnalysis
from rts2saf.datarun import DataRun
from rts2saf.data import FitFunctionFwhm, FitFunctionFlux

# thanks http://stackoverflow.com/questions/635483/what-is-the-best-way-to-implement-nested-dictionaries-in-python
class AutoVivification(dict):
    """Implementation of perl's autovivification feature."""
    def __getitem__(self, item):
        try:
            return dict.__getitem__(self, item)
        except KeyError:
            value = self[item] = type(self)()
            return value

# attempt to group sub directories into focus runs
# goal calculate filter offsets
#
#2013-09-04T22:18:35.077526/fliterwheel/filter
#                                                                                                         date    ftw  ft
MANYFILTERWHEELS = re.compile('.*/([0-9]{4,4}-[0-9]{2,2}-[0-9]{2,2}T[0-9]{2,2}\:[0-9]{2,2}\:[0-9]{2,2}\.[0-9]+)/(.+?)/(.+)')
#                                                                                                  date    ft
ONEFILTER = re.compile('.*/([0-9]{4,4}-[0-9]{2,2}-[0-9]{2,2}T[0-9]{2,2}\:[0-9]{2,2}\:[0-9]{2,2}\.[0-9]+)/(.+?)')

ONLYFILTERS = re.compile('.*/(.+?)')

# no filter
#2013-09-04T22:18:35.077526/
DATE = re.compile('.*/([0-9]{4,4}-[0-9]{2,2}-[0-9]{2,2}T[0-9]{2,2}\:[0-9]{2,2}\:[0-9]{2,2}\.[0-9]+)')

class AnalyzeRuns(object):
    """AnalyzeRuns analize focus runs in a subdirectory.

    :var debug: enable more debug output with --debug and --level
    :var basePath: where the FITS files are stored 
    :var args: command line arguments or their defaults
    :var rt: run time configuration usually read from /usr/local/etc/rts2/rts2saf/rts2saf.cfg
    :var ev: helper module for house keeping
    :var logger:  logger object
    :var fs: contains the subdirectory structure and the associated FITS files

    """

    def __init__(self, debug=False, basePath=None, args=None, rt=None, ev=None, logger=None, xdisplay = None, ftw = None, ft = None):

        self.debug = debug
        self.basePath = basePath
        self.args = args
        self.rt = rt
        self.ev = ev
        self.logger = logger
        self.xdisplay = xdisplay
        self.fS = AutoVivification()
        self.assocFn='/tmp/assoc.lst'
        self.ftw = ftw
        self.ft = ft

    def compareFwhm(self, rFt):
        # ToDo the correct values are stored in Focuser() object
        if self.rt.cfg['FWHM_MIN'] < rFt.extrFitVal < self.rt.cfg['FWHM_MAX']:
            if self.debug: self.logger.debug('compareFwhm: FWHM: {} < {} < {}, accepted'.format(self.rt.cfg['FWHM_MIN'], rFt.extrFitVal, self.rt.cfg['FWHM_MAX']))
            return True
        else:
            self.logger.warn('Focus: not writing FOC_DEF: {0}, minFitFwhm: {1}, out of bounds: {2},{3} (FWHM_MIN,FWHM_MAX), rejected'.format(int(rFt.extrFitPos), rFt.extrFitVal, self.rt.cfg['FWHM_MIN'], self.rt.cfg['FWHM_MAX']))
            return False

    def compareFunctionValues(self, fitPar= None, dataRn = None, extrFitPos = None, name = None, dtFitFunc = None):
        """ 

        :var fitPar:  fit parameters
        :var dataRn:  DataRun containg FOC_POS, function values
        :var extrFitPos:  focuser position of the extremum
        :var name:  type of the fit

        :rtype: Boolean.

        """
        fitFunc = dtFitFunc.fitFunc

        dSxs = sorted(dataRn.dataSxtrs, key=lambda x: int(x.focPos), reverse=False)
        mn = int(dSxs[0].focPos)
        mx = int(dSxs[-1].focPos)
        extremumValue = round(fitFunc(float(extrFitPos), fitPar), 2)

        if 'FWHM' in name:
            edgeValue = round(min(fitFunc(float(mn), fitPar), fitFunc(float(mx), fitPar)), 2)

            if extremumValue < edgeValue:
                if self.debug: self.logger.debug('{} compareFunctionValues, extremumValue: {} < {} edgeValue, accepted'.format(name, extremumValue, edgeValue))
                return True
            else:
                if self.debug: self.logger.debug('{} compareFunctionValues, extremumValue: {} < {} edgeValue, rejected'.format(name, extremumValue, edgeValue))
                return False
        else:
            edgeValue = round(max(fitFunc(float(mn), fitPar), fitFunc(float(mx), fitPar)), 2)

            if extremumValue > edgeValue:
                if self.debug: self.logger.debug('{} compareFunctionValues, extremumValue: {} > {} edgeValue), accepted'.format(name, extremumValue, edgeValue))
                return True
            else:
                if self.debug: self.logger.debug('{} compareFunctionValues, edgeValue: {} > {} extremumValue, rejected'.format(name, edgeValue, extremumValue))
                return False
                        

    def inflectionPoints(self, fitPar= None, dataRn = None, extrFitPos = None, name = None, fitfuncSS = None):
        """ 

        :var fitPar:  fit parameters
        :var dataRn:  DataRun containg FOC_POS
        :var extrFitPos:  focuser position of the extremum
        :var name:  type of the fit

        :rtype: Boolean.

        """
        # frosted: this is used
        p = fitPar
        x = Symbol('x')

        fitFunc = eval(fitfuncSS)
        fitFunc1 = diff( fitFunc, x)
        fitFunc2 = diff( fitFunc1, x)

        dSxs = sorted(dataRn.dataSxtrs, key=lambda x: int(x.focPos), reverse=False)
        lastSign = None
        signChange = list()
        mn = int(dSxs[0].focPos)
        mx = int(dSxs[-1].focPos)

        for fp in range(mn, mx): # ToDO: glitch! as step size something like int(self.rt.cfg['FOCUSER_RESOLUTION'])

            val = fitFunc2.evalf(subs={x:float(fp)})
            sign = round(val /abs(val), 2)

            if lastSign is None:
                lastSign = sign

            #if self.debug: self.logger.debug('{} inflectionPoints: sign: {}, at: {}'.format(math.copysign(1, val), fp))
            if lastSign != sign:
                lastSign = sign
                if self.debug: self.logger.debug('{} inflectionPoints: sign changed at: {}, {}/{}'.format(name, fp, sign, val))
                signChange.append(fp)

        signChanges = len(signChange)
        extrFitPos  = round(extrFitPos,2)
        if signChanges > 2:
            self.logger.warn('{} inflectionPoints: more than 2 sign changes: {}'.format(name, len(signChange)))
            return False
        elif signChanges == 2:
            # extrFitPos must lie between sign changes
            if signChange[0] < extrFitPos < signChange[1]:
                if self.debug: self.logger.debug('{} inflectionPoints: extrFitPos between sign changes: {} < {} < {}, accpeted'.format(name, signChange[0], extrFitPos, signChange[1]))
                return True
            else:
                if self.debug: self.logger.debug('{} inflectionPoints: sign changes at: {} and {}, extrFitPos at: {}, rejected'.format(name, signChange[0], signChange[1], extrFitPos))
                return False
            
        elif signChanges == 1:
            # extrFitPos must lie in the larger part of the interval
            if signChange[0] - mn < mx - signChange[0]:
                if extrFitPos > signChange[0]:
                    if self.debug: self.logger.debug('{} inflectionPoints: sign changes at: {}  lower: {}, upper limit: {}:, extrFitPos at: {}, accepted'.format(name, signChange[0], mn, mx, extrFitPos))
                    return True
                else:
                    if self.debug: self.logger.debug('{} inflectionPoints: sign changes at: {}  lower: {}, upper limit: {}:, extrFitPos at: {}, rejected'.format(name, signChange[0], mn, mx, extrFitPos))
                    return False
            else:
                if extrFitPos < signChange[0]:
                    if self.debug: self.logger.debug('{} inflectionPoints: sign changes at: {}  lower: {}, upper limit: {}:, extrFitPos at: {}, accepted'.format(name, signChange[0], mn, mx, extrFitPos))
                    return True
                else:
                    if self.debug: self.logger.debug('{} inflectionPoints: sign changes at: {}  lower: {}, upper limit: {}:, extrFitPos at: {}, rejected'.format(name, signChange[0], mn, mx, extrFitPos))
                    return False
        else:
            if self.debug: self.logger.debug('{} inflectionPoints: no sign changes found'.format(name))
            return True

    def bestResult(self, 
                   rFtFwhm=None, arFtFwhm=None,
                   rFtFlux=None, arFtFlux=None,
               ):
        """ Choose among simple and associated fit results.

        :param :  . 
        :type : .
        :rtype: ResultFit.

        """
        # ToDo this is the initial version
        if arFtFwhm is not None and arFtFlux is not None:
            diff = abs(arFtFwhm.extrFitPos - arFtFlux.extrFitPos)
            if  diff <= int(self.rt.cfg['FOCUSER_RESOLUTION']): 
                self.logger.info('bestResult: assoc smaller positional difference between Fwhm and Flux: {0:5d} than focuser resolution: {1:4d}'.format(int(diff), int(self.rt.cfg['FOCUSER_RESOLUTION'])))
                # ToDo return mean or something else
                return arFtFwhm

            else:
                self.logger.info('bestResult: assoc larger positional difference between Fwhm and Flux: {0:5d} than focuser resolution: {1:4d}'.format(int(diff), int(self.rt.cfg['FOCUSER_RESOLUTION'])))
                return arFtFwhm

        elif rFtFwhm is not None and rFtFlux is not None:

            diff = abs(rFtFwhm.extrFitPos - rFtFlux.extrFitPos)
            if  diff <= int(self.rt.cfg['FOCUSER_RESOLUTION']): 
                self.logger.info('bestResult: smaller positional difference between Fwhm and Flux: {0:5d} than focuser resolution: {1:4d}'.format(int(diff), int(self.rt.cfg['FOCUSER_RESOLUTION'])))
                # ToDo return mean or something else
                return rFtFwhm

            else:
                self.logger.info('bestResult: larger positional difference between Fwhm and Flux: {0:5d} than focuser resolution: {1:4d}'.format(int(diff), int(self.rt.cfg['FOCUSER_RESOLUTION'])))
                return rFtFwhm
        elif arFtFwhm is not None:
            # the associated FWHM minimum is usually better
            return arFtFwhm

        elif rFtFwhm is not None:
            return rFtFwhm
            self.logger.warn('bestResult: no fitted minimum found')

        return None

    def analyzeRun(self, dataRn = None):
        """Analyze single run.

        """

        if dataRn is None:
            return None, None, None, None 

        if not dataRn.numberOfFocPos():
            self.logger.error('analyzeRun: too few different focuser positions')
            return None, None, None, None 

        if  self.rt.cfg['ANALYZE_ASSOC'] and (len(dataRn.assocObjNmbrs) < self.rt.cfg['MINIMUM_OBJECTS']):
            self.logger.warn('analyzeRun: to few objects found: {0}, required: {1} (MINIMUM_OBJECTS)'.format(len(dataRn.assocObjNmbrs), self.rt.cfg['MINIMUM_OBJECTS']))
            return None, None, None, None 

        elif not self.rt.cfg['ANALYZE_ASSOC']:
            for dSx in [x for x in dataRn.dataSxtrs]: # can not remove on list iterated
                if len(dSx.catalog) < self.rt.cfg['MINIMUM_OBJECTS']:
                    self.logger.warn('analyzeRuns: to few objects found: {0}, required: {1} (MINIMUM_OBJECTS), dropping: {2}'.format(len(dSx.catalog), self.rt.cfg['MINIMUM_OBJECTS'], dSx.fitsFn))
                    dataRn.dataSxtrs.remove(dSx)

        binning=dict()
        for dSx in dataRn.dataSxtrs:
            try:
                binning[dSx.binning] += 1
            except Exception, e:
                binning[dSx.binning] = 1
                
        if len(binning) != 1:
            self.logger.error('analyzeRun: inconsistent binning found: {}'.format(repr(binning)))
            return None, None, None, None 

        date = dataRn.dataSxtrs[0].date.split('.')[0]

        rFtFwhm = rMnsFwhm = rFtFlux = rMnsFlux = None
        if self.args.catalogAnalysis:
            an = CatalogAnalysis(
                debug = self.debug, 
                date = date, 
                dataSxtr = dataRn.dataSxtrs, 
                Ds9Display = self.args.Ds9Display, 
                FitDisplay = self.args.FitDisplay, 
                xdisplay = self.xdisplay,
                focRes = float(self.rt.cfg['FOCUSER_RESOLUTION']), 
                moduleName = self.args.criteria, 
                ev=self.ev, 
                rt = self.rt, 
                logger = self.logger)
            # ToDo, expand to Flux!
            rFtFwhm, rrFt, allrFt, rMnsFwhm, rrMns, allrMns= an.selectAndAnalyze()
            # plotting is done within CatalogAnalysis
        else:
            an = SimpleAnalysis(
                debug = self.debug, 
                date = date, 
                dataSxtr = dataRn.dataSxtrs, 
                Ds9Display = self.args.Ds9Display, 
                FitDisplay = self.args.FitDisplay, 
                xdisplay = self.xdisplay,
                focRes = float(self.rt.cfg['FOCUSER_RESOLUTION']), 
                ev = self.ev, 
                rt = self.rt,
                logger = self.logger)

            rFtFwhm, rMnsFwhm, rFtFlux, rMnsFlux = an.analyze()

        if rFtFwhm is not  None and rFtFwhm.extrFitPos is not  None:
            # reject fit if curvature change sign, indicator for more than one minimum
            fitfuncSS = FitFunctionFwhm().fitFuncSS
            if not self.inflectionPoints(fitPar = rFtFwhm.fitPar, dataRn = dataRn, extrFitPos = rFtFwhm.extrFitPos, name = 'FWHM', fitfuncSS = fitfuncSS):
                self.logger.warn('analyzeRun: FWHM fit rejected due to sign change of 2nd derivative')
            else:
                if self.compareFwhm(rFtFwhm):
                    if not self.compareFunctionValues(fitPar = rFtFwhm.fitPar, dataRn = dataRn, extrFitPos = rFtFwhm.extrFitPos, name = 'FWHM', dtFitFunc = FitFunctionFwhm()):
                        self.logger.warn('analyzeRun: FWHM fit rejected due to FWHM not lower than one of at the edges')
                    else:
                        if self.debug: self.logger.debug('analyzeRun: FWHM fit accpeted')
                        rFtFwhm.accepted = True
                else:
                    self.logger.warn('analyzeRun: FWHM fit rejected due to bad FWHM value')

        if rFtFlux is not  None and rFtFlux.extrFitPos is not  None:

            fitfuncSS = FitFunctionFlux().fitFuncSS
            if not self.inflectionPoints(fitPar = rFtFlux.fitPar, dataRn = dataRn, extrFitPos = rFtFlux.extrFitPos, name = 'flux', fitfuncSS = fitfuncSS):
                self.logger.warn('analyzeRun: flux fit rejected due to sign change of 2nd derivative')
            else:
                if not self.compareFunctionValues(fitPar = rFtFlux.fitPar, dataRn = dataRn, extrFitPos = rFtFlux.extrFitPos, name = 'flux', dtFitFunc = FitFunctionFlux()):
                    self.logger.warn('analyzeRun: flux fit rejected due to flux not higher than one of at the edges')
                else:
                    if self.debug: self.logger.debug('analyzeRun: flux fit accpeted')
                    rFtFlux.accepted = True

        if self.rt.cfg['ANALYZE_ASSOC']:
            if self.ftw is not None and self.ft is not None:
                self.logger.info('assoc fitFocDef for filter wheel:{0}, filter:{1}'.format(self.ftw.name, self.ft.name))

            if rFtFwhm is not None and rFtFwhm.extrFitPos:

                self.logger.info('assoc fwhm fitFocDef: {0:5.0f}: minFitPos'.format(rFtFwhm.extrFitPos))
                self.logger.info('assoc fwhm fitFocDef: {0:5.2f}: minFitFwhm'.format(rFtFwhm.extrFitVal))
            else:
                self.logger.warn('assoc fwhm fitFocDef: no fitted minimum found')

            if rFtFlux is not None and rFtFlux.extrFitPos:
                self.logger.info('assoc flux fitFocDef: {0:5.0f}: minFitPos'.format(rFtFlux.extrFitPos))
                self.logger.info('assoc flux fitFocDef: {0:5.2f}: minFitFwhm'.format(rFtFlux.extrFitVal))
            else:
                if self.args.flux: self.logger.warn('assoc flux fitFocDef: no fitted minimum found')
        else:
            if self.ftw is not None and self.ft is not None:
                self.logger.info('fitFocDef for filter wheel:{0}, filter:{1}'.format(self.ftw.name, self.ft.name))
            if rFtFwhm is not None and rFtFwhm.extrFitPos:
                self.logger.info('fwhm fitFocDef: {0:5.0f}: minFitPos'.format(rFtFwhm.extrFitPos))
                self.logger.info('fwhm fitFocDef: {0:5.2f}: minFitFwhm'.format(rFtFwhm.extrFitVal))
            else:
                self.logger.warn('fwhm fitFocDef: no fitted minimum found')

            if rFtFlux is not None and rFtFlux.extrFitPos:
                self.logger.info('flux fitFocDef: {0:5.0f}: minFitPos'.format(rFtFlux.extrFitPos))
                self.logger.info('flux fitFocDef: {0:5.2f}: minFitFlux'.format(rFtFlux.extrFitVal))
            else:
                if self.args.flux: self.logger.warn('flux fitFocDef: no fitted minimum found')

        # on e.g. Bootes-2 creating a plot and write takes 30 seconds, too long
        if self.rt.cfg['WITH_MATHPLOTLIB']:
            if self.args.catalogAnalysis:
                if rFtFwhm.fitFlag:
                    an.anAcc.display()
            else:
                if rFtFwhm.fitFlag:
                    an.display()

        # ToDo expand to Flux?
        if self.rt.cfg['WEIGHTED_MEANS']:
            # ToDo what is that:
            if not rMnsFwhm is not  None:
                rMnsFwhm.logWeightedMeans()

        return rFtFwhm, rMnsFwhm, rFtFlux, rMnsFlux

    def sextract(self, fitsFns=None, dSxReference = None, dataRn = None):
        for fitsFn in fitsFns:
            rsx= Sextract(debug=self.args.sxDebug, rt=self.rt, logger=self.logger)
            if self.rt.cfg['ANALYZE_FLUX']:
                rsx.appendFluxFields()
                
            if dSxReference is not None:
                rsx.appendAssocFields()
                dSx=rsx.sextract(fitsFn=fitsFn, assocFn=self.assocFn)
            else:
                dSx=rsx.sextract(fitsFn=fitsFn, assocFn=None)

            if dSx is not None and dSx.fwhm>0. and dSx.stdFwhm>0.:
                dataRn.dataSxtrs.append(dSx)
                self.logger.debug('sextract:  {}, ok: {}'.format(len(dSx.catalog), fitsFn))
            else:
                self.logger.warn('sextract: no result, file: {0}'.format(fitsFn))

    def createAssocList(self, fitsFn=None):
        rsx= Sextract(debug=self.args.sxDebug, createAssoc=self.rt.cfg['ANALYZE_ASSOC'], rt=self.rt, logger=self.logger)
        if self.rt.cfg['ANALYZE_FLUX']:
            rsx.appendFluxFields()

        return rsx.sextract(fitsFn=fitsFn, assocFn=self.assocFn)

    def sextractRun(self, fitsFns = None):
        """Analyze a set of FITS files having all the same filter set  forming a 
        single focus run.


        :param fitsFns:  list of FITS file names. 
        :type fitsFns: list of strings
        :rtype: ResultFit and ResultMeans.

        """

        # define the reference FITS as the one with the most sextracted objects
        # ToDo may be weighted means
        dataRnR = DataRun(debug = self.debug, args = self.args, rt = self.rt, logger = self.logger)
        self.sextract(fitsFns = fitsFns, dataRn = dataRnR)

        if not len(dataRnR.dataSxtrs):
            self.logger.warn('sextractRun: no results, returning')
            return None

        dataRnR.dataSxtrs.sort(key = lambda x: x.nObjs, reverse = True)
        fitsFnR = dataRnR.dataSxtrs[0].fitsFn
        self.logger.info('sextractRun: reference at FOC_POS: {0}, {1}'.format(dataRnR.dataSxtrs[0].focPos, fitsFnR))

        dSxR = None
        if self.rt.cfg['ANALYZE_ASSOC']:
            dSxR = self.createAssocList(fitsFn = fitsFnR)
            dataRn = DataRun(debug = self.debug, dSxReference = dSxR, args = self.args, rt = self.rt, logger = self.logger)
        else:
            dataRn = DataRun(debug = self.debug, args = self.args, rt = self.rt, logger = self.logger)

        self.sextract(fitsFns = fitsFns, dSxReference = dSxR, dataRn = dataRn)

        # removal of images with too few objects
        if self.rt.cfg['ANALYZE_ASSOC']:
            dataRn.onAlmostImagesAssoc()
        elif dSxR is not None:
            # the reference is needed for option --fraction
            dataRn.dSxReference=dSxR
            dataRn.onAlmostImages()

        if not dataRn.numberOfFocPos():
            return None 

        return dataRn

    # used in off line analysis only
    # used in off line analysis only
    def analyzeRuns(self):
        """Loop over self.fs and call analyzeRun(). Calculate and log the filter offsets.

        :returns: ResultFit
        :rtype: list

        """
        rFtsFwhm = list()
        rFtsFlux = list()
        for dftw, ft  in self.fS.iteritems():
            info = '{}: '.format(repr(dftw))
            # ToDo dictionary comprehension
            for k, fns in ft.iteritems():
                info += 'Ftname: {}, images: {}; '.format(k, len(fns))
                if len(fns) <= self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
                    self.logger.warn('analyzeRuns: to few DIFFERENT focuser positions: {0}<={1} (MINIMUM_FOCUSER_POSITIONS), continuing'.format(len(fns), self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']))
                    info += ', continuing'
                    continue

                dataRn = self.sextractRun(fitsFns = fns)
                if dataRn is None:
                    self.logger.warn('analyzeRuns: no results from sextract, breaking')
                    info += ', breaking'
                    break

                rFtFwhm, rMnsFwhm, rFtFlux, rMnsFlux = self.analyzeRun(dataRn = dataRn)
                if rFtFwhm is not None and rFtFwhm.accepted:
                    rFtsFwhm.append(rFtFwhm)

                if rFtFlux is not None and rFtFlux.accepted:
                    rFtsFlux.append(rFtFlux)

            self.logger.info( 'analyzeRuns: {}'.format(info))

        openMinFitPos = None
        # ToDo expand to Flux
        for rFt in rFtsFwhm:
            if rFt.ftName in self.rt.cfg['EMPTY_SLOT_NAMES']:
                openMinFitPos = int(rFt.extrFitPos)
                break
        else:
            # no empty slot found
            if len(rFtsFwhm) > 1:
                self.logger.warn('analyzeRuns: found {} fitted data sets, but no empty slot, do not calculate filter offsets'.format(len(rFtsFwhm)))

        # ToDo expand to Flux
        for rFt in rFtsFwhm:
            if rFt.extrFitPos is not None:
                if openMinFitPos is not None:
                    
                    self.logger.info('analyzeRuns: {0:5d} minPos, {1:5d}  offset, {2} ftName, {3} degC ambient temperature'.format( int(rFt.extrFitPos), int(rFt.extrFitPos)-openMinFitPos, rFt.ftName.rjust(8, ' '), rFt.ambientTemp))
                else:
                    self.logger.info('analyzeRuns: {0:5d} minPos, {1} ftName,  {2} degC ambient temperature'.format( int(rFt.extrFitPos),  rFt.ftName.rjust(8, ' '), rFt.ambientTemp))

                self.logger.info('analyzeRuns: fitPar {} '.format(rFt.fitPar))


        return rFtsFwhm, rFtsFlux

    # used in off line analysis only
    def aggregateRuns(self):
        """Find FITS files which form a focus run and store them in self.fs."""
        for root, dirnames, filenames in os.walk(self.args.basePath):
            fns = fnmatch.filter(filenames, '*.fits')
            if len(fns)==0:
                continue
            mFtws =  MANYFILTERWHEELS.match(root)
            oFtw  =  ONEFILTER.match(root)
            oFlt  =  ONLYFILTERS.match(root)
            d     =  DATE.match(root)
            FpFns = [os.path.join(root, fn) for fn in fns]
            
            if mFtws:
                # if filters are defined on the cmd line use these
                if self.args.filterNames:
                    if mFtws.group(3) in self.args.filterNames: 
                        self.fS[(mFtws.group(1), mFtws.group(2))] [mFtws.group(3)] = FpFns
                else:
                    self.fS[(mFtws.group(1), mFtws.group(2))] [mFtws.group(3)] = FpFns
            elif oFtw:
                self.fS[(oFtw.group(1), 'NOFTW')] [oFtw.group(2)] = FpFns
            elif oFlt:
                self.fS[('NODATE', 'NOFTW')] [oFlt.group(1)] = FpFns
            elif d:
                self.fS[('NODATE', 'NOFTW')] ['NOFT'] = FpFns
            

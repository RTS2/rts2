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

import fnmatch
import os
import re
import sys
import collections

from rts2saf.sextract import Sextract
from rts2saf.analyze import SimpleAnalysis,CatalogAnalysis
from rts2saf.temperaturemodel import TemperatureFocPosModel

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
manyFilterWheels= re.compile('.*/([0-9]{4,4}-[0-9]{2,2}-[0-9]{2,2}T[0-9]{2,2}\:[0-9]{2,2}\:[0-9]{2,2}\.[0-9]+)/(.+?)/(.+)')
#                                                                                                  date    ft
oneFilter= re.compile('.*/([0-9]{4,4}-[0-9]{2,2}-[0-9]{2,2}T[0-9]{2,2}\:[0-9]{2,2}\:[0-9]{2,2}\.[0-9]+)/(.+?)')
# no filter
#2013-09-04T22:18:35.077526/
date= re.compile('.*/([0-9]{4,4}-[0-9]{2,2}-[0-9]{2,2}T[0-9]{2,2}\:[0-9]{2,2}\:[0-9]{2,2}\.[0-9]+)')

class Do(object):
    def __init__(self, debug=False, basePath=None, args=None, rt=None, ev=None, logger=None):
        self.debug=debug
        self.basePath=basePath
        self.args=args
        self.rt=rt
        self.ev=ev
        self.logger=logger
        self.fS= AutoVivification()

    def buildCats(self, i_nmbrAssc=None, dataSex=None, dSxReference=None):
        i_nmbr=dSxReference.fields.index('NUMBER')
        cats=collections.defaultdict(int)
        # count the occurence of each object
        focPosS=collections.defaultdict(int)
        for nmbr in sorted([ int(x[i_nmbr]) for x in dSxReference.catalog ]):
            for dSx in dataSex:
                for nmbrAssc in [ int(x[i_nmbrAssc]) for x in dSx.catalog ]:
                    if nmbr == nmbrAssc:
                        cats[nmbr] +=1
                        focPosS[dSx.focPos] +=1
                        break

        return cats,focPosS

    def dropDSx(self, focPosS=None, dataSex=None, dSxReference=None):
        # drop those catalogs which have to few objects
        # this is a glitch in case two dSx are present with the same focPos
        for focPos, val in focPosS.iteritems():
            if val < self.args.fractObjs * len(dSxReference.catalog):
                # remove dSx
                dSxdrp=None
                for dSx in dataSex:
                    if focPos == dSx.focPos:
                        dSxdrp=dSx
                        break
                else:
                    continue
                dataSex.remove(dSxdrp)
                self.logger.warn('dropDSx: focuser position: {0:5d} dropped dSx, {1:5d}, {2:6.2f} {3:5d}'.format(int(focPos), val, self.args.fractObjs * len(dSxReference.catalog), len(dSxReference.catalog)))

        return dataSex

    def onAlmostImagesAssoc(self, dataSex=None, dSxReference=None):
        # ToDO clarify that -1
        i_nmbrAssc= -1 + dataSex[0].fields.index('NUMBER_ASSOC') 
        # build cats
        cats,focPosS=self.buildCats( i_nmbrAssc=i_nmbrAssc, dataSex=dataSex, dSxReference=dSxReference)
        # drop
        dataSex= self.dropDSx(focPosS=focPosS, dataSex=dataSex, dSxReference=dSxReference)
        # rebuild the cats
        cats, focPosS=self.buildCats(i_nmbrAssc=i_nmbrAssc, dataSex=dataSex, dSxReference=dSxReference)

        foundOnAll=len(dataSex)
        assocObjNmbrs=list()
        # identify those objects which are on all images
        for k  in  cats.keys():
            if cats[k] == foundOnAll:
                assocObjNmbrs.append(k) 

        # save the raw values for later analysis
        # initialize data.catalog
        for dSx in dataSex:
            dSx.toRawCatalog()
        # copy those catalog entries which are found on all
        for k  in  assocObjNmbrs:
            for dSx in dataSex:
                for sx in dSx.rawCatalog:
                    if k == sx[i_nmbrAssc]:
                        dSx.catalog.append(sx)
                        break
                else:
                    self.logger.debug('onAllImages: no break for object: {0}'.format(k))

        # recalculate FWHM, Flux etc.
        i_fwhm= dataSex[0].fields.index('FWHM_IMAGE') 

        i_flux=None
        if self.args.flux:
            i_flux= dataSex[0].fields.index('FLUX_MAX') 

        for dSx in dataSex:
            dSx.fillData(i_fwhm=i_fwhm, i_flux=i_flux)

            if self.args.flux:
                if self.debug: self.logger.debug('onAllImages: {0:5d} {1:8.3f} {2:8.3f} {3:5d}'.format(int(dSx.focPos), dSx.fwhm, dSx.flux, len(dSx.catalog)))
            else:
                if self.debug: self.logger.debug('onAllImages: {0:5d} {1:8.3f} {2:5d}'.format(int(dSx.focPos), dSx.fwhm, len(dSx.catalog)))

        self.logger.info('onAlmostImages: objects: {0}'.format(len(assocObjNmbrs)))

        return dataSex

    def onAlmostImages(self, dataSex=None, dSxReference=None):

        focPosS=collections.defaultdict(int)
        for dSx in dataSex:
            focPosS[dSx.focPos]=dSx.nObjs

        dataSex= self.dropDSx(focPosS=focPosS, dataSex=dataSex, dSxReference=dSxReference)

        return dataSex

    def sextractLoop(self, fitsFns=None, assocFn=None):

        dataSex=list()
        for fitsFn in fitsFns:
            rsx= Sextract(debug=args.sexDebug, rt=rt, logger=self.logger)
            if self.args.flux:
                rsx.appendFluxFields()
                
            if assocFn:
                rsx.appendAssocFields()

            dSx=rsx.sextract(fitsFn=fitsFn, assocFn=assocFn)

            if dSx!=None and dSx.fwhm>0. and dSx.stdFwhm>0.:
                dataSex.append(dSx)
            else:
                self.logger.warn('sextractLoop: no result: file: {0}'.format(fitsFn))

        return dataSex

    def analyzeRun(self, fitsFns=None):

        # define the reference FITS as the one with the most sextracted objects
        # ToDo may be weighted means
        dataSex=self.sextractLoop(fitsFns=fitsFns)

        dataSex.sort(key = lambda x: x.nObjs, reverse=True)
        fitsFn=dataSex[0].fitsFn
        self.logger.info('analyzeRun: reference at FOC_POS: {0}, {1}'.format(dataSex[0].focPos, fitsFn))
        
        rsxR= Sextract(debug=args.sexDebug, createAssoc=self.args.associate, rt=rt, logger=self.logger)
        if args.flux:
            rsxR.appendFluxFields()

        assocFn=None
        if self.args.associate:
            assocFn='/tmp/assoc.lst'

        dSxReference=rsxR.sextract(fitsFn=fitsFn, assocFn=assocFn)

        dataSex=self.sextractLoop(fitsFns=fitsFns, assocFn=assocFn)

        pos=collections.defaultdict(int)
        for dSx in dataSex:
            pos[dSx.focPos] += 1 

        if len(pos) <= self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
            self.logger.warn('analyzeRun: to few DIFFERENT focuser positions: {0}<={1} (see MINIMUM_FOCUSER_POSITIONS), continuing'.format(len(pos), self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']))
            if self.debug:
                for p,v in pos.iteritems():
                    self.logger.debug('analyzeRun:{0:5.0f}: {1}'.format(p,v))
            return 

        if self.args.associate:
            dataSex=self.onAlmostImagesAssoc(dataSex=dataSex, dSxReference=dSxReference)
        else:
            dataSex=self.onAlmostImages(dataSex=dataSex, dSxReference=dSxReference)


        if args.catalogAnalysis:
            an=CatalogAnalysis(debug=self.debug, dataSex=dataSex, Ds9Display=self.args.Ds9Display, FitDisplay=self.args.FitDisplay, focRes=float(self.rt.cfg['FOCUSER_RESOLUTION']), moduleName=args.criteria, ev=self.ev, rt=rt, logger=self.logger)
            rFt=an.selectAndAnalyze()
        else:
            an=SimpleAnalysis(debug=self.debug, dataSex=dataSex, Ds9Display=self.args.Ds9Display, FitDisplay=self.args.FitDisplay, focRes=float(self.rt.cfg['FOCUSER_RESOLUTION']), ev=self.ev, logger=self.logger)
            rFt=an.analyze()
            an.display()
        if not rFt!=None:
            self.logger.info('analyzeRun: result: wMObjects: {0:5.0f}, wMCombined:{1:5.0f}, wMStdFwhm:{1:5.0f}, minFitPos: {2:5.0f}, minFitFwhm: {3:5.0f}'.format(rFt.weightedMeanObjects, rFt.weightedMeanCombined, rFt.weightedMeanStdFwhm, rFt.minFitPos, rFt.minFitFwhm))
        return rFt

    def analyzeRuns(self):
        rFts=list()
        for dftw, ft  in self.fS.iteritems():
            info='{} :: '.format(repr(dftw))
            out=False
            # ToDo dictionary comprehension
            for k,fns in ft.iteritems():
                info += '{} {} '.format(k,len(fns))
                if len(fns) >self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
                    out=True
                    rFt, rMns=self.analyzeRun(fitsFns=fns)
                    if rFt:
                        rFts.append(rFt)
            if out:
                self.logger.info( 'analyzeRuns: {}'.format(info))

        openMinFitPos=None
        for rFt in rFts:
            if rFt.ftName in self.rt.cfg['EMPTY_SLOT_NAMES']:
                openMinFitPos=int(rFt.minFitPos)
                break
        else:
            # is not a filter wheel run 
            return rFts

        for rFt in rFts:
            logger.info('analyzeRuns: {0:5d} minPos, {1:5d}  offset, {2} ftName'.format( int(rFt.minFitPos), int(rFt.minFitPos)-openMinFitPos, rFt.ftName.rjust(8, ' ')))

        return rFts

    def aggregateRuns(self):
        for root, dirnames, filenames in os.walk(self.args.basePath):
            fns=fnmatch.filter(filenames, '*.fits')
            if len(fns)==0:
                continue
            mFtws= manyFilterWheels.match(root)
            oFtw = oneFilter.match(root)
            d    = date.match(root)
            FpFns=[os.path.join(root, fn) for fn in fns]
            if mFtws:

                if self.args.filterNames:
                    if mFtws.group(3) in self.args.filterNames: 
                        self.fS[(mFtws.group(1), mFtws.group(2))] [mFtws.group(3)]=FpFns
                else:
                    # print 'wheels', root
                    self.fS[(mFtws.group(1), mFtws.group(2))] [mFtws.group(3)]=FpFns
            elif oFtw:
                #print 'one filter', root
                self.fS[(oFtw.group(1), 'NOFTW')] [oFtw.group(2)]=FpFns
            elif d:
                #print 'dateofits', root
                self.fS[(d.group(1), 'NOFTW')] ['NOFT']=FpFns
            else:
                #print 'else/fits', root
                self.fS[('NODATE', 'NOFTW')] ['NOFT']=FpFns
            

if __name__ == '__main__':

    import argparse
    from rts2saf.config import Configuration
    from rts2saf.environ import Environment
    from rts2saf.log import Logger

    prg= re.split('/', sys.argv[0])[-1]
    parser= argparse.ArgumentParser(prog=prg, description='rts2asaf analysis')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--sexdebug', dest='sexDebug', action='store_true', default=False, help=': %(default)s,add more output on SExtract')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--topath', dest='toPath', metavar='PATH', action='store', default='.', help=': %(default)s, write log file to path')
    parser.add_argument('--logfile',dest='logfile', default='{0}.log'.format(prg), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--config', dest='config', action='store', default='/usr/local/etc/rts2/rts2saf/rts2saf.cfg', help=': %(default)s, configuration file path')
    parser.add_argument('--basepath', dest='basePath', action='store', default=None, help=': %(default)s, directory where FITS images from possibly many focus runs are stored')
    parser.add_argument('--filternames', dest='filterNames', action='store', default=None, type=str, nargs='+', help=': %(default)s, list of filters to analyzed, None: all')
#ToDo    parser.add_argument('--ds9region', dest='ds9region', action='store_true', default=False, help=': %(default)s, create ds9 region files')
    parser.add_argument('--ds9display', dest='Ds9Display', action='store_true', default=False, help=': %(default)s, display fits images and region files')
    parser.add_argument('--fitdisplay', dest='FitDisplay', action='store_true', default=False, help=': %(default)s, display fit')
    parser.add_argument('--cataloganalysis', dest='catalogAnalysis', action='store_true', default=False, help=': %(default)s, ananlys is done with CatalogAnalysis')
    parser.add_argument('--criteria', dest='criteria', action='store', default='rts2saf.criteria_radius', help=': %(default)s, CatalogAnalysis criteria Python module to load at run time')
    parser.add_argument('--associate', dest='associate', action='store_true', default=False, help=': %(default)s, let sextractor associate the objects among images')
    parser.add_argument('--flux', dest='flux', action='store_true', default=False, help=': %(default)s, do flux analysis')
    parser.add_argument('--model', dest='model', action='store_true', default=False, help=': %(default)s, fit temperature model')
    parser.add_argument('--fraction', dest='fractObjs', action='store', default=0.5, type=float, help=': %(default)s, fraction of objects which must be present on each image, base: object number on reference image, this option is used only together with --associate')

    args=parser.parse_args()

    if args.debug:
        args.level= 'DEBUG'
        args.toconsole=True

    # logger
    logger= Logger(debug=args.debug, args=args).logger # if you need to chage the log format do it here
    # config
    rt=Configuration(logger=logger)
    rt.readConfiguration(fileName=args.config)
    rt.checkConfiguration()
    # environment
    ev=Environment(debug=args.debug, rt=rt,logger=logger)

    if not args.basePath:
        parser.print_help()
        logger.warn('rts2saf_analyze: no --basepath specified')
        sys.exit(1)

    if not args.toconsole:
        print 'you may wish to enable logging to console --toconsole'
        print 'log file is writte to: {}'.format(args.logfile)


    do=Do(debug=args.debug, basePath=args.basePath, args=args, rt=rt, ev=ev, logger=logger)
    do.aggregateRuns()
    if len(do.fS)==0:
        logger.warn('rts2saf_analyze: exiting, no files found in basepath: {}'.format(args.basePath))
        sys.exit(1)

    rFF=do.analyzeRuns()
    if rFF[0].ambientTemp in 'NoTemp':
        logger.warn('rts2saf_analyze: no ambient temperature available in FITS files, no model fitted')
    else:
        if args.model:
            # temperature model
            plotFn=ev.expandToPlotFileName( plotFn='{}/temp-model.png'.format(args.basePath))
            dom=TemperatureFocPosModel(showPlot=True, date=ev.startTime[0:19],  comment='test run', plotFn=plotFn, resultFitFwhm=rFF, logger=logger)
            dom.fitData()
            dom.plotData()
            logger.info('rts2saf_analyze: storing plot at: {}'.format(plotFn))
        

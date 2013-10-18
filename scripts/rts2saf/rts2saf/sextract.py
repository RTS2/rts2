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
from astropy.io.fits import getheader
import sys
import time
# ToDo sort that out with Petr import rts2.sextractor as rsx
import rts2saf.sextractor as rsx
import rts2saf.data as dt
import rts2saf.fitfwhm as ft
import rts2saf.ds9region as ds9r

class Sextract(object):
    """Sextract a FITS image"""
    def __init__(self, debug=False, rt=None, logger=None):
        self.debug=debug
        self.rt= rt
        self.sexpath=rt.cfg['SEXPATH']
        self.sexconfig=rt.cfg['SEXCFG']
        self.starnnw=rt.cfg['STARNNW_NAME']
        self.fields=rt.cfg['FIELDS']
        self.nbrsFtwsInuse=len(rt.cfg['FILTER WHEELS IN USE'])
        self.logger=logger


    def sextract(self, fitsFn):

        sex = rsx.Sextractor(fields=self.fields,sexpath=self.sexpath,sexconfig=self.sexconfig,starnnw=self.starnnw)
        stde=sex.runSExtractor(fitsFn)
        if stde:
            self.logger.error( 'sextract: {0} not found, sextractor message:\n{1}\nexiting'.format(fitsFn,stde))
            return dt.DataSex()

        # find the sextractor counts
        objectCount = len(sex.objects)

        try:
            hdr = getheader(fitsFn, 0)
        except Exception, e:
            self.logger.error( 'sextract: FITS {0} \nmessage: {1} returning'.format(fitsFn, e))
            return dt.DataSex()

        try:
            focPos = float(hdr['FOC_POS'])
        except Exception, e:
            self.logger.error( 'sextract: in FITS {0} key word FOC_POS not found\nmessage: {1} returning'.format(fitsFn, e))
            return dt.DataSex()

        # these values are remaped in config.py
        try:
            ambientTemp = '{0:3.1f}'.format(float(hdr[self.rt.cfg['AMBIENTTEMPERATURE']]))
        except:
            # that is not required
            ambientTemp='NoTemp'

        try:
            binning = float(hdr[self.rt.cfg['BINNING']])
        except:
            # if CatalogAnalysis is done
            binning=None

        binningXY=None
        if not binning:
            binningXY=list()
            try:
                binningXY.append(float(hdr[self.rt.cfg['BINNING_X']]))
            except:
                # if CatalogAnalysis is done
                if self.debug: self.logger.warn( 'sextract: no x-binning information found, {0}'.format(fitsFn, focPos, objectCount))

            try:
                binningXY.append(float(hdr[self.rt.cfg['BINNING_Y']]))
            except:
                # if CatalogAnalysis is done
                if self.debug: self.logger.warn( 'sextract: no y-binning information found, {0}'.format(fitsFn, focPos, objectCount))

            if len(binningXY) < 2:
                if self.debug: self.logger.warn( 'sextract: no binning information found, {0}'.format(fitsFn, focPos, objectCount))

        try:
            naxis1 = float(hdr['NAXIS1'])
        except:
            self.logger.warn( 'sextract: no NAXIS1 information found, {0}'.format(fitsFn, focPos, objectCount))
            naxis1=None
        try:
            naxis2 = float(hdr['NAXIS2'])
        except:
            self.logger.warn( 'sextract: no NAXIS2 information found, {0}'.format(fitsFn, focPos, objectCount))
            naxis2=None
        try:
            ftName = hdr['FILTER']
        except:
            self.logger.warn( 'sextract: no filter name information found, {0}'.format(fitsFn, focPos, objectCount))
            ftName=None
        # ToDo clumsy
        ftAName=None
        if self.nbrsFtwsInuse > 0:
            try:
                ftAName = hdr['FILTA']
            except:
                self.logger.warn( 'sextract: no FILTA name information found, {0}'.format(fitsFn, focPos, objectCount))

        # ToDo clumsy
        ftBName=None
        if self.nbrsFtwsInuse > 1:
            try:
                ftBName = hdr['FILTB']
            except:
                self.logger.warn( 'sextract: no FILTB name information found, {0}'.format(fitsFn, focPos, objectCount))

        # ToDo clumsy
        ftCName=None
        if self.nbrsFtwsInuse > 2:
            try:
                ftCName = hdr['FILTC']
            except:
                self.logger.warn( 'sextract: no FILTC name information found, {0}'.format(fitsFn, focPos, objectCount))

        try:
            fwhm,stdFwhm,nstars=sex.calculate_FWHM(filterGalaxies=False)
        except Exception, e:
            self.logger.warn( 'sextract: focPos: {0:5.0f}, raw objects: {1}, no objects found, {0} (after filtering), {2}, \nmessage rts2saf.sextractor: {3}'.format(focPos, objectCount, fitsFn, e))
            return dt.DataSex()

        # store results
        dataSex=dt.DataSex(fitsFn=fitsFn, focPos=focPos, fwhm=float(fwhm), stdFwhm=float(stdFwhm),nstars=int(nstars), ambientTemp=ambientTemp, catalog=sex.objects, binning=binning, binningXY=binningXY, naxis1=naxis1, naxis2=naxis2,fields=self.fields, ftName=ftName, ftAName=ftAName, ftBName=ftBName, ftCName=ftCName)
        if self.debug: self.logger.debug( 'sextract: {0} {1:5.0f} {2:4d} {3:5.1f} {4:5.1f} {5:4d}'.format(fitsFn, focPos, len(sex.objects), fwhm, stdFwhm, nstars))

        return dataSex

if __name__ == '__main__':

    import sys
    import os
    import re
    import numpy as np
    import time
    import argparse
    import glob
    import logging
    import rts2saf.config as cfgd
    import rts2saf.log as lg


    prg= re.split('/', sys.argv[0])[-1]
    parser= argparse.ArgumentParser(prog=prg, description='rts2asaf analysis')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--topath', dest='toPath', metavar='PATH', action='store', default='.', help=': %(default)s, write log file to path')
    parser.add_argument('--logfile',dest='logfile', default='{0}.log'.format(prg), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--dryfitsfiles', dest='dryfitsfiles', action='store', default='../samples', help=': %(default)s, directory where a FITS files are stored from a earlier focus run')
    # ToDo    parser.add_argument('--ds9region', dest='ds9region', action='store_true', default=False, help=': %(default)s, create ds9 region files')
    parser.add_argument('--ds9display', dest='ds9display', action='store_true', default=False, help=': %(default)s, display fits images and region files')
    parser.add_argument('--config', dest='config', action='store', default='/usr/local/etc/rts2/rts2saf/rts2saf.cfg', help=': %(default)s, configuration file path')

    args=parser.parse_args()

    lgd= lg.Logger(debug=args.debug, args=args) # if you need to chage the log format do it here
    logger= lgd.logger 

    rt=cfgd.Configuration(logger=logger)
    rt.readConfiguration(fileName=args.config)

    dryFitsFiles=glob.glob('{0}/{1}'.format(args.dryfitsfiles, rt.cfg['FILE_GLOB']))

    if len(dryFitsFiles)==0:
        logger.error('analyze: no FITS files found in:{}'.format(args.dryfitsfiles))
        logger.info('analyze: set --dryfitsfiles or'.format(args.dryfitsfiles))
        logger.info('analyze: download a sample from wget http://azug.minpet.unibas.ch/~wildi/rts2saf-test-focus-2013-09-14.tgz')
        logger.info('analyze: and store it in directory: {0}'.format(args.dryfitsfiles))
        sys.exit(1)

    
    dataSex=dict()
    for k, fitsFn in enumerate(dryFitsFiles):
        print fitsFn
        sx= Sextract(debug=args.debug, rt=rt, logger=logger)
        
        dSx=sx.sextract(fitsFn=fitsFn) 
        try:
            dSx=sx.sextract(fitsFn=fitsFn) 
        except Exception, e:
            logger.error('sextract: error with sextract:\nrts2saf.sextractor: {0}'.format(e))

        if dSx and dSx.fwhm>0. and dSx.stdFwhm>0.:
            dataSex[k]=dSx 
        else:
            print 'dropped: {}'.format(dSx.fitsFn)

    # very ugly
    nobjs=list()
    pos=list()
    fwhm=list()
    errx=list()
    stdFwhm=list()

    for k  in dataSex.keys():
        # all sextracted objects
        nObjs= len(dataSex[k].catalog)
        print dataSex[k].focPos, dataSex[k].fwhm
        # star like objects
        nobjs.append(dataSex[k].nstars)
        pos.append(dataSex[k].focPos)
        fwhm.append(dataSex[k].fwhm)
        errx.append(20.)
        stdFwhm.append(dataSex[k].stdFwhm)
    # Weighted mean based on number of extracted objects (stars)
    weightedMean= np.average(a=pos, axis=0, weights=nobjs) 
    print '--------weightedMean'
    print weightedMean

    # Weighted mean based on median FWHM
    posC= pos[:]
    fwhmC= fwhm[:]
    while True:
        try:
            ind=fwhmC.index(0.)
        except:
            break
        del posC[ind]
        del fwhmC[ind]

    weightedMean= np.average(a=posC, axis=0, weights=map( lambda x: 1./x, fwhmC)) 
    print '--------weightedMeanFwhm'
    print weightedMean

    # Fit median FWHM data
    dataFitFwhm=dt.DataFitFwhm(plotFn='./sextract-plot.png',ambientTemp='-271.3', ftName='SX', pos=np.asarray(pos),fwhm=np.asarray(fwhm),errx=np.asarray(errx),stdFwhm=np.asarray(stdFwhm))
    # ToDo mismatch!!
    fit=ft.FitFwhm(showPlot=False, date='2013-09-08T09:30:09', comment='Test sextract', dataFitFwhm=dataFitFwhm, logger=logger)
    minFitPos, minFitFwhm, fitPar= fit.fitData()
    print '--------minFwhm fwhm'
    print minFitPos, minFitFwhm

    fit.plotData()
    # plot them through ds9
    if args.ds9display:
        display=ds9()

        for k in dataSex.keys():
            dr=ds9r.Ds9Region( dataSex=dataSex[k], display=display)
            dr.displayWithRegion()
            time.sleep(1.)

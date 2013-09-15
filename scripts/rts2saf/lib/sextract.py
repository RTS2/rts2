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
import pyfits

import rts2.sextractor as rsx
try:
    import lib.data as dt
except:
    import data as dt
try:
    import lib.fitfwhm as ft
except:
    import fitfwhm as ft

try:
    import lib.ds9region as ds9r
except:
    import ds9region as ds9r


class Sextract(object):
    """Sextract a FITS image"""
    def __init__(self, debug=False, rt=None, logger=None):
        self.debug=debug
        self.rt= rt
        self.sexpath=rt.cfg['SEXPATH']
        self.sexconfig=rt.cfg['SEXCFG']
        self.starnnw=rt.cfg['STARNNW_NAME']
        self.fields=rt.cfg['SEX FIELDS']
        self.logger=logger

    def sextract(self, fitsFn):
        # ToDo: flexible
        sex = rsx.Sextractor(fields=self.fields,sexpath=self.sexpath,sexconfig=self.sexconfig,starnnw=self.starnnw)
        sex.runSExtractor(fitsFn)

        # find the sextractor counts
        objectCount = len(sex.objects)
        focPos = int(pyfits.getval(fitsFn,'FOC_POS'))
       
        # store results
        try:
            fwhm,stdFwhm,nstars=sex.calculate_FWHM(filterGalaxies=False)
        except Exception, e:
            self.logger.warn( 'sextract: {0}: {1:5d}, no objects found:\n{2}'.format(fitsFn, focPos, e))
            return None

        dataSex=dt.DataSex(fitsFn=fitsFn, focPos=focPos, fwhm=fwhm, stdFwhm=stdFwhm,nstars=nstars, catalogue=sex.objects, fields=self.fields)
        if self.debug: self.logger.debug( 'sextract: {0} {1:5d} {2:4d} {3:5.1f} {4:5.1f} {5:4d}'.format(fitsFn, focPos, len(sex.objects), fwhm, stdFwhm, nstars))

        return dataSex


if __name__ == '__main__':

    import sys
    import os
    import numpy as np
    import time
    import argparse
    import logging
    try:
        import lib.config as cfgd
    except:
        import config as cfgd

    parser= argparse.ArgumentParser(prog=sys.argv[0], description='rts2asaf analysis')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--logfile',dest='logfile', default='/tmp/{0}.log'.format(sys.argv[0]), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('fitsFiles', metavar='fits', nargs='*', type=str, default=None, help=': %(default)s, fits files to process')
    # ToDo    parser.add_argument('--ds9region', dest='ds9region', action='store_true', default=False, help=': %(default)s, create ds9 region files')
    parser.add_argument('--ds9display', dest='ds9display', action='store_true', default=False, help=': %(default)s, display fits images and region files')
    parser.add_argument('--config', dest='config', action='store', default='./rts2saf-my.cfg', help=': %(default)s, configuration file path')

    args=parser.parse_args()

    logformat= '%(asctime)s:%(name)s:%(levelname)s:%(message)s'
    logging.basicConfig(filename=args.logfile, level=args.level.upper(), format= logformat)
    logger = logging.getLogger()

    if args.level in 'DEBUG' or args.level in 'INFO':
        toconsole=True
    else:
        toconsole=args.toconsole

    if toconsole:
    #http://www.mglerner.com/blog/?p=8
        soh = logging.StreamHandler(sys.stdout)
        soh.setLevel(args.level)
        logger.addHandler(soh)

    rt=cfgd.Configuration(logger=logger)
    rt.readConfiguration(fileName=args.config)

    if len(args.fitsFiles)==0:
        dirname='./samples'
        fitsFiles=list()
        for path, dirs, files in os.walk(dirname):
            for file in files:
                fitsFiles.append(os.path.join(path, file))
    else:
        fitsFiles= args.fitsFiles.split()

    cnt=0
    dataSex=dict()
    for fitsFn in fitsFiles:
        print fitsFn
        sx= Sextract(debug=args.debug, rt=rt, logger=logger)
        dataSex[cnt]=sx.sextract(fitsFn=fitsFn) 
        cnt +=1

    # very ugly
    nobjs=list()
    pos=list()
    fwhm=list()
    errx=list()
    stdFwhm=list()

    for cnt in dataSex.keys():
        # all sextracted objects
        nObjs= len(dataSex[cnt].catalogue)
        print dataSex[cnt].focPos, dataSex[cnt].fwhm
        # star like objects
        nobjs.append(dataSex[cnt].nstars)
        pos.append(dataSex[cnt].focPos)
        fwhm.append(dataSex[cnt].fwhm)
        errx.append(20.)
        stdFwhm.append(dataSex[cnt].stdFwhm)
    # Weighted mean based on number of extracted objects (stars)
    weightedMean= np.average(a=pos, axis=0, weights=nobjs) 
    print '--------'
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
    print '--------'
    print weightedMean

    # Fit median FWHM data
    dataFwhm=dt.DataFwhm(pos=np.asarray(pos),fwhm=np.asarray(fwhm),errx=np.asarray(errx),stdFwhm=np.asarray(stdFwhm))

    fit=ft.FitFwhm(showPlot=True, filterName='U', objects=10, temperature=20., date='2013-09-08T09:30:09', comment='Test', pltFile='./test.png', dataFwhm=dataFwhm)
    fit.fitData()
    fit.plotData()

    # plot them through ds9
    if args.ds9display:
        display=ds9()

        for cnt in dataSex.keys():
            dr=ds9r.Ds9Region( dataSex=dataSex[cnt], display=display)
            dr.displayWithRegion()
            time.sleep(1.)

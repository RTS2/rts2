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
import pyfits

# ToDo sort that out with Petrimport rts2.sextractor as rsx
try:
    import lib.sextractor as sex
except:
    import sextractor  as sex
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
        self.fields=rt.cfg['FIELDS']
        self.logger=logger


    def sextract(self, fitsFn):

        sex = rsx.Sextractor(fields=self.fields,sexpath=self.sexpath,sexconfig=self.sexconfig,starnnw=self.starnnw)
        sex.runSExtractor(fitsFn)

        # find the sextractor counts
        objectCount = len(sex.objects)
        focPos = int(pyfits.getval(fitsFn,'FOC_POS'))
       
        # store results
        try:
            fwhm,stdFwhm,nstars=sex.calculate_FWHM(filterGalaxies=False)
        except Exception, e:
            self.logger.warn( 'sextract: {0}: {1:5d}, raw objects: {2}, no objects found (after filtering), \nmessage rts2.sextractor: {2}'.format(fitsFn, focPos, objectCount, e))
            self.logger.warn( 'sextract: new version of rts2.sextractor installed???')
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
    import glob
    import logging
    try:
        import lib.config as cfgd
    except:
        import config as cfgd
    try:
        import lib.log as lg
    except:
        import log as lg


    parser= argparse.ArgumentParser(prog=sys.argv[0], description='rts2asaf analysis')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--logfile',dest='logfile', default='/tmp/{0}.log'.format(sys.argv[0]), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--dryfitsfiles', dest='dryfitsfiles', action='store', default='./samples', help=': %(default)s, directory where a FITS files are stored from a earlier focus run')
    # ToDo    parser.add_argument('--ds9region', dest='ds9region', action='store_true', default=False, help=': %(default)s, create ds9 region files')
    parser.add_argument('--ds9display', dest='ds9display', action='store_true', default=False, help=': %(default)s, display fits images and region files')
    parser.add_argument('--config', dest='config', action='store', default='/etc/rts2/rts2saf/rts2saf.cfg', help=': %(default)s, configuration file path')

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
    for k, fitsFn in enumerate(fitsFiles):
        print fitsFn
        sx= Sextract(debug=args.debug, rt=rt, logger=logger)
        try:
            dataSex[k]=sx.sextract(fitsFn=fitsFn) 
        except Exception, e:
            logger.error('sextract: error with sextract:\nrts2.sextractor: {0}'.format(e))
    # very ugly
    nobjs=list()
    pos=list()
    fwhm=list()
    errx=list()
    stdFwhm=list()

    for k  in dataSex.keys():
        # all sextracted objects
        nObjs= len(dataSex[k].catalogue)
        print dataSex[k].focPos, dataSex[k].fwhm
        # star like objects
        nobjs.append(dataSex[k].nstars)
        pos.append(dataSex[k].focPos)
        fwhm.append(dataSex[k].fwhm)
        errx.append(20.)
        stdFwhm.append(dataSex[k].stdFwhm)
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

    fit=ft.FitFwhm(showPlot=True, filterName='U', temperature=20., date='2013-09-08T09:30:09', comment='Test sextract', pltFile='./test-sextract.png', dataFwhm=dataFwhm)
    fit.fitData()
    fit.plotData()

    # plot them through ds9
    if args.ds9display:
        display=ds9()

        for k in dataSex.keys():
            dr=ds9r.Ds9Region( dataSex=dataSex[k], display=display)
            dr.displayWithRegion()
            time.sleep(1.)

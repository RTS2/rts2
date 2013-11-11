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
from rts2saf.data import DataSex

class Sextract(object):
    """Sextract a FITS image"""
    def __init__(self, debug=False, createAssoc=False, rt=None, logger=None):
        self.debug=debug
        self.createAssoc=createAssoc
        self.rt= rt
        self.sexpath=rt.cfg['SEXPATH']
        self.sexconfig=rt.cfg['SEXCFG']
        self.starnnw=rt.cfg['STARNNW_NAME']
        self.fields=rt.cfg['FIELDS'][:] # it is a reference, gell :-))
        self.nbrsFtwsInuse=len(rt.cfg['FILTER WHEELS INUSE'])
        self.stdFocPos=rt.cfg['FOCUSER_RESOLUTION']
        self.logger=logger
        
    def appendFluxFields(self):
        self.fields.append('FLUX_MAX')
        self.fields.append('FLUX_APER')
        self.fields.append('FLUXERR_APER')

    def appendAssocFields(self):
        self.fields.append('VECTOR_ASSOC({0:d})'.format(len(self.fields)))
        self.fields.append('NUMBER_ASSOC')

    def sextract(self, fitsFn, assocFn=None):
        sex = rsx.Sextractor(fields=self.fields,sexpath=self.sexpath,sexconfig=self.sexconfig,starnnw=self.starnnw, createAssoc=self.createAssoc)
        # create the skyList
        stde= sex.runSExtractor(fitsFn, assocFn=assocFn)

        if stde:
            self.logger.error( 'sextract: {0} not found, sextractor message:\n{1}\nreturning'.format(fitsFn,stde))
            return DataSex()

        # find the sextractor counts
        objectCount = len(sex.objects)

        try:
            hdr = getheader(fitsFn, 0)
        except Exception, e:
            self.logger.error( 'sextract: FITS {0} \nmessage: {1} returning'.format(fitsFn, e))
            return DataSex()

        try:
            focPos = float(hdr['FOC_POS'])
        except Exception, e:
            self.logger.error( 'sextract: in FITS {0} key word FOC_POS not found\nmessage: {1} returning'.format(fitsFn, e))
            return DataSex()

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

        try:
            date = hdr['DATE'] # DATE-OBS
        except:
            self.logger.warn( 'sextract: no date information found, {0}'.format(fitsFn, focPos, objectCount))
            date=None


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
            return DataSex()

        # store results
        dataSex=DataSex(
            date=date,
            fitsFn=fitsFn, 
            focPos=focPos, 
            stdFocPos=float(self.stdFocPos), # nothing real...
            fwhm=float(fwhm), 
            stdFwhm=float(stdFwhm),
            nstars=int(nstars), 
            ambientTemp=ambientTemp, 
            catalog=sex.objects, 
            binning=binning, 
            binningXY=binningXY, 
            naxis1=naxis1, 
            naxis2=naxis2,
            fields=self.fields, 
            ftName=ftName, 
            ftAName=ftAName, 
            ftBName=ftBName, 
            ftCName=ftCName,
            assocFn=assocFn)

        try:
            i_flux = dataSex.fields.index('FLUX_MAX')
            dataSex.fillFlux(i_flux=i_flux)
        except:
            if self.debug: self.logger.debug( 'sextract: no FLUX_MAX available: {0}'.format(fitsFno))

        if self.debug: self.logger.debug( 'sextract: {0} {1:5.0f} {2:4d} {3:5.1f} {4:5.1f} {5:4d}'.format(fitsFn, focPos, len(sex.objects), fwhm, stdFwhm, nstars))

        return dataSex

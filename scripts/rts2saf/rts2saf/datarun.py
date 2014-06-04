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

import collections

class DataRun(object):
    def __init__(self, debug=False, dSxReference=None, args=None, rt=None, logger=None):
        self.debug=debug
        self.args=args
        self.rt=rt
        self.logger=logger
        self.dataSxtrs=list()
        self.dSxReference=dSxReference
        self.assocObjNmbrs=list()


    def buildCats(self, i_nmbrAssc=None):
        i_nmbr=self.dSxReference.fields.index('NUMBER')
        cats=collections.defaultdict(int)
        # count the occurrence of each object
        focPosS=collections.defaultdict(int)
        nmbrs=sorted([ int(x[i_nmbr]) for x in self.dSxReference.catalog ])
        for dSx in self.dataSxtrs:
            for nmbr in nmbrs:

                for nmbrAssc in [ int(x[i_nmbrAssc]) for x in dSx.catalog ]:
                    if nmbr == nmbrAssc:
                        cats[nmbr] +=1
                        focPosS[dSx.focPos] +=1
                        break

        return cats,focPosS

    def dropDSx(self, focPosS=None):
        # drop those catalogs which have to few objects
        # this is a glitch in case two dSx are present with the same focPos
        for focPos, val in focPosS.iteritems():
            if val < self.rt.cfg['ANALYZE_ASSOC_FRACTION'] * len(self.dSxReference.catalog):
                # remove dSx
                dSxdrp=None
                for dSx in self.dataSxtrs:
                    if focPos == dSx.focPos:
                        dSxdrp=dSx
                        break
                else:
                    continue
                self.dataSxtrs.remove(dSxdrp)
                self.logger.warn('dropDSx: focuser position: {0:5d} dropped dSx, sextracted objects: {1:5d}, required objects: {2:6.2f}, total objects reference: {3:5d}'.format(int(focPos), val, self.rt.cfg['ANALYZE_ASSOC_FRACTION'] * len(self.dSxReference.catalog), len(self.dSxReference.catalog)))

    def onAlmostImagesAssoc(self):
        # ToDo clarify that -1
        # ['NUMBER', 'EXT_NUMBER', 'X_IMAGE', 'Y_IMAGE', 'MAG_BEST', 
        #  'FLAGS', 'CLASS_STAR', 'FWHM_IMAGE', 'A_IMAGE', 'B_IMAGE', 
        # 'FLUX_MAX', 'FLUX_APER', 'FLUXERR_APER', 'VECTOR_ASSOC(13)', 
        # 'NUMBER_ASSOC']
        # data from VECTOR_ASSOC appears behind NUMBER_ASSOC
        # ToDo interchange them
        #
        i_nmbrAssc= -1 + self.dataSxtrs[0].fields.index('NUMBER_ASSOC') 
        # build cats
        cats,focPosS=self.buildCats( i_nmbrAssc=i_nmbrAssc)
        # drop those which have not enough sextracted objects
        self.dropDSx(focPosS=focPosS)
        # rebuild the cats
        cats, focPosS=self.buildCats(i_nmbrAssc=i_nmbrAssc)

        remainingCats=len(self.dataSxtrs)
        # identify those objects which are on all images
        for k  in  cats.keys():
            if cats[k] == remainingCats:
                self.assocObjNmbrs.append(k) 

        # copy those catalog entries (sextracted objects) which are found on all images
        for dSx in self.dataSxtrs:
            # save the original cleaned values for later analysis
            # initialize data.catalog
            dSx.toReducedCatalog()
            for sx in dSx.reducedCatalog:
                if sx[i_nmbrAssc] in self.assocObjNmbrs:
                    dSx.catalog.append(sx)


        # recalculate FWHM, Flux etc.
        i_fwhm= self.dataSxtrs[0].fields.index('FWHM_IMAGE') 

        i_flux=None
        if self.rt.cfg['ANALYZE_FLUX']:

            i_flux= self.dataSxtrs[0].fields.index('FLUX_MAX') 
        if len(self.dataSxtrs):
            for dSx in sorted(self.dataSxtrs, key=lambda x: int(x.focPos), reverse=False):
                dSx.fillData(i_fwhm=i_fwhm, i_flux=i_flux)

                if self.rt.cfg['ANALYZE_FLUX']:
                    if self.debug: self.logger.debug('onAllImages: {0:5d} {1:8.3f}/{2:5.3f} {3:8.3f}/{4:5.3f} {5:5d}'.format(int(dSx.focPos), dSx.fwhm, dSx.stdFwhm, dSx.flux, dSx.stdFlux, len(dSx.catalog)))
                else:
                    if self.debug: self.logger.debug('onAllImages: {0:5d} {1:8.3f}/{2:5.3f}  {3:5d}'.format(int(dSx.focPos), dSx.fwhm, dSx.stdFwhm, len(dSx.catalog)))
        
        self.logger.info('onAlmostImages: objects: {0}'.format(len(self.assocObjNmbrs)))

    def onAlmostImages(self):
        focPosS=collections.defaultdict(int)
        for dSx in self.dataSxtrs:
            focPosS[dSx.focPos]=dSx.nObjs

        self.dropDSx(focPosS=focPosS)

    def numberOfFocPos(self):
        pos=collections.defaultdict(int)
        for dSx in self.dataSxtrs:
            pos[dSx.focPos] += 1 

        ok=True
        if len(pos) <= self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']:
            self.logger.warn('numberOfFocPos: to few DIFFERENT focuser positions: {0}<={1} (see MINIMUM_FOCUSER_POSITIONS), continuing'.format(len(pos), self.rt.cfg['MINIMUM_FOCUSER_POSITIONS']))
            ok=False
        return ok

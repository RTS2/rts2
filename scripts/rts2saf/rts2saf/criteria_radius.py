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

import math


class Criteria(object):
    def __init__(self, dataSxtr=None, rt=None):
        self.dataSxtr=dataSxtr
        self.rt=rt

        self.i_x = self.dataSxtr[0].fields.index('X_IMAGE')
        self.i_y = self.dataSxtr[0].fields.index('Y_IMAGE')
        self.center=[ self.dataSxtr[0].naxis1/2.,self.dataSxtr[0].naxis2/2. ] 
        rds= self.rt.cfg['RADIUS'] 
        if self.dataSxtr[0].binning:
            self.radius= rds/self.dataSxtr[0].binning 
        elif self.dataSxtr[0].binningXY:
            # ToDo (bigger): only x value is used
            self.radius= rds/self.dataSxtr[0].binningXY[0] 
        else:
            # everything should come
            self.radius=pow(self.dataSxtr[0].naxis1, 2) + pow(self.dataSxtr[0].naxis2, 2)
        

    def decide(self, catalogEntry=None):
        rd= math.sqrt(pow(catalogEntry[self.i_x]-self.center[0],2)+ pow(catalogEntry[self.i_y]-self.center[1],2))
        if rd < self.radius:
            return True
        else:
            return False

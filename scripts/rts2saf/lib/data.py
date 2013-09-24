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

class DataFwhm(object):
    def __init__(self, pos=list(), fwhm=list(),errx=list(), stdFwhm=list()):
        self.pos=pos
        self.fwhm=fwhm
        self.errx=errx
        self.stdFwhm=stdFwhm

class DataSex(object):
    def __init__(self, fitsFn=None, focPos=None, fwhm=None, stdFwhm=None, nstars=None, ambientTemp=None, catalogue=None, fields=None):
        self.fitsFn=fitsFn
        self.focPos=focPos
        self.fwhm=fwhm
        self.stdFwhm=stdFwhm
        self.nstars=nstars
        self.ambientTemp=ambientTemp
        self.catalogue=catalogue
        self.fields=fields

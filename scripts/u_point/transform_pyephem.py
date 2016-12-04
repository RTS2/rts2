# (C) 2016, Markus Wildi, wildi.markus@bluewin.ch
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
#
__author__ = 'wildi.markus@bluewin.ch'

# Transform with pyephem

import ephem
import numpy as np

from datetime import datetime
from astropy import units as u
from astropy.coordinates import SkyCoord

class Transformation(object):
  def __init__(self, lg=None,obs=None):
    #
    self.lg=lg
    self.name='PE PyEphem'
    self.obs_astropy=obs
    self.obs=ephem.Observer()
    # positive values are used for eastern longitudes
    longitude,latitude,height=self.obs_astropy.to_geodetic()
    self.obs.lon=longitude.radian
    self.obs.lat=latitude.radian
    # ToDo
    # print(type(height))
    # print(str(height))
    # 3236.9999999999477 m
    # print(height.meter)
    # AttributeError: 'Quantity' object has no 'meter' member
    self.obs.elevation=float(str(height).replace('m','').replace('meter',''))
    # do that later
    #self.obs.date

  def transform_to_hadec(self,ic=None,tem=0.,pre=0.,hum=0.,apparent=None):
    star=self.create_star(ic=ic,tem=tem,pre=pre)
    HA= self.obs.sidereal_time() - star.ra 
    ha=SkyCoord(ra=HA,dec=star.dec, unit=(u.radian,u.radian), frame='cirs',location=ic.location,obstime=ic.obstime,pressure=pre,temperature=tem,relative_humidity=hum)

    return ha

  def transform_to_altaz(self,ic=None,tem=0.,pre=0.,hum=0.,apparent=None):
    star=self.create_star(ic=ic,tem=tem,pre=pre)
    aa=SkyCoord(az=star.az,alt=star.alt, unit=(u.radian,u.radian), frame='altaz',location=ic.location,obstime=ic.obstime,pressure=pre,temperature=tem,relative_humidity=hum)    
    return aa

  def create_star(self,ic=None,tem=0.,pre=0.):
    dt=str(ic.obstime).replace('-','/')
    self.obs.date=ephem.Date(dt)
    self.obs.pressure=pre
    self.obs.temp=tem
    star = ephem.FixedBody()
    star._ra = ic.ra.radian
    star._dec = ic.dec.radian
    star.compute(self.obs)
    return star

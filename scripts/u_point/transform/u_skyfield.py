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

# Transform with skyfield

from skyfield.api import Star,load,Angle,utc
planets = load('de421.bsp')
from skyfield.api import Topos


import numpy as np

from datetime import datetime
from astropy import units as u
from astropy.coordinates import SkyCoord

class Transformation(object):
  def __init__(self, lg=None,obs=None,refraction_method=None):
    #
    self.lg=lg
    self.name='SF Skyfield'
    self.obs_astropy=obs
    self.refraction_method=refraction_method
    longitude,latitude,height=self.obs_astropy.to_geodetic()
    # positive values are used for eastern longitudes
    # ToDo
    # print(type(height))
    # print(str(height))
    # 3236.9999999999477 m
    # print(height.meter)
    # AttributeError: 'Quantity' object has no 'meter' member
    elevation=float(str(height).replace('m','').replace('meter',''))
    earth = planets['earth']
    # N +, E + hurray
    self.obs=earth + Topos(
          latitude_degrees=latitude.degree,
          longitude_degrees=longitude.degree,
          elevation_m=elevation,
      )
    self.ts=load.timescale()

  def transform_to_hadec(self,tf=None,sky=None,mount_set_icrs=None):
    tem=sky.temperature
    pre=sky.pressure
    hum=sky.humidity
    
    ra=Angle(radians=tf.ra.radian*u.radian)
    dec=Angle(radians=tf.dec.radian*u.radian)
    star=Star(ra=ra,dec=dec)
    #HA= self.obs.sidereal_time() - star.ra 
    #ha=SkyCoord(ra=HA,dec=star.dec, unit=(u.radian,u.radian), frame='cirs',location=tf.location,obstime=tf.obstime,pressure=pre,temperature=tem,relative_humidity=hum)
    self.lg.error('not  yet implemented')
    sys.exit(1)

    #return

  def transform_to_altaz(self,tf=None,sky=None,mount_set_icrs=None):
    # use ev. other refraction methods
    if sky is None:
      tem=pre=hum=0.
    else:
      tem=sky.temperature
      pre=sky.pressure
      hum=sky.humidity
      
    ra=Angle(radians=tf.ra.radian*u.radian)
    dec=Angle(radians=tf.dec.radian*u.radian)
    star=Star(ra=ra,dec=dec)
    t=self.ts.utc(tf.obstime.datetime.replace(tzinfo=utc))

    if mount_set_icrs:
      # Apparent GCRS ("J2000.0") coordinates
      alt, az, distance=self.obs.at(t).observe(star).apparent().altaz()
    else:
      # is already apparent
      #
      alt, az, distance=self.obs.at(t).observe(star).apparent().altaz()
      
    aa=SkyCoord(az=az.to(u.radian),alt=alt.to(u.radian), unit=(u.radian,u.radian), frame='altaz',location=tf.location,obstime=tf.obstime,pressure=pre,temperature=tem,relative_humidity=hum)    
    return aa


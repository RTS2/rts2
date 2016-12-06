#!/usr/bin/env python3
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

# Transform with Toshimi Taki
# geocentric, puerly geometrical plus refraction
#

import numpy as np

from datetime import datetime
from astropy import units as u
from astropy.coordinates import SkyCoord

def x_rot(ang):
  s=np.sin(ang)
  c=np.cos(ang)
  return np.matrix([
  [1.,0., 0.], 
  [0.,c, -s],
  [0.,s,  c]])

def y_rot(ang):
  s=np.sin(ang)
  c=np.cos(ang)
  return np.matrix([
  [ c, 0.,s],
  [ 0.,1.,0.], 
  [-s, 0.,c]])

def z_rot(ang):
  s=np.sin(ang)
  c=np.cos(ang)
  return np.matrix([
  [c, -s, 0.],
  [s,  c, 0.],
  [0., 0.,1.]]) 

def cosine(lat=None,lon=None):
  s=np.array([np.cos(lat)*np.cos(lon),np.cos(lat)*np.sin(lon), np.sin(lat)]).transpose()
  s.shape=(3,1)
  return s

# ToDo: status not yet prime time!!!!!!!!!!!!!!
class Transformation(object):
  def __init__(self, lg=None,obs=None,refraction_method=None):
    #
    self.lg=lg
    self.name='TS Toshimi Taki'
    self.obs_astropy=obs
    self.refraction_method=refraction_method
    longitude,latitude,height=self.obs_astropy.to_geodetic()
    self.latitude=latitude.radian
    self.MAA=y_rot(self.latitude-np.pi/2.)
    self.MAA_I=np.linalg.inv(self.MAA)
    self.lg.warn('status not yet prime time!!!!!!!!!!!!!!')
    
  def transform_to_hadec(self,tf=None,sky=None,apparent=None):
    tem=sky.temperature
    pre=sky.pressure
    hum=sky.humidity
    aa=self.transform_to_altaz(tf=tf,sky=sky,apparent=apparent)
    #
    # ToDO chack right handed                  
    cs=cosine(lat=aa.alt.radian,lon=aa.az.radian)

    t = self.MAA_I * cs
    dec=np.arcsin(float(t[2]))
    # ToDo left right handed
    HA= np.arctan2(float(t[1]),float(t[0]))

    ha=SkyCoord(ra=HA,dec=dec, unit=(u.radian,u.radian), frame='cirs',location=tf.location,obstime=tf.obstime,pressure=pre,temperature=tem,relative_humidity=hum)    

    return ha

  def transform_to_altaz(self,tf=None,sky=None,apparent=None):
    tem=sky.temperature
    pre=sky.pressure
    hum=sky.humidity
    dt = tf.obstime
    sid=dt.sidereal_time(kind='apparent',longitude=self.obs_astropy.longitude)
    #
    ha = sid.radian - tf.ra.radian
    # left handed                   |||
    cs=cosine(lat=tf.dec.radian,lon=-ha)

    t = self.MAA * cs
    alt=np.arcsin(float(t[2]))
    az= -np.arctan2(float(t[1]),float(t[0]))
    d_alt=0.
    if apparent:
      #d_alt=refraction_bennett(alt=alt,tem=tem,pre=pre,hum=hum)
      #self.lg.debug('B: d_alt: {} arcmin, alt: {}, deg'.format(d_alt*60.*180./np.pi,alt * 180./np.pi))
      d_alt=self.refraction_method(alt=alt,tem=tem,pre=pre,hum=hum)
      self.lg.debug('O:d_alt: {} arcmin, alt: {}, deg'.format(d_alt*60.*180./np.pi,alt * 180./np.pi))

    aa=SkyCoord(az=az,alt=alt+d_alt, unit=(u.radian,u.radian), frame='altaz',location=tf.location,obstime=tf.obstime,pressure=pre*u.hPa,temperature=tem*u.deg_C,relative_humidity=hum)    

    return aa

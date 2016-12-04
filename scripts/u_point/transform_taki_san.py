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
            
# refraction: J.C. Owens, 1967, Optical Refractive Index of Air: Dependence on Pressure, Temperature and Composition  
def refractive_index(pressure_qfe=None,temperature=None,humidity=None,obswl=0.5):
  #extern double tdk, hm, rh, pmb ;
  # pressure_t = pressure_o+1013.25*pow((1.-(0.0065* hm/288.)), 5.255)  
  # Calculate the partial pressures for H2O ONLY
  # may be soon we have to inclide the carbon
  #
  # temperature: degC
  if pressure_qfe==0.:
    return np.nan
  
  tdk=temperature + 273.15
  rh=humidity
  obswl *= 1000. # Angstroem
  a=   7.6 ;
  b= 240.7 ;
  if tdk > 273.15:
    a=   7.5
    b= 235.0
    
  p_w = rh*(6.107*pow(10,(a*(tdk-273.15)/(b+(tdk-273.15)))))
  p_s= pressure_qfe-p_w ;

  # Calculate the factors D_s and D_w 
  d_s= p_s/tdk*(1.+p_s*(57.9e-8-9.3250e-4/tdk+0.25844/pow(tdk, 2.)))
  d_w= p_w/tdk*(1.+p_w*(1.+3.7e-4*p_w)*(-2.37321e-3+2.23366/tdk-710.792/pow(tdk,2.) 
                                 +7.75141e4/pow(tdk,3.)))
  
  u1=d_s*(2371.34+683939.7/(130.-pow(obswl,-2.))+4547.3/(38.99-pow(obswl,-2.)))
  u2=d_w*(6487.31+58.058 * pow( obswl, -2.)-0.7115*pow(obswl,-4.)+0.08851*pow(obswl,-6.))

  n_minus_1=u1+u2
  # Attention NO +1
  return n_minus_1 * 1.e-8  ;

# ToDo: status not yet prime time!!!!!!!!!!!!!!
class Transformation(object):
  def __init__(self, lg=None,obs=None):
    #
    self.lg=lg
    self.name='TS Toshimi Taki'
    self.obs_astropy=obs
    # positive values are used for eastern longitudes
    longitude,latitude,height=self.obs_astropy.to_geodetic()
    # negative to the east: see p.15 Uccle Observatory
    self.lon=-longitude.radian
    self.lat=latitude.radian
    # no height
    self.MAA=y_rot(self.lat-np.pi/2.)
    self.MAA_I=np.linalg.inv(self.MAA)
    self.lg.warn('status not yet prime time!!!!!!!!!!!!!!')
    
  def transform_to_hadec(self,ic=None,tem=0.,pre=0.,hum=0.,apparent=None):
    aa=self.transform_to_altaz(ic=ic,tem=tem,pre=pre,hum=0.,apparent=apparent)
    #
    # ToDO chack right handed                  
    cs=cosine(lat=aa.alt.radian,lon=aa.az.radian)

    t = self.MAA_I * cs
    dec=np.arcsin(float(t[2]))
    # ToDo left right handed
    HA= np.arctan2(float(t[1]),float(t[0]))

    ha=SkyCoord(ra=HA,dec=dec, unit=(u.radian,u.radian), frame='cirs',location=ic.location,obstime=ic.obstime,pressure=pre,temperature=tem,relative_humidity=hum)    

    return ha

  def transform_to_altaz(self,ic=None,tem=0.,pre=0.,hum=0.,apparent=None):
    dt = ic.obstime
    sid=dt.sidereal_time(kind='apparent',longitude=self.obs_astropy.longitude)
    #
    ha = sid.radian - ic.ra.radian
    # left handed                   |||
    cs=cosine(lat=ic.dec.radian,lon=-ha)

    t = self.MAA * cs
    alt=np.arcsin(float(t[2]))
    az= -np.arctan2(float(t[1]),float(t[0]))
    d_alt=0.
    if apparent:
      n_minus_1=refractive_index(pressure_qfe=pre,temperature=tem,humidity=hum,obswl=0.5)
      if not np.isnan(n_minus_1) :
        # ToDo something elese?
        # To np.ana may be a glitch
        d_alt= np.pi/2.-np.arcsin(np.cos(alt)/(n_minus_1+1.))-alt
        self.lg.debug('d_alt: {} arcmin, alt: {}, deg'.format(d_alt*180.*60./np.pi,alt * 180./np.pi))

    aa=SkyCoord(az=az,alt=alt+d_alt, unit=(u.radian,u.radian), frame='altaz',location=ic.location,obstime=ic.obstime,pressure=pre*u.hPa,temperature=tem*u.deg_C,relative_humidity=hum)    

    return aa


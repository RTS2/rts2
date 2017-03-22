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
__author__ = 'wildi.markus@bluewin.ch'

# Transform with libnova
# Python bindings for libnova
#
import numpy as np
from astropy.coordinates import Longitude,Latitude,Angle
from astropy import units as u
from astropy.coordinates import SkyCoord

from ctypes import *
class LN_equ_posn(Structure):
  _fields_ = [("ra", c_double),("dec", c_double)]

class LN_hrz_posn(Structure):
  _fields_ = [("az", c_double),("alt", c_double)]

class LN_lnlat_posn(Structure):
  _fields_ = [("lng", c_double),("lat", c_double)]

class LN_nut(Structure):
  _fields_ = [("longitude", c_double),("obliquity", c_double),("ecliptic", c_double)]

# add full path if it is not on LD_PATH
ln=cdll.LoadLibrary("libnova.so")
ln.ln_get_equ_aber.restype = None
ln.ln_get_equ_prec.restype = None
ln.ln_get_equ_from_ecl.restype = None
ln.ln_get_nutation.restype = None
 
ln.ln_get_refraction_adj.restype = c_double
ln.ln_get_angular_separation.restype = c_double

ln_pos_eq=LN_equ_posn()
ln_pos_eq_ab=LN_equ_posn()
ln_pos_eq_pm=LN_equ_posn()
ln_pos_eq_app=LN_equ_posn()
ln_pos_eq_pr=LN_equ_posn()
ln_pos_aa_ab=LN_hrz_posn()
ln_hrz_posn=LN_hrz_posn()
ln_lnlat_posn=LN_lnlat_posn()
ln_nut= LN_nut()
ln_pos_eq_nut=LN_equ_posn()

class Transformation(object):
  def __init__(self, lg=None,obs=None,refraction_method=None):
    #
    self.lg=lg
    self.name='LN Libnova'
    self.refraction_method=refraction_method
    self.obs=obs
    self.ln_obs=LN_lnlat_posn()    
    self.ln_obs.lng=obs.longitude.degree # deg
    self.ln_obs.lat=obs.latitude.degree  # deg
    self.ln_hght=obs.height  # hm, no .meter?? m, not a libnova quantity

  def transform_to_hadec(self,tf=None,sky=None,mount_set_icrs=None):
    tem=sky.temperature
    pre=sky.pressure
    hum=sky.humidity
    pre_qfe=pre # to make it clear what is used
    aa=self.LN_EQ_to_AltAz(ra=Longitude(tf.ra.radian,u.radian).degree,dec=Latitude(tf.dec.radian,u.radian).degree,ln_pressure_qfe=pre_qfe,ln_temperature=tem,ln_humidity=hum,obstime=tf.obstime)
    ha=self.LN_AltAz_to_HA(az=aa.az.degree,alt=aa.alt.degree,obstime=tf.obstime)  
    return ha

  def transform_to_altaz(self,tf=None,sky=None,mount_set_icrs=None):
    tem=sky.temperature
    pre=sky.pressure
    hum=sky.humidity
    aa=self.LN_EQ_to_AltAz(ra=Longitude(tf.ra.radian,u.radian).degree,dec=Latitude(tf.dec.radian,u.radian).degree,ln_pressure_qfe=pre_qfe,ln_temperature=tem,ln_humidity=hum,obstime=tf.obstime,mount_set_icrs=mount_set_icrs)
    return aa

  def LN_nutation_meeus(self,eq_pr,JD=None):
    # first order correction
    ln.ln_get_nutation(c_double(JD),byref(ln_nut))
    d_psi=ln_nut.longitude /180.*np.pi
    epsilon_0=(ln_nut.ecliptic+ln_nut.obliquity)/180.*np.pi # true obliquity
    d_epsilon=ln_nut.obliquity/180.*np.pi

    ra=Longitude(eq_pr.ra,u.degree).radian
    dec=Latitude(eq_pr.dec,u.degree).radian
    d_ra=(np.cos(epsilon_0)+np.sin(epsilon_0)*np.sin(ra)*np.tan(dec))*d_psi-np.cos(ra)*np.tan(dec)*d_epsilon
    d_dec=(np.sin(epsilon_0)*np.cos(ra))*d_psi+np.sin(ra)*d_epsilon
    ln_pos_eq_nut.ra=eq_pr.ra + d_ra * 180./np.pi
    ln_pos_eq_nut.dec=eq_pr.dec + d_dec* 180./np.pi

    return ln_pos_eq_nut
  
  def LN_EQ_to_AltAz(self,ra=None,dec=None,ln_pressure_qfe=None,ln_temperature=None,ln_humidity=None,obstime=None,mount_set_icrs=False):
    ln_pos_eq.ra=ra
    ln_pos_eq.dec=dec
    if mount_set_icrs:
      # ToDo missing see Jean Meeus, Astronomical Algorithms, chapter 23
      # proper motion
      # annual paralax (0".8)
      # gravitational deflection of light (0".003)
      ln.ln_get_equ_prec(byref(ln_pos_eq), c_double(obstime.jd), byref(ln_pos_eq_pr))
      ln_pos_eq_nut=self.LN_nutation_meeus(eq_pr=ln_pos_eq_pr,JD=obstime.jd)
      ln.ln_get_equ_aber(byref(ln_pos_eq_nut), c_double(obstime.jd), byref(ln_pos_eq_ab))
      ln.ln_get_hrz_from_equ(byref(ln_pos_eq_ab), byref(self.ln_obs), c_double(obstime.jd), byref(ln_pos_aa_ab))
      # here we use QFE not pressure at sea level!
      # E.g. at Dome-C this formula:
      # ln_pressure=ln_see_pres * pow(1. - (0.0065 * ln_alt) / 288.15, (9.80665 * 0.0289644) / (8.31447 * 0.0065));
      # is not precise.
      if self.refraction_method is None:
        d_alt_deg=ln.ln_get_refraction_adj(c_double(ln_pos_aa_ab.alt),c_double(ln_pressure_qfe),c_double(ln_temperature))
      else:
        d_alt_deg=180./np.pi* self.refraction_method(alt=ln_pos_aa_ab.alt,tem=ln_temperature,pre=ln_pressure_qfe,hum=ln_humidity)
    else:
      # ... but not for the star position as measured in mount frame
      ln.ln_get_hrz_from_equ(byref(ln_pos_eq), byref(self.ln_obs), c_double(obstime.jd), byref(ln_pos_aa_ab));
      d_alt_deg=0.
  
    a_az=Longitude(ln_pos_aa_ab.az,u.deg)
    a_alt=Latitude(ln_pos_aa_ab.alt + d_alt_deg,u.deg)

    pos_aa=SkyCoord(az=a_az.radian,alt=a_alt.radian,unit=(u.radian,u.radian),frame='altaz',location=self.obs,obstime=obstime,obswl=0.5*u.micron, pressure=ln_pressure_qfe*u.hPa,temperature=ln_temperature*u.deg_C,relative_humidity=ln_humidity)
    return pos_aa

  def LN_AltAz_to_HA(self,az=None,alt=None,obstime=None):
    ln_hrz_posn.alt=alt
    ln_hrz_posn.az=az
    ln.ln_get_equ_from_hrz(byref(ln_hrz_posn),byref(self.ln_obs), c_double(obstime.jd),byref(ln_pos_eq))
    # calculate HA
    ra=Longitude(ln_pos_eq.ra,u.deg)
    HA= obstime.sidereal_time('apparent') - ra
    # hm, ra=ha a bit ugly
    ha=SkyCoord(ra=HA, dec=Latitude(ln_pos_eq.dec,u.deg).radian,unit=(u.radian,u.radian),frame='cirs')
    return ha

  def LN_ICRS_to_GCRS(self,ra=None,dec=None,ln_pressure_qfe=None,ln_temperature=None,ln_humidity=None,obstime=None):
    ln_pos_eq.ra=ra
    ln_pos_eq.dec=dec
    ln.ln_get_equ_prec(byref(ln_pos_eq), c_double(obstime.jd), byref(ln_pos_eq_pr))
    ln_pos_eq_nut=self.LN_nutation_meeus(eq_pr=ln_pos_eq_pr,JD=obstime.jd)
    ln.ln_get_equ_aber(byref(ln_pos_eq_nut), c_double(obstime.jd), byref(ln_pos_eq_ab))
    ra=Longitude(ln_pos_eq_ab.ra,u.deg)
    dec=Latitude(ln_pos_eq_ab.dec,u.deg)
    gcrs=SkyCoord(ra=ra.radian,dec=dec.radian,unit=(u.radian,u.radian),frame='gcrs',location=self.obs,obstime=obstime,obswl=0.5*u.micron, pressure=ln_pressure_qfe*u.hPa,temperature=ln_temperature*u.deg_C,relative_humidity=ln_humidity)

    return gcrs

  def LN_ICRS_to_AltAz(self,ra=None,dec=None,ln_pressure_qfe=None,ln_temperature=None,ln_humidity=None,obstime=None,mount_set_icrs=True):
    ln_pos_eq.ra=ra
    ln_pos_eq.dec=dec
    if mount_set_icrs:
      # libnova corrections for catalog data ...
      # ToDo missing see Jean Meeus, Astronomical Algorithms, chapter 23
      # proper motion
      # annual paralax (0".8)
      # gravitational deflection of light (0".003)
      ln.ln_get_equ_prec(byref(ln_pos_eq), c_double(obstime.jd), byref(ln_pos_eq_pr))
      ln_pos_eq_nut=self.LN_nutation_meeus(eq_pr=ln_pos_eq_pr,JD=obstime.jd)
      ln.ln_get_equ_aber(byref(ln_pos_eq_nut), c_double(obstime.jd), byref(ln_pos_eq_ab))
      ln.ln_get_hrz_from_equ(byref(ln_pos_eq_ab), byref(self.ln_obs), c_double(obstime.jd), byref(ln_pos_aa_ab))
      # here we use QFE not pressure at sea level!
      # E.g. at Dome-C this formula:
      # ln_pressure=ln_see_pres * pow(1. - (0.0065 * ln_alt) / 288.15, (9.80665 * 0.0289644) / (8.31447 * 0.0065));
      # is not precise.
      if self.refraction_method is None:
        d_alt_deg=ln.ln_get_refraction_adj(c_double(ln_pos_aa_ab.alt),c_double(ln_pressure_qfe),c_double(ln_temperature))
      else:
        d_alt_deg=180./np.pi* self.refraction_method(alt=ln_pos_aa_ab.alt,tem=ln_temperature,pre=ln_pressure_qfe,hum=ln_humidity)
    else:
      # ... but not for the star position as measured in mount frame
      ln.ln_get_hrz_from_equ(byref(ln_pos_eq), byref(self.ln_obs), c_double(obstime.jd), byref(ln_pos_aa_ab));
      d_alt_deg=0.
  
    a_az=Longitude(ln_pos_aa_ab.az,u.deg)
    a_az.wrap_at(0.*u.degree)
    a_alt=Latitude(ln_pos_aa_ab.alt + d_alt_deg,u.deg)

    pos_aa=SkyCoord(az=a_az.radian,alt=a_alt.radian,unit=(u.radian,u.radian),frame='altaz',location=self.obs,obstime=obstime,obswl=0.5*u.micron, pressure=ln_pressure_qfe*u.hPa,temperature=ln_temperature*u.deg_C,relative_humidity=ln_humidity)
    return pos_aa

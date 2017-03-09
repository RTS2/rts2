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

# Transform with astropy

from astropy import units as u
from astropy.coordinates import AltAz
from astropy.coordinates import SkyCoord

class Transformation(object):
  def __init__(self, lg=None,obs=None,refraction_method=None):
    #
    self.lg=lg
    self.name='AP astropy'
    self.obs=obs
    self.refraction_method=refraction_method
    
  def transform_to_hadec(self,tf=None,sky=None,mount_set_icrs=None):
    tem=sky.temperature
    pre_qfe=pre=sky.pressure
    hum=sky.humidity

    # 2017-03-15: deprecated:
    #if mount_set_icrs:
    #  pre_qfe=pre # to make it clear what is used
    #  aa=tf.transform_to(AltAz(obswl=0.5*u.micron, pressure=pre_qfe*u.hPa,temperature=tem*u.deg_C,relative_humidity=hum))
    #else:
    #  pre_qfe=0.
    #  # ToDo: still hoping that no precession etc. is applied
    #  ra=tf.ra.radian
    #  dec=tf.dec.radian
    #  # ToDo: clarify icrs vs. gcrs
    #  ti=SkyCoord(ra=ra,dec=dec,unit=(u.rad,u.rad), frame='cirs',obstime=tf.obstime,location=tf.location,pressure=0.*u.hPa,temperature=0.*u.deg_C,relative_humidity=0.)
    #  aa=ti.transform_to(AltAz(obswl=0.5*u.micron,pressure=0.*u.hPa,temperature=0.*u.deg_C,relative_humidity=0.))
      
    # ToDo: check that if correct!
    #       if pressure is non zero refraction is subtracted
    # wrong:
    #ci=aa.cirs
    # from apparent AltAz to apparent 'CIRS': do not correct subtract refraction
    # pressure=temperature=humidiy=0.
    #
    
    aa=tf.transform_to(AltAz(obswl=0.5*u.micron, pressure=pre_qfe*u.hPa,temperature=tem*u.deg_C,relative_humidity=hum))
    aa_no_pressure=SkyCoord(az=aa.az,alt=aa.alt, unit=(u.radian,u.radian), frame='altaz',obstime=aa.obstime,location=aa.location,pressure=0.*u.hPa,temperature=0.*u.deg_C,relative_humidity=0.)
    gc=aa_no_pressure.gcrs
    HA= gc.obstime.sidereal_time('apparent') - gc.ra
    ha=SkyCoord(ra=HA,dec=gc.dec, unit=(u.rad,u.rad), frame='gcrs',obstime=gc.obstime,location=aa.location,pressure=0.*u.hPa,temperature=0.*u.deg_C,relative_humidity=0.)
      
    return ha

  def transform_to_altaz(self,tf=None,sky=None,mount_set_icrs=None):
    # use ev. other refraction methods
    if sky is None:
      tem=pre=hum=0.
    else:
      tem=sky.temperature
      pre=sky.pressure
      hum=sky.humidity
    '''
    There are substancial differences between astropy and libnova apparent coordinates. 
    Choose option --astropy for Astropy, default is Libnova
    '''
    pre_qfe=pre # to make it clear what is used
      
    # from https://github.com/liberfa/erfa/blob/master/src/refco.c
    # phpa   double    pressure at the observer (hPa = millibar)
    # this is QFE

    # ToDo check that
    aa=tf.transform_to(AltAz(location=self.obs,obswl=0.5*u.micron,pressure=pre_qfe*u.hPa,temperature=tem*u.deg_C,relative_humidity=hum))
    return aa
    # 2017-03-15: deprecated:
    # reminder astropy transforms from any ICRS,GCRS to Altaz
    # set tf to the correct coordinate system
    
    #if mount_set_icrs:
    #  # tf is already GCRS (apparent)
    #  # ToDo: still hoping that no precession etc. is applied: see test_coordinates_altaz.p
    #  # astropy gcrs:
    #  # A coordinate or frame in the Geocentric Celestial Reference System (GCRS).
    #  # GCRS is distinct form ICRS mainly in that it is relative to the Earth’s center-of-mass rather than the solar system Barycenter.
    #  # That means this frame includes the effects of aberration (unlike ICRS).
    #  # here I want simply RA,Dec to AltAz (geometrical, plus refraction)
    #  ti=SkyCoord(ra=tf.ra.radian,dec=tf.dec.radian, unit=(u.rad,u.rad), frame='gcrs',obstime=tf.obstime,location=tf.location,pressure=0.*u.hPa,temperature=0.*u.deg_C,relative_humidity=0.)
    #else:
    #  # tf,ic are ICRS, including meteo data
    #  ti=tf 

    #aa=ti.transform_to(AltAz(location=self.obs,obswl=0.5*u.micron,pressure=pre_qfe*u.hPa,temperature=tem*u.deg_C,relative_humidity=hum))
    #return aa

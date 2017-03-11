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

    
    aa=tf.transform_to(AltAz(obswl=0.5*u.micron, pressure=pre_qfe*u.hPa,temperature=tem*u.deg_C,relative_humidity=hum))
    aa_no_pressure=SkyCoord(az=aa.az,alt=aa.alt, unit=(u.radian,u.radian), frame='altaz',obstime=aa.obstime,location=aa.location,pressure=0.*u.hPa,temperature=0.*u.deg_C,relative_humidity=0.)
    gc=aa_no_pressure.gcrs
    HA= gc.obstime.sidereal_time('apparent') - gc.ra
    ha=SkyCoord(ra=HA,dec=gc.dec, unit=(u.rad,u.rad), frame='gcrs',obstime=gc.obstime,location=aa.location,pressure=0.*u.hPa,temperature=0.*u.deg_C,relative_humidity=0.)
      
    return ha
  # ToD mount_set_icrs will disappear
  def transform_to_altaz(self,tf=None,sky=None,mount_set_icrs=None):
    # from https://github.com/liberfa/erfa/blob/master/src/refco.c
    # phpa   double    pressure at the observer (hPa = millibar)
    # this is QFE
    if sky is None:
      tem=pre_qfe=hum=0.
    else:
      tem=sky.temperature
      pre_qfe=sky.pressure # to make it clear what is used
      hum=sky.humidity
      
    if self.refraction_method is None:
      # ToDo check that
      aa=tf.transform_to(AltAz(location=self.obs,obswl=0.5*u.micron,pressure=pre_qfe*u.hPa,temperature=tem*u.deg_C,relative_humidity=hum))
    else:
      # transform to altaz and add refraction
      aa=tf.transform_to(AltAz(location=self.obs,obswl=0.5*u.micron,pressure=0.*u.hPa,temperature=0.*u.deg_C,relative_humidity=0.))
      d_alt=self.refraction_method(alt=aa.alt,tem=tem,pre=pre_qfe,hum=hum)
      aa.alt += d_alt
    return aa

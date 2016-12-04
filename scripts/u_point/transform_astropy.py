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
  def __init__(self, lg=None,obs=None):
    #
    self.lg=lg
    self.name='AP astropy'
    self.obs=obs

  def transform_to_hadec(self,ic=None,tem=0.,pre=0.,hum=0.,apparent=None):
    pre_qfe=pre # to make it clear what is used

    aa=ic.transform_to(AltAz(obswl=0.5*u.micron, pressure=pre_qfe*u.hPa,temperature=tem*u.deg_C,relative_humidity=hum))
    # ToDo: check that if correct!
    #       if pressure is non zero refraction is subtracted
    # wrong:
    #ci=aa.cirs
    # from apparent AltAz to apparent 'CIRS': do not correct subtract refraction
    # pressure=temperature=humidiy=0.
    aa_no_pressure=SkyCoord(az=aa.az,alt=aa.alt, unit=(u.radian,u.radian), frame='altaz',obstime=aa.obstime,location=aa.location,pressure=0.*u.hPa,temperature=0.*u.deg_C,relative_humidity=0.)
    ci=aa_no_pressure.cirs
    HA= ci.obstime.sidereal_time('apparent') - ci.ra
    ha=SkyCoord(ra=HA,dec=ci.dec, unit=(u.rad,u.rad), frame='cirs',obstime=ci.obstime,location=aa.location,pressure=0.*u.hPa,temperature=0.*u.deg_C,relative_humidity=0.)
    return ha

  def transform_to_altaz(self,ic=None,tem=0.,pre=0.,hum=0.,apparent=None):
    '''
    There are substancial differences between astropy and libnova apparent coordinates. 
    Choose option --astropy for Astropy, default is Libnova
    '''
    pre_qfe=pre # to make it clear what is used
      
    # from https://github.com/liberfa/erfa/blob/master/src/refco.c
    # phpa   double    pressure at the observer (hPa = millibar)
    # this is QFE
    aa=ic.transform_to(AltAz(location=self.obs,obswl=0.5*u.micron,pressure=pre_qfe*u.hPa,temperature=tem*u.deg_C,relative_humidity=hum))
    return aa

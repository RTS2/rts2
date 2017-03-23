#!/usr/bin/env python3.5
#
# (C) 2017, Markus Wildi, wildi.markus@bluewin.ch
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


'''
u_sofa.py's purpose is solely to check the astropy transformation.

'''



# Transform with iauAtco13
# http://www.iausofa.org/2016_0503_C/sofa/sofa_ast_c.pdf
# /* ICRS to observed. */
# if ( iauAtco13 ( rc, dc, pr, pd, px, rv, utc1, utc2, dut1,
#      elong, phi, hm, xp, yp, phpa, tc, rh, wl,
#      &aob, &zob, &hob, &dob, &rob, &eo ) ) return -1;

# Python bindings for libsofa_c
#
import numpy as np
import sys
import logging,argparse
import dateutil.parser
from astropy.coordinates import Longitude,Latitude,Angle
from astropy.coordinates import EarthLocation
from astropy import units as u
from astropy.coordinates import SkyCoord
from astropy.time import Time
from astropy.utils import iers
import warnings
# see np.isnan() below
warnings.filterwarnings('ignore', category=UserWarning, append=True)

from astroquery.simbad import Simbad
from ctypes import *
# add full path if it is not on LD_PATH
sf=cdll.LoadLibrary('/home/wildi/sw/sofa/sofa/20160503_c/c/src/libsofa_c.so')
sf.iauDtf2d.argtypes=[POINTER(c_char), c_int, c_int, c_int, c_int, c_int, c_double, POINTER(c_double),POINTER(c_double)]
sf.iauDtf2d.restype=c_int
sf.iauUtctai.restype=c_int
sf.iauTaitt.restype=c_int
sf.iauAf2a.argtypes=[c_char,c_int,c_int,c_double,POINTER(c_double)]
sf.iauAf2a.restype=c_int
sf.iauAtco13.argtypes=[
  c_double,c_double,
  c_double,c_double,
  c_double,c_double,
  c_double,c_double,
  c_double,
  c_double,c_double,
  c_double,c_double,
  c_double,c_double,
  c_double,c_double,
  c_double,
  POINTER(c_double),
  POINTER(c_double),
  POINTER(c_double),
  POINTER(c_double),
  POINTER(c_double),
  POINTER(c_double)]
sf.iauAtco13.restype=c_int

#1./3600. * np.pi /180. = 4.84813681109536e-06
DAS2R=4.848136811095359935899141e-6

class Transformation(object):
  def __init__(self, lg=None,obs=None,refraction_method=None):
    #
    self.lg=lg
    self.name='SF SOFA'
    self.refraction_method=refraction_method
    self.obs_astropy=obs
    longitude,latitude,height=self.obs_astropy.to_geodetic()
    self.elong=longitude
    self.phi=latitude
    self.hm=float(str(height).replace('m','').replace('meter',''))
    self.smbd=Simbad()
    self.smbd.add_votable_fields('propermotions','rv_value','parallax','flux(V)')
    # flux UBVR....
    #print(smbd.get_votable_fields())


  def transform_to_radec(self,tf=None,sky=None,simbad=False):
    aa,hd,rd=self.sofa(tf=tf,sky=sky,simbad=simbad)
    return rd
  
  def transform_to_hadec(self,tf=None,sky=None,simbad=False):
    aa,hd,rd=self.sofa(tf=tf,sky=sky,simbad=simbad)
    return hd

  def transform_to_altaz(self,tf=None,sky=None,simbad=False):
    aa,hd,rd=self.sofa(tf=tf,sky=sky,simbad=simbad)
    return aa
    
  def sofa(self,tf=None,sky=None,simbad=False):
    tc=phpa=rh=0.
    if sky is not None:
      tc=sky.temperature
      phpa=sky.pressure
      rh=sky.humidity
      
    wl=0.55
    utc1=c_double()
    utc2=c_double()
    dtt=tf.obstime.datetime.timetuple()
    if sf.iauDtf2d (b'UTC',
                    dtt.tm_year,
                    dtt.tm_mon,
                    dtt.tm_mday,
                    dtt.tm_hour,
                    dtt.tm_min,
                    dtt.tm_sec,
                    byref(utc1),
                    byref(utc2)):
      self.lg.error('sofa: iauDtf2d failed')
      return None,None,None

    tai1=c_double()
    tai2=c_double()
    if sf.iauUtctai(utc1,utc2,byref(tai1), byref(tai2)):
      self.lg.error('sofa: iauUtctai failed')
      return None,None,None
    
    tt1=c_double()
    tt2=c_double()
    if sf.iauTaitt(tai1,tai2,byref(tt1), byref(tt2)):
      self.lg.error('sofa: iauTaitt failed')
      return None,None,None
    
    #/* EOPs:  polar motion in radians, UT1-UTC in seconds. */
    #xp = 50.995e-3 * DAS2R
    #yp = 376.723e-3 * DAS2R
    #dut1 = 155.0675e-3
    iers_b = iers.IERS_B.open()
    ###dt=dateutil.parser.parse('2013-04-04T23:15:43.55Z')
    dtt=Time(tf.obstime.datetime)
    xp=yp=xp_u=yp_u=0.
    try:
      dut1_u=dtt.delta_ut1_utc = iers_b.ut1_utc(dtt)
      (xp_u,yp_u)=iers_b.pm_xy(dtt)
      dut1=dut1_u.value
      xp=xp_u.value*DAS2R
      yp=yp_u.value*DAS2R
    except iers.IERSRangeError as e:
      self.lg.error('sofa: IERSRangeError setting dut1,xp,yp to zero')
      dut1=0.
      
    #/* Corrections to IAU 2000A CIP (radians). */
    #dx = 0.269e-3 * DAS2R
    #dy = -0.274e-3 * DAS2R
    dx=dy=0.              
    rc=tf.ra.radian
    dc=tf.dec.radian
    pr=pd=px=rv=0.
    # identify star on SIMBAD
    if simbad:
      rslt_tbl = self.smbd.query_region(tf,radius='0d1m0s')
      #print(rslt_tbl.info)
      #MAIN_ID          RA           DEC      RA_PREC DEC_PREC    PMRA     PMDEC   PMRA_PREC PMDEC_PREC RV_VALUE PM_QUAL
      #             "h:m:s"       "d:m:s"                      mas / yr  mas / yr                       km / s
      #------------ ------------- ------------- ------- -------- --------- --------- --------- ---------- -------- -------
      #CPD-80  1068 23 21 32.1685 -79 31 09.138  8        8     3.900    -2.400         2       2        0.341       B                 
      #print(rslt_tbl['MAIN_ID','RA','DEC','RA_PREC','DEC_PREC','PMRA','PMDEC','PMRA_PREC','PMDEC_PREC','RV_VALUE','PM_QUAL'])
      #
      if len(rslt_tbl['RA'])>1:
        rslt_tbl.sort('FLUX_V')
        self.lg.warn('transform_to_altaz: found mor than one entry in SIMBAD table, using brightest main_id:{}'.format(rslt_tbl['MAIN_ID'][0]))
        
        
      print(rslt_tbl)
      (h,m,s)=str(rslt_tbl['RA'][0]).split()      
      rc=Longitude('{0} {1} {2} hours'.format(h,m,s)).radian
      #
      (d,m,s)=str(rslt_tbl['DEC'][0]).split()
      dc=Latitude('{0} {1} {2} degrees'.format(d,m,s)).radian
      #/* Proper motion: RA/Dec derivatives, epoch J2000.0. */
      #/* pr = atan2 ( -354.45e-3 * DAS2R, cos(dc) ); */
      #/* pd = 595.35e-3 * DAS2R; */
      # 
      pmra=float(rslt_tbl['PMRA'][0])
      pmrd=float(rslt_tbl['PMDEC'][0])
      pr=np.arctan2(pmra*DAS2R,np.cos(dc))
      pd=pmrd * DAS2R
      #/* Parallax (arcsec) and recession speed (km/s). */
      #/* px = 164.99e-3; */
      #/* rv = 0.0; */
      # it is a masked table
      px = float(rslt_tbl['PLX_VALUE'][0])
      if np.isnan(px):
        px=0.
      if np.isnan(rv):
        rv = float(rslt_tbl['RV_VALUE'][0])
      
    #/* ICRS to observed. */
    aob=c_double()
    zob=c_double()
    hob=c_double()
    dob=c_double()
    rob=c_double()
    eo= c_double()
    phi=c_double(self.phi.radian)
    elong=c_double(self.elong.radian)
    hm=c_double(self.hm)

    if sf.iauAtco13(rc,dc,pr,pd,px,rv,utc1,utc2,dut1,
                    elong,phi,hm,xp,yp,phpa,tc,rh,wl,
                    byref(aob),
                    byref(zob),
                    byref(hob),
                    byref(dob),
                    byref(rob),
                    byref(eo)):
      self.lg.error('sofa: iauAtco13 failed')
      return None,None,None
    
    aa=SkyCoord(az=aob.value,alt=(np.pi/2.-zob.value),unit=(u.radian,u.radian),frame='altaz',location=self.obs_astropy,obstime=dtt,obswl=wl*u.micron, pressure=phpa*u.hPa,temperature=tc*u.deg_C,relative_humidity=rh)
    # observed RA,Dec rob,dob
    # observed HA,Dec hob,dec
    hd=SkyCoord(ra=hob.value,dec=dob,unit=(u.radian,u.radian),frame='gcrs',location=self.obs_astropy,obstime=dtt,obswl=wl*u.micron, pressure=phpa*u.hPa,temperature=tc*u.deg_C,relative_humidity=rh)
    rd=SkyCoord(ra=rob.value,dec=dob,unit=(u.radian,u.radian),frame='gcrs',location=self.obs_astropy,obstime=dtt,obswl=wl*u.micron, pressure=phpa*u.hPa,temperature=tc*u.deg_C,relative_humidity=rh)
    
    if abs(eo.value)> 1.: 
      self.lg.error('sofa: eo=ERA-GST: {}'.format(eo.value))
      
    return aa,hd,rd

# really ugly!
def arg_floats(value):
  return list(map(float, value.split()))

def arg_float(value):
  if 'm' in value:
    return -float(value[1:])
  else:
    return float(value)

if __name__ == "__main__":
  sys.path.append('../')
  from u_point.structures import SkyPosition
  from transform.u_astropy import Transformation as AstroPyTF


  parser= argparse.ArgumentParser(prog=sys.argv[0], description='Analyze observed positions')
  parser.add_argument('--level', dest='level', default='WARN', help=': %(default)s, debug level')
  parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
  parser.add_argument('--obs-longitude', dest='obs_lng', action='store', default=123.2994166666666,type=arg_float, help=': %(default)s [deg], observatory longitude + to the East [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-latitude', dest='obs_lat', action='store', default=-75.1,type=arg_float, help=': %(default)s [deg], observatory latitude [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-height', dest='obs_height', action='store', default=3237.,type=arg_float, help=': %(default)s [m], observatory height above sea level [m], negative value: m10. equals to -10.')
  parser.add_argument('--base-path', dest='base_path', action='store', default='/tmp/u_point/',type=str, help=': %(default)s , directory where images are stored')
  args=parser.parse_args()
  
  if args.toconsole:
    args.level='DEBUG'

  filename='/tmp/{}.log'.format(sys.argv[0].replace('.py','')) # ToDo datetime, name of the script
  logformat= '%(asctime)s:%(name)s:%(levelname)s:%(message)s'
  logging.basicConfig(filename=filename, level=args.level.upper(), format= logformat)
  logger=logging.getLogger()
    
  if args.toconsole:
    # http://www.mglerner.com/blog/?p=8
    soh=logging.StreamHandler(sys.stdout)
    soh.setLevel(args.level)
    logger.addHandler(soh)

  obs=EarthLocation(lon=float(args.obs_lng)*u.degree, lat=float(args.obs_lat)*u.degree, height=float(args.obs_height)*u.m)
  sofa=Transformation(lg=logger,obs=obs)
  astr=AstroPyTF(lg=logger,obs=obs)

  #star=SkyCoord.from_name('Achernar', frame='icrs')
  dt_now=dateutil.parser.parse('2016-12-31T09:15:57.0Z')
  #CPD-80 1068
  # 23 21 32.1685 -79 31 09.138 [ 32.58 28.34 0 ]
  # propper motion 3.90 -2.40 [2.70 2.20 0] B
  # Radial velocity / Redshift / cz : V(km/s) 0.341 [1.933]
  
  star=SkyCoord('23h21m32.1685s -79d31m09.138s',frame='icrs',obstime=Time(dt_now), location=obs)

  sky=SkyPosition(
    nml_id=-1,
    cat_no=-1,
    nml_aa=None,
    cat_ic=star,
    dt_begin=None,
    dt_end=dt_now,
    dt_end_query=None,
    JD=None,
    mnt_ra_rdb=None,
    mnt_aa_rdb=None,
    image_fn=None,
    exp=None,
    pressure=652.0,
    temperature=-55.5,
    humidity=0.5,
    mount_type_eq=None,
  )
  simbad=True
  aa=sofa.transform_to_altaz(tf=star,sky=sky,simbad=simbad)
  star_obs_sofa=sofa.transform_to_radec(tf=star,sky=sky,simbad=simbad)

  print(star.ra.arcsec-star_obs_sofa.ra.arcsec)
  print(star.dec.arcsec-star_obs_sofa.dec.arcsec)




  
  star_obs_astr=astr.transform_to_radec(tf=star,sky=sky)
  print(star.ra.arcsec-star_obs_astr.ra.arcsec)
  print(star.dec.arcsec-star_obs_astr.dec.arcsec)

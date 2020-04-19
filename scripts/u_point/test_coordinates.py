#!/usr/bin/env python3

import sys
import argparse
import logging
import dateutil.parser
from ctypes import byref,c_double
import numpy as np
import astropy.units as u
from astropy.coordinates import SkyCoord
from astropy.coordinates import EarthLocation
from datetime import datetime
from astropy.time import Time,TimeDelta
from astropy.utils import iers
from astropy.utils import iers
iers.Conf.iers_auto_url.set('ftp://cddis.gsfc.nasa.gov/pub/products/iers/finals2000A.all')
#try:
#  iers.IERS.iers_table = iers.IERS_A.open(iers.IERS_A_FILE)
##                                               ###########
#except:
#  print('download:')
#  print('wget http://maia.usno.navy.mil/ser7/finals2000A.all')
#  sys.exit(1)

import ephem
from transform.u_astropy import Transformation as AstroPyTF
from transform.u_libnova import Transformation as LibnovaTF
from transform.u_libnova import ln,ln_pos_eq,ln_lnlat_posn,ln_pos_eq_pr 
from transform.u_pyephem import Transformation as PyEphemTF
from transform.u_skyfield import Transformation as SkyfieldTF

  
def arg_floats(value):
  return list(map(float, value.split()))

def arg_float(value):
  if 'm' in value:
    return -float(value[1:])
  else:
    return float(value)

def plot(as_x=None,as_y=None,ln_x=None,ln_y=None,pye_x=None,pye_y=None,ln_as_x=None,ln_as_y=None,pye_as_x=None,pye_as_y=None,pye_ln_x=None,pye_ln_y=None):

    import matplotlib
    import matplotlib.animation as animation
    # this varies from distro to distro:
    matplotlib.rcParams["backend"] = "TkAgg"
    import matplotlib.pyplot as plt
    plt.ioff()
    fig1 = plt.figure(figsize=(8,6))
    ax1 = fig1.add_subplot(111)
    ax1.set_title('Ecliptic pole to GCRS, red: astropy-a0, green: libnova-l0')
    ax1.scatter(as_x,as_y,color='red',s=300)
    ax1.scatter(ln_x,ln_y,color='green',s=50)
    ax1.scatter(pye_x,pye_y,color='blue',s=10)
    ax1.set_xlabel('RA [deg]')
    ax1.set_ylabel('Dec [deg]')
    ax1.grid(True)
    #plt.show()
    fig2 = plt.figure(figsize=(8,6))
    ax2 = fig2.add_subplot(111)
    ax2.set_title('Ecliptic pole to GCRS, difference blue:LN-AS,red:PYE-AS,green: PYE-LN')
    ax2.scatter(ln_as_x,ln_as_y,color='blue',s=100)
    ax2.scatter(pye_as_x,pye_as_y,color='red',s=50)
    ax2.scatter(pye_ln_x,pye_ln_y,color='green',s=20)
    ax2.set_xlabel('d_RA [arcsec]')
    ax2.set_ylabel('d_Dec [arcsec]')
    ax1.grid(True)
    plt.show()

if __name__ == "__main__":

  parser= argparse.ArgumentParser(prog=sys.argv[0], description='test precession, aberration in controlled environment')
  parser.add_argument('--level', dest='level', default='WARN', help=': %(default)s, debug level')
  parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')

  parser.add_argument('--obs-longitude', dest='obs_lng', action='store', default=123.2994166666666,type=arg_float, help=': %(default)s [deg], observatory longitude + to the East [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-latitude', dest='obs_lat', action='store', default=-75.1,type=arg_float, help=': %(default)s [deg], observatory latitude [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-height', dest='obs_height', action='store', default=3237.,type=arg_float, help=': %(default)s [m], observatory height above sea level [m], negative value: m10. equals to -10.')

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
  ast=AstroPyTF(lg=logger,obs=obs,refraction_method=None)
  lbn=LibnovaTF(lg=logger,obs=obs,refraction_method=None)
  pye=PyEphemTF(lg=logger,obs=obs,refraction_method=None)
  skf=SkyfieldTF(lg=logger,obs=obs,refraction_method=None)
  
  now_dt=datetime.utcnow()
  j2000_dt=dateutil.parser.parse('2000-01-01T00:00:0.0Z')
  j2100_dt=dateutil.parser.parse('2100-01-01T00:00:0.0Z')

  now=Time(now_dt, scale='utc',location=obs,out_subfmt='date_hms')
  j2000=Time(j2000_dt ,location=obs,out_subfmt='date_hms')
  j2100=Time(j2100_dt ,location=obs,out_subfmt='date_hms')
  
  epoch_dt=j2000_dt
  epoch=j2000
  # libnova precession
  #if False:
  #  ln_pos_eq.ra=0.
  #  ln_pos_eq.dec=0.
  #  #ln.ln_get_equ_prec(byref(ln_pos_eq), c_double(epoch.jd), byref(ln_pos_eq_pr))
  #  ln.ln_get_equ_prec(byref(ln_pos_eq), c_double(j2100.jd), byref(ln_pos_eq_pr))
  #  logger.info('LN     precession: ra: {0:9.6f} [deg], dec: {1:9.6f} [deg]'.format(ln_pos_eq_pr.ra,ln_pos_eq_pr.dec))
  #  sys.exit(1) 
  as_x=list()
  as_y=list()
  ln_x=list()
  ln_y=list()
  pye_x=list()
  pye_y=list()
  ln_as_x=list()
  ln_as_y=list()
  pye_as_x=list()
  pye_as_y=list()
  pye_ln_x=list()
  pye_ln_y=list()
  ec_lat=90.
  # astropy
  #  geocentrictrueecliptic??
  as_ec_0=SkyCoord(0*u.degree,ec_lat*u.degree, frame='barycentrictrueecliptic',location=obs,obstime=epoch,equinox=epoch)
  # libnova
  ln_lnlat_posn.lng=as_ec_0.lon.degree
  ln_lnlat_posn.lat=as_ec_0.lat.degree
  ln.ln_get_equ_from_ecl(byref(ln_lnlat_posn),c_double(epoch.jd),byref(ln_pos_eq))
  ln_ic_0=SkyCoord(ra=ln_pos_eq.ra,dec=ln_pos_eq.dec,unit=(u.degree,u.degree), frame='icrs',location=obs,obstime=epoch,equinox=epoch)
    
  # pyephem
  pye_ec_0=ephem.Ecliptic(ephem.degrees('{}'.format(as_ec_0.lon.degree)),ephem.degrees('{}'.format(as_ec_0.lat.degree)),epoch=j2000.datetime)
  pye_ic_0=ephem.FixedBody()
  
  pye_ic_0=ephem.Equatorial(pye_ec_0,epoch=j2100.datetime)
  as_gc_0=as_ec_0.gcrs
  as_ic_0=as_ec_0.icrs
  # canonical way
  as_lon,as_lat=as_ic_0.to_string(style='dms').split()
  ln_lon,ln_lat=ln_ic_0.to_string(style='dms').split()
  logger.info('AS     EC(ICRS): ra: {} [deg], dec: {} [deg]'.format(as_lon,as_lat))
  logger.info('LN     EC(ICRS): ra: {} [deg], dec: {} [deg]'.format(ln_lon,ln_lat))
  logger.info('PYE    EC(ICRS): ra: {} [deg], dec: {} [deg]'.format(pye_ic_0.ra,pye_ic_0.dec))
  logger.info('PYE-AS EC(ICRS): ra: {} [arcsec], dec: {} [arcsec]'.format((pye_ic_0.ra-as_ic_0.ra.radian) * 3600.*180./np.pi,(pye_ic_0.dec-as_ic_0.dec.radian) * 3600.*180./np.pi))
  sys.exit(1)
  
  ln_gc_0=lbn.LN_ICRS_to_GCRS(ra=as_ic_0.ra.degree,dec=as_ic_0.dec.degree,ln_pressure_qfe=0.,ln_temperature=0.,ln_humidity=0.,obstime=as_ic_0.obstime)
  star=pye.create_star(tf=as_ic_0) 
  pye_gc_0=SkyCoord(ra=star.g_ra,dec=star.g_dec, unit=(u.radian,u.radian), frame='gcrs',location=obs,obstime=now)
  
  for d_t in np.linspace(0.,366.,36.6):
    t_now= now + TimeDelta(float(d_t) *86400,format='sec')
    # transform the ecliptic pole to GCRS
    # geocentrictrueecliptic??
    as_ec = SkyCoord(0*u.degree,ec_lat*u.degree, frame='barycentrictrueecliptic',location=obs,obstime=t_now,equinox=j2000)# ToDo equinox think about that
    as_gc=as_ec.gcrs
    as_x.append(as_gc.ra.degree)
    as_y.append(as_gc.dec.degree)
    # Libnova
    ln_lnlat_posn.lng=as_ec.lon.degree
    ln_lnlat_posn.lat=as_ec.lat.degree
    # nutation is included in ln_get_equ_from_ecl
    #ln.ln_get_equ_from_ecl(byref(ln_lnlat_posn),c_double(t_now.jd),byref(ln_pos_eq))
    #ln_ic=SkyCoord(ra=ln_pos_eq.ra,dec=ln_pos_eq.dec,unit=(u.degree,u.degree), frame='icrs',location=obs,obstime=t_now,equinox=t_now) # ToDo equinox think about that
    as_ic=as_ec.icrs
    ln_ic=SkyCoord(ra=as_ic.ra,dec=as_ic.dec,unit=(u.radian,u.radian), frame='icrs',location=obs,obstime=t_now,equinox=j2000) # ToDo equinox think about that
    ln_gc=lbn.LN_ICRS_to_GCRS(ra=ln_ic.ra.degree,dec=ln_ic.dec.degree,ln_pressure_qfe=0.,ln_temperature=0.,ln_humidity=0.,obstime=ln_ic.obstime)
    ln_x.append(ln_gc.ra.degree)
    ln_y.append(ln_gc.dec.degree)
    # PyEphem
    pye_ec=ephem.Ecliptic(ephem.degrees('{}'.format(as_ec.lon.degree)),ephem.degrees('{}'.format(as_ec.lat.degree)),epoch=j2000.datetime)
    pye_ic=ephem.Equatorial(pye_ec,epoch=t_now.datetime) 
    pye_gc=SkyCoord(ra=star.ra,dec=star.dec, unit=(u.radian,u.radian), frame='gcrs',location=obs,obstime=t_now)
    pye_x.append(pye_gc.ra.degree)
    pye_y.append(pye_gc.dec.degree)
    # difference
    ln_as_x.append(ln_gc.ra.arcsec-as_gc.ra.arcsec)
    ln_as_y.append(ln_gc.dec.arcsec-as_gc.dec.arcsec)
    pye_as_x.append(pye_gc.ra.arcsec-as_gc.ra.arcsec)
    pye_as_y.append(pye_gc.dec.arcsec-as_gc.dec.arcsec)
    pye_ln_x.append(pye_gc.ra.arcsec-ln_gc.ra.arcsec)
    pye_ln_y.append(pye_gc.dec.arcsec-ln_gc.dec.arcsec)

  plot(as_x=as_x,as_y=as_y,ln_x=ln_x,ln_y=ln_y,pye_x=pye_x,pye_y=pye_y,ln_as_x=ln_as_x,ln_as_y=ln_as_y,pye_as_x=pye_as_x,pye_as_y=pye_as_y,pye_ln_x=pye_ln_x,pye_ln_y=pye_ln_y)

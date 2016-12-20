#!/usr/bin/env python3

import sys
import argparse
import logging

import numpy as np
import astropy.units as u
from astropy.coordinates import SkyCoord
from astropy.coordinates import EarthLocation
from datetime import datetime
from astropy.time import Time,TimeDelta
from astropy.utils import iers
try:
  iers.IERS.iers_table = iers.IERS_A.open(iers.IERS_A_FILE)
#                                               ###########
except:
  print('download:')
  print('wget http://maia.usno.navy.mil/ser7/finals2000A.all')
  sys.exit(1)

from transform.u_astropy import Transformation as AstroPyTF
from transform.u_libnova import Transformation as LibnovaTF

  
def arg_floats(value):
  return list(map(float, value.split()))

def arg_float(value):
  if 'm' in value:
    return -float(value[1:])
  else:
    return float(value)

def plot(as_x=None,as_y=None,ln_x=None,ln_y=None,ln_as_x=None,ln_as_y=None):

    import matplotlib
    import matplotlib.animation as animation
    # this varies from distro to distro:
    matplotlib.rcParams["backend"] = "TkAgg"
    import matplotlib.pyplot as plt
    plt.ioff()
    fig = plt.figure(figsize=(8,6))
    ax1 = fig.add_subplot(111)#, projection="mollweide")
    ax1.set_title('Ecliptic pole to GCRS, red: astropy, gren: libnova, blue: LN-AS (*frac-off) ')
    ax1.scatter(as_x,as_y,color='red',s=300)
    ax1.scatter(ln_x,ln_y,color='green',s=50)
    ax1.scatter(ln_as_x,ln_as_y,color='blue',s=100)
    ax1.set_xlabel('d_RA [arcsec]')
    ax1.set_ylabel('d_Dec [arcsec]')
    ax1.grid(True)

    plt.show()
if __name__ == "__main__":

  parser= argparse.ArgumentParser(prog=sys.argv[0], description='test precession, aberration in controlled environment')
  parser.add_argument('--level', dest='level', default='WARN', help=': %(default)s, debug level')
  parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
  parser.add_argument('--break_after', dest='break_after', action='store', default=10000000, type=int, help=': %(default)s, read max. positions, mostly used for debuging')

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
  now=Time(datetime.utcnow(), scale='utc',location=obs,out_subfmt='date_hms')
  as_x=list()
  as_y=list()
  ln_x=list()
  ln_y=list()
  ln_as_x=list()
  ln_as_y=list()
  ec_0=SkyCoord(0*u.degree,-89.99*u.degree, frame='geocentrictrueecliptic',location=obs,obstime=now)
  as_gc_0=ec_0.gcrs
  ic_0=ec_0.icrs
  ln_gc_0=lbn.LN_ICRS_to_GCRS(ra=ic_0.ra.degree,dec=ic_0.dec.degree,ln_pressure_qfe=0.,ln_temperature=0.,ln_humidity=0.,obstime=ic_0.obstime,apparent=False)

  for d_t in range(0,360,10):
    t= now + TimeDelta(float(d_t) *86400,format='sec')
    # transform the ecliptic pole to GCRS
    # 
    ec = SkyCoord(0*u.degree,-89.99*u.degree, frame='geocentrictrueecliptic',location=obs,obstime=t)
    as_gc=ec.gcrs
    as_x.append(as_gc.ra.arcsec-as_gc_0.ra.arcsec)
    as_y.append(as_gc.dec.arcsec-as_gc_0.dec.arcsec)
    ic=ec.icrs
    ln_gc=lbn.LN_ICRS_to_GCRS(ra=ic.ra.degree,dec=ic.dec.degree,ln_pressure_qfe=0.,ln_temperature=0.,ln_humidity=0.,obstime=ic.obstime,apparent=False)
    ln_x.append(ln_gc.ra.arcsec-ln_gc_0.ra.arcsec)
    ln_y.append(ln_gc.dec.arcsec-ln_gc_0.dec.arcsec)
    #
    # difference
    frac=100.
    ln_as_x.append((ln_gc.ra.arcsec-as_gc.ra.arcsec)*frac+150.)
    ln_as_y.append((ln_gc.dec.arcsec-as_gc.dec.arcsec)*frac-20.)

  plot(as_x=as_x,as_y=as_y,ln_x=ln_x,ln_y=ln_y,ln_as_x=ln_as_x,ln_as_y=ln_as_y)

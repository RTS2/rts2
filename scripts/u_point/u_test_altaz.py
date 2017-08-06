#!/usr/bin/env python3

import sys
import argparse
import logging
import dateutil.parser

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
from transform.u_pyephem import Transformation as PyEphemTF
from transform.u_libnova import Transformation as LibnovaTF
from transform.u_taki_san import Transformation as TakiSanTF
from transform.u_skyfield import Transformation as SkyfieldTF
from transform.u_sofa import Transformation as SofaTF


def plot(red_x=None,red_y=None,green_x=None,green_y=None,blue_x=None,blue_y=None,magenta_x=None,magenta_y=None,cyan_x=None,cyan_y=None,black_x=None,black_y=None,yellow_x=None,yellow_y=None,ln_apparent=None):

    import matplotlib
    import matplotlib.animation as animation
    # this varies from distro to distro:
    matplotlib.rcParams["backend"] = "TkAgg"
    import matplotlib.pyplot as plt
    plt.ioff()
    fig = plt.figure(figsize=(8,6))
    ax1 = fig.add_subplot(111)

    title='Difference color-SOFA, red:Libnova,green:AstroPy,blue:Skyfield,magenta:PyEphem'
      
    ax1.set_title(title,fontsize=10)
    ax1.scatter(red_x,red_y,color='red')
    ax1.scatter(green_x,green_y,color='green')
    ax1.scatter(blue_x,blue_y,color='blue')
    ax1.scatter(magenta_x,magenta_y,color='magenta')
    ax1.scatter(cyan_x,cyan_y,color='cyan')
    ax1.scatter(black_x,black_y,color='black')
    ax1.scatter(yellow_x,yellow_y,color='black',s=50)
    ax1.scatter(yellow_x,yellow_y,color='yellow',s=30)
    #
    ax1.set_xlabel('d_azimuth [arcsec]')
    ax1.set_ylabel('d_altitude [arcsec]')
    ax1.grid(True)
    plt.show()


def arg_float(value):
  if 'm' in value:
    return -float(value[1:])
  else:
    return float(value)

if __name__ == "__main__":

  parser= argparse.ArgumentParser(prog=sys.argv[0], description='test precession, aberration in controlled environment')
  parser.add_argument('--level', dest='level', default='WARN', help=': %(default)s, debug level')
  parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
  parser.add_argument('--obs-longitude', dest='obs_lng', action='store', default=123.2994166666666,type=arg_float, help=': %(default)s [deg], observatory longitude + to the East [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-latitude', dest='obs_lat', action='store', default=-75.1,type=arg_float, help=': %(default)s [deg], observatory latitude [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-height', dest='obs_height', action='store', default=3237.,type=arg_float, help=': %(default)s [m], observatory height above sea level [m], negative value: m10. equals to -10.')
  parser.add_argument('--ln-apparent', dest='ln_apparent', action='store_true', default=False, help=': %(default)s, True: libnova coordinates are transformed to apparent')
  parser.add_argument('--declination', dest='declination', action='store', default=50.,type=arg_float, help=': %(default)s [deg], declination of the object')

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
  dummy=1
  tks=TakiSanTF(lg=logger,obs=obs,refraction_method=dummy)
  skf=SkyfieldTF(lg=logger,obs=obs,refraction_method=dummy)
  sfa=SofaTF(lg=logger,obs=obs,refraction_method=dummy)
  now=Time(dateutil.parser.parse('2013-04-04T23:15:43.55Z'), scale='utc',location=obs,out_subfmt='date_hms')

  ln_sf_x=list()
  ln_sf_y=list()
  as_sf_x=list()
  as_sf_y=list()
  sk_sf_x=list()
  sk_sf_y=list()
  pe_sf_x=list()
  pe_sf_y=list()
  
  for RA in np.linspace(0.,360.,36.):
  #for RA in np.linspace(0.,90.,9.):
    icrs=SkyCoord(RA*u.degree,args.declination*u.degree, frame='icrs',location=obs,obstime=now,pressure=0.)
    ast_aa=ast.transform_to_altaz(tf=icrs,sky=None)
    ln_aa=lbn.LN_ICRS_to_AltAz(ra=icrs.ra.degree,dec=icrs.dec.degree,ln_pressure_qfe=0.,ln_temperature=0.,ln_humidity=0.,obstime=icrs.obstime)   
    pye_aa=pye.transform_to_altaz(tf=icrs,sky=None)
    skf_aa=skf.transform_to_altaz(tf=icrs,sky=None)
    sfa_aa=sfa.transform_to_altaz(tf=icrs,sky=None)
    #tks_aa=tks.transform_to_altaz(tf=icrs,sky=None)
    

    # Libnova S=0, W=90
    daz=(ln_aa.az.radian-np.pi)-sfa_aa.az.radian
    if daz < -np.pi:
      daz += 2.*np.pi
    sign=1.
    ln_sf_x.append(sign * daz *60.* 180./np.pi)
    ln_sf_y.append(( sign * (ln_aa.alt.arcsec-sfa_aa.alt.arcsec)))
    # astropy N=0, E=90
    daz=ast_aa.az.radian-sfa_aa.az.radian
    as_sf_x.append(daz *60.* 180./np.pi) 
    as_sf_y.append(ast_aa.alt.arcsec-sfa_aa.alt.arcsec)

    # Skyfield N=0, E=90
    daz=skf_aa.az.radian-sfa_aa.az.radian
    sk_sf_x.append(daz *60.* 180./np.pi) 
    sk_sf_y.append(skf_aa.alt.arcsec-sfa_aa.alt.arcsec)

    # PyEphem N=0, E=90
    daz=pye_aa.az.radian-sfa_aa.az.radian
    pe_sf_x.append(daz *60.* 180./np.pi) 
    pe_sf_y.append(pye_aa.alt.arcsec-sfa_aa.alt.arcsec)
    
    
  plot(red_x=ln_sf_x,red_y=ln_sf_y,
       green_x=as_sf_x,green_y=as_sf_y,
       blue_x=sk_sf_x,blue_y=sk_sf_y,
       magenta_x=pe_sf_x,magenta_y=pe_sf_y,
       )




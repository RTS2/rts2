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
from transform.u_pyephem import Transformation as PyEphemTF
from transform.u_libnova import Transformation as LibnovaTF
from transform.u_taki_san import Transformation as TakiSanTF


def plot(r_x=None,r_y=None,g_x=None,g_y=None,b_x=None,b_y=None,m_x=None,m_y=None,ln_apparent=None):

    import matplotlib
    import matplotlib.animation as animation
    # this varies from distro to distro:
    matplotlib.rcParams["backend"] = "TkAgg"
    import matplotlib.pyplot as plt
    plt.ioff()
    fig = plt.figure(figsize=(8,6))
    ax1 = fig.add_subplot(111)#, projection="mollweide")

    title='red: TS-AS, green: LN-AS, blue: LN-TS, magenta: TS-PYE'
    if ln_apparent:
      title += ',LN:TF to apparent'
    else:
      title += ',LN:TF geometrical'
      
    ax1.set_title(title)
    ax1.scatter(r_x,r_y,color='red')
    ax1.scatter(g_x,g_y,color='green')
    ax1.scatter(b_x,b_y,color='blue')
    ax1.scatter(m_x,m_y,color='magenta')
    #
    ax1.set_xlabel('d_azimuth [arcmin]')
    ax1.set_ylabel('d_altitude [arcmin]')
    ax1.grid(True)
    plt.show()


def arg_floats(value):
  return list(map(float, value.split()))

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
  now=Time(datetime.utcnow(), scale='utc',location=obs,out_subfmt='date_hms')

  ts_as_x=list()
  ts_as_y=list()
  ln_as_x=list()
  ln_as_y=list()
  ln_ts_x=list()
  ln_ts_y=list()
  ts_py_x=list()
  ts_py_y=list()

  for RA in range(0,360,10):
    gc=SkyCoord(float(RA)*u.degree,args.declination*u.degree, frame='gcrs',location=obs,obstime=now,pressure=0.)
    ast_aa=ast.transform_to_altaz(tf=gc,sky=None,apparent=None)
    ln_aa=lbn.LN_GCRS_to_AltAz(ra=gc.ra.degree,dec=gc.dec.degree,ln_pressure_qfe=0.,ln_temperature=0.,ln_humidity=0.,obstime=gc.obstime,apparent=args.ln_apparent)   
    # Taki - astropy
    tks_aa=tks.transform_to_altaz(tf=gc,sky=None,apparent=None)
    daz=(tks_aa.az.radian+np.pi)-ast_aa.az.radian
    if daz > np.pi:
      daz -= 2.*np.pi 
    ts_as_x.append(daz *60.* 180./np.pi) # westward from south
    ts_as_y.append(tks_aa.alt.arcmin-ast_aa.alt.arcmin)

    # Libnova -astropy
    daz=(ln_aa.az.radian-np.pi)-ast_aa.az.radian
    if daz < -np.pi:
      daz += 2.*np.pi
    frac= 1.
    ln_as_x.append(daz *60.* 180./np.pi *frac)# * 60. *180./np.pi) # westward from south
    ln_as_y.append((ln_aa.alt.arcmin-ast_aa.alt.arcmin)*frac)

    # Libnova - Taki
    daz=(ln_aa.az.radian-np.pi)-tks_aa.az.radian+np.pi
    if daz > np.pi:
      daz -= 2.*np.pi
    fac=-1.
    ln_ts_x.append(daz *60.* 180./np.pi * fac)# * 60. *180./np.pi) # westward from south
    ln_ts_y.append((ln_aa.alt.arcmin-tks_aa.alt.arcmin)*fac)
    
    # Taki - PyEphem
    pye_aa=pye.transform_to_altaz(tf=gc,sky=None,apparent=None)
    daz=(tks_aa.az.radian+np.pi)-pye_aa.az.radian
    if daz > np.pi:
      daz -= 2.*np.pi 
    ts_py_x.append(daz *60.* 180./np.pi) # westward from south
    ts_py_y.append(tks_aa.alt.arcmin-pye_aa.alt.arcmin)
    
    
  plot(r_x=ts_as_x,r_y=ts_as_y,g_x=ln_as_x,g_y=ln_as_y,b_x=ln_ts_x,b_y=ln_ts_y,m_x=ts_py_x,m_y=ts_py_y,ln_apparent=args.ln_apparent)




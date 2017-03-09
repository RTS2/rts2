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


def plot(red_x=None,red_y=None,green_x=None,green_y=None,blue_xx=None,blue_xy=None,magenta_x=None,magenta_y=None,cyan_x=None,cyan_y=None,black_x=None,black_y=None,yellow_x=None,yellow_y=None,ln_apparent=None):

    import matplotlib
    import matplotlib.animation as animation
    # this varies from distro to distro:
    matplotlib.rcParams["backend"] = "TkAgg"
    import matplotlib.pyplot as plt
    plt.ioff()
    fig = plt.figure(figsize=(8,6))
    ax1 = fig.add_subplot(111)#, projection="mollweide")

    title='red:TS-AS,green:LN-AS,blue:LN-TS,magenta:SF-AS,cyan:LN-PYE,black:AS-PYE,yellow:SKF-PYE'
    if ln_apparent:
      title += ',LN:TF to apparent'
    else:
      title += ',LN:TF geometrical'
      
    ax1.set_title(title,fontsize=10)
    ax1.scatter(red_x,red_y,color='red')
    ax1.scatter(green_x,green_y,color='green')
    ax1.scatter(blue_xx,blue_xy,color='blue')
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
  skf=SkyfieldTF(lg=logger,obs=obs,refraction_method=dummy)
  sfa=SofaTF(lg=logger,obs=obs,refraction_method=dummy)
  now=Time(dateutil.parser.parse('2013-04-04T23:15:43.55Z'), scale='utc',location=obs,out_subfmt='date_hms')

  ##now=Time(datetime.utcnow(), scale='utc',location=obs,out_subfmt='date_hms')

  RA_0= now.sidereal_time('apparent') - 0. # HA=0.
  ra_0=SkyCoord(ra=RA_0,dec=0., unit=(u.rad,u.rad),frame='icrs',location=obs,obstime=now,pressure=0.)
  ln_aa_0=lbn.LN_ICRS_to_AltAz(ra=ra_0.ra.degree,dec=ra_0.dec.degree,ln_pressure_qfe=0.,ln_temperature=0.,ln_humidity=0.,obstime=now)   
  logger.info('LN  HA=0.: {0:+11.6f}, {1:+11.6f}'.format(ln_aa_0.az.deg,ln_aa_0.alt.deg))
  ast_aa_0=ast.transform_to_altaz(tf=ra_0,sky=None)
  logger.info('AS  HA=0.: {0:+11.6f}, {1:+11.6f}'.format(ast_aa_0.az.deg,ast_aa_0.alt.deg))
  tks_aa_0=tks.transform_to_altaz(tf=ra_0,sky=None)
  logger.info('TS  HA=0.: {0:+11.6f}, {1:+11.6f}'.format(tks_aa_0.az.deg,tks_aa_0.alt.deg))
  pye_aa_0=pye.transform_to_altaz(tf=ra_0,sky=None)
  logger.info('PYE HA=0.: {0:+11.6f}, {1:+11.6f}'.format(pye_aa_0.az.deg,pye_aa_0.alt.deg))
  skf_aa_0=skf.transform_to_altaz(tf=ra_0,sky=None)
  logger.info('SKF HA=0.: {0:+11.6f}, {1:+11.6f}'.format(skf_aa_0.az.deg,skf_aa_0.alt.deg))
  sfa_aa_0=sfa.transform_to_altaz(tf=ra_0,sky=None)
  logger.info('SFA HA=0.: {0:+11.6f}, {1:+11.6f}'.format(sfa_aa_0.az.deg,sfa_aa_0.alt.deg))

  ts_as_x=list()
  ts_as_y=list()
  ln_as_x=list()
  ln_as_y=list()
  ln_ts_x=list()
  ln_ts_y=list()
  #ts_py_x=list()
  #ts_py_y=list()
  ln_py_x=list()
  ln_py_y=list()
  as_py_x=list()
  as_py_y=list()
  sk_py_x=list()
  sk_py_y=list()
  sf_as_x=list()
  sf_as_y=list()

  for RA in np.linspace(0.,360.,36.):
    #gc=SkyCoord(float(RA)*u.degree,args.declination*u.degree, frame='gcrs',location=obs,obstime=now,pressure=0.)
    icrs=SkyCoord(RA*u.degree,args.declination*u.degree, frame='icrs',location=obs,obstime=now,pressure=0.)
    rs=icrs.gcrs
    
    ast_aa=ast.transform_to_altaz(tf=rs,sky=None,mount_set_icrs=None)
    ln_aa=lbn.LN_ICRS_to_AltAz(ra=rs.ra.degree,dec=rs.dec.degree,ln_pressure_qfe=0.,ln_temperature=0.,ln_humidity=0.,obstime=rs.obstime,mount_set_icrs=args.ln_apparent)   
    pye_aa=pye.transform_to_altaz(tf=rs,sky=None,mount_set_icrs=None)
    skf_aa=skf.transform_to_altaz(tf=rs,sky=None,mount_set_icrs=None)
    sfa_aa=sfa.transform_to_altaz(tf=rs,sky=None,mount_set_icrs=None)
    # Taki - astropy
    tks_aa=tks.transform_to_altaz(tf=rs,sky=None,mount_set_icrs=None)
    daz=(tks_aa.az.radian+np.pi)-ast_aa.az.radian
    if daz > np.pi:
      daz -= 2.*np.pi 
    ts_as_x.append(daz *60.* 180./np.pi) # westward from south
    ts_as_y.append(tks_aa.alt.arcsec-ast_aa.alt.arcsec)

    # Libnova -astropy
    daz=(ln_aa.az.radian-np.pi)-ast_aa.az.radian
    if daz < -np.pi:
      daz += 2.*np.pi
    frac= 1.
    ln_as_x.append(daz *60.* 180./np.pi *frac)# * 60. *180./np.pi) # westward from south
    ln_as_y.append((ln_aa.alt.arcsec-ast_aa.alt.arcsec)*frac)

    # Libnova - Taki
    daz=(ln_aa.az.radian-np.pi)-tks_aa.az.radian+np.pi
    if daz > np.pi:
      daz -= 2.*np.pi
    fac=-1.
    ln_ts_x.append(daz *60.* 180./np.pi * fac)# * 60. *180./np.pi) # westward from south
    ln_ts_y.append((ln_aa.alt.arcsec-tks_aa.alt.arcsec)*fac)
    
    # Taki - PyEphem
    #daz=(tks_aa.az.radian+np.pi)-pye_aa.az.radian
    #if daz > np.pi:
    #  daz -= 2.*np.pi 
    #ts_py_x.append(daz *60.* 180./np.pi) # westward from south
    #ts_py_y.append(tks_aa.alt.arcsec-sfa_aa.alt.arcsec)
    # Libnova - PyEphem
    daz=(ln_aa.az.radian-np.pi)-pye_aa.az.radian
    if daz < -np.pi:
      daz += 2.*np.pi 
    ln_py_x.append(daz *60.* 180./np.pi) # westward from south
    ln_py_y.append(ln_aa.alt.arcsec-sfa_aa.alt.arcsec)
    
    # astropy - PyEphem
    daz=(ast_aa.az.radian)-pye_aa.az.radian
    if daz < -np.pi:
      daz += 2.*np.pi 
    as_py_x.append(daz *60.* 180./np.pi) 
    as_py_y.append(ast_aa.alt.arcsec-sfa_aa.alt.arcsec)

    # Skyfield - PyEphem
    daz=(skf_aa.az.radian)-pye_aa.az.radian
    if daz > np.pi:
      daz -= 2.*np.pi 
    sk_py_x.append(daz *60.* 180./np.pi) 
    sk_py_y.append(skf_aa.alt.arcsec-sfa_aa.alt.arcsec)
    # SOFA - Astropy
    daz=(ast_aa.az.radian)-sfa_aa.az.radian
    if daz > np.pi:
      daz -= 2.*np.pi 
    sf_as_x.append(daz *60.* 180./np.pi) 
    sf_as_y.append(skf_aa.alt.arcsec-sfa_aa.alt.arcsec)

    
  plot(red_x=ts_as_x,red_y=ts_as_y,
       green_x=ln_as_x,green_y=ln_as_y,
       blue_xx=ln_ts_x,blue_xy=ln_ts_y,
       magenta_x=sf_as_x,magenta_y=sf_as_y,
       cyan_x=ln_py_x,cyan_y=ln_py_y,
       black_x=as_py_x,black_y=as_py_y,
       yellow_x=sk_py_x,yellow_y=sk_py_y,
       ln_apparent=args.ln_apparent)




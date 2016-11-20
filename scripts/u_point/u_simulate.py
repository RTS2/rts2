#!/usr/bin/env python3
#
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
'''
Crate data for u_point.py

AltAz: Astropy N=0,E=pi/2, Libnova S=0,W=pi/1

'''
__author__ = 'wildi.markus@bluewin.ch'

import sys
import argparse
import logging
import numpy as np
from random import uniform
import scipy.optimize
from astropy import units as u
from astropy.time import Time,TimeDelta
from astropy.coordinates import SkyCoord,EarthLocation
from astropy.coordinates import AltAz,CIRS,ITRS
from astropy.coordinates import Longitude,Latitude,Angle

# Python bindings for libnova
from ctypes import *
class LN_equ_posn(Structure):
  _fields_ = [("ra", c_double),("dec", c_double)]

class LN_hrz_posn(Structure):
  _fields_ = [("az", c_double),("alt", c_double)]

class LN_lnlat_posn(Structure):
  _fields_ = [("lng", c_double),("lat", c_double)]

# add full path if it is not on LD_PATH
ln=cdll.LoadLibrary("libnova.so")
ln.ln_get_equ_aber.restype = None
ln.ln_get_equ_prec.restype = None
ln.ln_get_refraction_adj.restype = c_double

ln_pos_eq=LN_equ_posn()
ln_pos_eq_ab=LN_equ_posn()
ln_pos_eq_pm=LN_equ_posn()
ln_pos_eq_app=LN_equ_posn()
ln_pos_eq_pr=LN_equ_posn()
ln_pos_aa_pr=LN_hrz_posn()
ln_hrz_posn=LN_hrz_posn()

class Parameter:
  def __init__(self, value):
    self.value = value
  def set(self, value):
    self.value = value
  def __call__(self):
    return self.value

# J.Condon 1992
# Hamburg:
# PUBLICATIONS OF THE ASTRONOMICAL SOCIETY OF THE PACIFIC, 120:425–429, 2008 April
# The Temperature Dependence of the Pointing Model of the Hamburg Robotic Telescope
#
def d_az(azs,alts,d_azs):
  #    C1   C2                C3                C4                            C5             
  #                                            NO!! (see C4 below): yes, minus! see paper Hamburg
  #                                            |
  #val=(C1()+C2()*np.cos(alts)+C3()*np.sin(alts)+C4()*np.cos(azs)*np.sin(alts)+C5()*np.sin(azs)*np.sin(alts))/np.cos(alts) 
  val=(C1()\
       +C2()*np.cos(alts)\
       +C3()*np.sin(alts)\
       +C4()*np.cos(azs)*np.sin(alts)\
       +C5()*np.sin(azs)*np.sin(alts))/np.cos(alts) 
  return val-d_azs

def d_alt(azs,alts,d_alts):
  # this the way to expand the pointing model
  # Condon 1992, see page 6, Eq. 5: D_N cos(A) - D_W sin(A)
  #              d_alt equation is wrong on p.7
  #   C6   C7                C4               C5          
  #                         minus sign here
  #                         |
  #val=C6()+C7()*np.cos(alts)-C4()*np.sin(azs)+C5()*np.cos(azs)
  val=C6()\
       +C7()*np.cos(alts)\
       -C4()*np.sin(azs)\
       +C5()*np.cos(azs)

  return val-d_alts
# T-Point
global d_ha_t_point
def d_ha_t_point(has_cat,decs_cat,d_has,phi):
  val= IH()\
       +CH()/np.cos(decs_cat)\
       +NP()*np.tan(decs_cat)\
       -MA()*np.cos(has_cat)*np.tan(decs_cat)\
       +ME()*np.sin(has_cat)*np.tan(decs_cat)\
       +TF()*np.cos(phi)*np.sin(has_cat)/np.cos(decs_cat)\
       -DAF()*(np.cos(phi)*np.cos(has_cat)+np.sin(phi)*np.tan(decs_cat))
  return val-d_has

global d_dec_t_point
def d_dec_t_point(has_cat,decs_cat,d_decs,phi):
  val=ID()\
      +MA()*np.sin(has_cat)\
      +ME()*np.cos(has_cat)\
      +TF()*(np.cos(phi)*np.cos(has_cat)*np.sin(decs_cat)-np.sin(phi)*np.cos(decs_cat))\
      +FO()*np.cos(has_cat)
  return val-d_decs

# Marc W. Buie 2003
def d_ha(has_cat,decs_cat,d_has,phi):
  # ToDo: has_cat-theta(), this is the mount ha1
  val= Dt()\
       -gamma()*np.sin(has_cat-theta())*np.tan(decs_cat)\
       +c()/np.cos(decs_cat)\
       -ip()*np.tan(decs_cat)\
       +ep()*np.cos(phi)*1./np.cos(decs_cat)*np.sin(has_cat)\
       +l()*(np.sin(phi)*np.tan(decs_cat)+np.cos(decs_cat)*np.cos(has_cat))\
       +r()*has_cat
  return val-d_has

def d_dec(has_cat,decs_cat,d_decs,phi):
  # ToDo: has_cat-theta(), this is the mount ha1
  val=Dd()\
      -gamma()*np.cos(has_cat-theta())\
      -ep()*(np.sin(phi)*np.cos(decs_cat)-np.cos(phi)*np.sin(decs_cat)*np.cos(has_cat))
  return val-d_decs

class SimulationModel(object):
  def __init__(self, dbg=None,lg=None, obs_lng=None, obs_lat=None, obs_height=None):
    self.dbg=dbg
    self.lg=lg
    #
    self.obs=EarthLocation(lon=float(obs_lng)*u.degree, lat=float(obs_lat)*u.degree, height=float(obs_height)*u.m)
    #
    self.ln_obs=LN_lnlat_posn()
    self.ln_obs.lng=obs_lng     # deg
    self.ln_obs.lat=obs_lat     # deg
    self.ln_hght=obs_height     # m, not a libnova quantity

  def LN_AltAz_to_EQ(self,az=None,alt=None,obstime=None):
    ln_hrz_posn.alt=alt
    ln_hrz_posn.az=az
    
    ln.ln_get_equ_from_hrz(byref(ln_hrz_posn),byref(self.ln_obs),c_double(obstime.jd),byref(ln_pos_eq))
    pos_eq=SkyCoord(ra=ln_pos_eq.ra,dec=ln_pos_eq.dec,unit=(u.degree,u.degree),frame='icrs',obstime=obstime)
    return pos_eq
  
  def LN_EQ_to_AltAz(self,ra=None,dec=None,ln_pressure_qfe=None,ln_temperature=None,ln_humidity=None,obstime=None,correct_cat=False):

    ln_pos_eq.ra=ra
    ln_pos_eq.dec=dec
    if correct_cat:
      # libnova corrections for catalog data ...
      # ToDo missing (see P.T. Wallace: Telescope Pointing)
      # propper motion
      # annual aberration
      # light deflection
      # annual parallax
      # diurnal aberration
      # polar motion 
      ln.ln_get_equ_aber(byref(ln_pos_eq), c_double(obstime.jd), byref(ln_pos_eq_ab))
      ln.ln_get_equ_prec(byref(ln_pos_eq_ab), c_double(obstime.jd), byref(ln_pos_eq_pr))
      ln.ln_get_hrz_from_equ(byref(ln_pos_eq_pr), byref(self.ln_obs), c_double(obstime.jd), byref(ln_pos_aa_pr))
      # here we use QFE not pressure at sea level!
      # E.g. at Dome-C this formula:
      ###ln_pressure=ln_see_pres * pow(1. - (0.0065 * ln_alt) / 288.15, (9.80665 * 0.0289644) / (8.31447 * 0.0065));
      # is not precise.
      d_alt_deg= ln.ln_get_refraction_adj(c_double(ln_pos_aa_pr.alt),c_double(ln_pressure_qfe),c_double(ln_temperature))
    else:
      # ... but not for the star position as measured in telescope frame
      ln.ln_get_hrz_from_equ(byref(ln_pos_eq), byref(self.ln_obs), c_double(obstime.jd), byref(ln_pos_aa_pr));
      d_alt_deg=0.
  
    a_az=Longitude(ln_pos_aa_pr.az,u.deg)
    # add refraction
    a_alt=Latitude(ln_pos_aa_pr.alt + d_alt_deg,u.deg)
    pos_aa=SkyCoord(az=a_az.radian,alt=a_alt.radian,unit=(u.radian,u.radian),frame='altaz',location=self.obs,obstime=obstime,obswl=0.5*u.micron, pressure=ln_pressure_qfe*u.hPa,temperature=ln_temperature*u.deg_C,relative_humidity=ln_humidity)
    return pos_aa
  
  def transform_to_altaz(self,eq=None,tem=None,pre=None,hum=None,astropy_f=False,correct_cat_f=None):
    '''
    There are substancial differences between astropy and libnova apparent coordinates. 
    Choose option --astropy for Astropy, default is Libnova
    '''
    if astropy_f:      
      # https://github.com/liberfa/erfa/blob/master/src/refco.c
      # phpa   double    pressure at the observer (hPa = millibar)
      pre_qfe=pre # to make it clear what is used
      pos_aa=eq.transform_to(AltAz(location=self.obs,obswl=0.5*u.micron, pressure=pre_qfe*u.hPa,temperature=tem*u.deg_C,relative_humidity=hum))
    else:
      pre_qfe=pre # to make it clear what is used
      pos_aa=self.LN_EQ_to_AltAz(ra=Longitude(eq.ra.radian,u.radian).degree,dec=Latitude(eq.dec.radian,u.radian).degree,ln_pressure_qfe=pre_qfe,ln_temperature=tem,ln_humidity=hum,obstime=eq.obstime,correct_cat=correct_cat_f)

    return pos_aa
  

  def print_altaz(self):
    self.lg.info('input parameters')
    self.lg.info('C1: horizontal telescope collimation:{0:+10.4f} [arcsec]'.format(C1()*180.*3600./np.pi))
    self.lg.info('C2: constant azimuth offset         :{0:+10.4f} [arcsec]'.format(C2()*180.*3600./np.pi))
    self.lg.info('C3: tipping-mount collimation       :{0:+10.4f} [arcsec]'.format(C3()*180.*3600./np.pi))
    self.lg.info('C4: azimuth axis tilt West          :{0:+10.4f} [arcsec]'.format(C4()*180.*3600./np.pi))
    self.lg.info('C5: azimuth axis tilt North         :{0:+10.4f} [arcsec]'.format(C5()*180.*3600./np.pi))
    self.lg.info('C6: vertical telescope collimation  :{0:+10.4f} [arcsec]'.format(C6()*180.*3600./np.pi))
    self.lg.info('C7: gravitational tube bending      :{0:+10.4f} [arcsec]'.format(C7()*180.*3600./np.pi))
    self.lg.info('end input parameters')

  def print_hadec(self,t_point=False):
    self.lg.info('input parameters')
    if t_point:
      self.lg.info('IH : ha index error                 :{0:+12.4f} [arcsec]'.format(IH()*180.*3600./np.pi))
      self.lg.info('ID : delta index error              :{0:+12.4f} [arcsec]'.format(ID()*180.*3600./np.pi))
      self.lg.info('CH : collimation error              :{0:+12.4f} [arcsec]'.format(CH()*180.*3600./np.pi))
      self.lg.info('NP : ha/delta non-perpendicularity  :{0:+12.4f} [arcsec]'.format(NP()*180.*3600./np.pi))
      self.lg.info('MA : polar axis left-right alignment:{0:+12.4f} [arcsec]'.format(MA()*180.*3600./np.pi))
      self.lg.info('ME : polar axis verticsl alignment  :{0:+12.4f} [arcsec]'.format(ME()*180.*3600./np.pi))
      self.lg.info('TF : tube flexure                   :{0:+12.4f} [arcsec]'.format(TF()*180.*3600./np.pi))
      self.lg.info('FO : fork flexure                   :{0:+12.4f} [arcsec]'.format(FO()*180.*3600./np.pi))
      self.lg.info('DAF: dec axis flexure               :{0:+12.4f} [arcsec]'.format(DAF()*180.*3600./np.pi))
    else:
      self.lg.info('Dd:    declination zero-point offset    :{0:+12.4f} [arcsec]'.format(Dd()*180.*3600./np.pi))
      self.lg.info('Dt:    hour angle zero-point offset     :{0:+12.4f} [arcsec]'.format(Dt()*180.*3600./np.pi))
      self.lg.info('ip:    angle(polar,declination) axis    :{0:+12.4f} [arcsec]'.format(ip()*180.*3600./np.pi))
      self.lg.info('c :    angle(optical,mechanical) axis   :{0:+12.4f} [arcsec]'.format(c()*180.*3600./np.pi))
      self.lg.info('ep:    tube flexure away from the zenith:{0:+12.4f} [arcsec]'.format(ep()*180.*3600./np.pi))
      self.lg.info('gamma: angle(true,instrumental) pole    :{0:+12.4f} [arcsec]'.format(gamma()*180.*3600./np.pi))
      self.lg.info('theta: hour angle instrumental pole     :{0:+12.4f} [deg]'.format(theta()*180./np.pi))
      self.lg.info('l :    bending of declination axle      :{0:+12.4f} [arcsec]'.format(l()*180.*3600./np.pi))
      self.lg.info('r :    hour angle scale error           :{0:+12.4f} [arcsec]'.format(r()*180.*3600./np.pi))
    self.lg.info('end input parameters')

# hm, first not niceness in argparse
def arg_floats(value):
  return list(map(float, value.split()))

def arg_float(value):
  if 'm' in value:
    return -float(value[1:])
  else:
    return float(value)

if __name__ == "__main__":

  parser= argparse.ArgumentParser(prog=sys.argv[0], description='Create data for an AltAz or EQ pointing model')
  parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
  parser.add_argument('--level', dest='level', default='DEBUG', help=': %(default)s, debug level')
  parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
  parser.add_argument('--break_after', dest='break_after', action='store', default=10000000, type=int, help=': %(default)s, read max. positions, mostly used for debuging')

  parser.add_argument('--gpoint', dest='gpoint', action='store_true', default=False, help=': %(default)s, output format for Petr\'s gpoint, not yet done')
  parser.add_argument('--obs-longitude', dest='obs_lng', action='store', default=123.2994166666666,type=arg_float, help=': %(default)s [deg], observatory longitude + to the East [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-latitude', dest='obs_lat', action='store', default=-75.1,type=arg_float, help=': %(default)s [deg], observatory latitude [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-height', dest='obs_height', action='store', default=3237.,type=arg_float, help=': %(default)s [m], observatory height above sea level [m], negative value: m10. equals to -10.')
  parser.add_argument('--mount-data', dest='mount_data', action='store', default='./mount_data.txt', help=': %(default)s, mount data filename (output)')
  parser.add_argument('--create-eq', dest='create_eq', action='store_true', default=False, help=': %(default)s, True create data for EQ model, else AltAz')
  parser.add_argument('--sigma', dest='sigma', action='store', default=5., type=float,help=': %(default)s, gaussian noise on mount apparent position [arcsec]')
  group = parser.add_mutually_exclusive_group()
  # here: NO  nargs=1!!
  group.add_argument('--eq-params', dest='eq_params', default=[-60.,+12.,-60.,+60.,+30.,+12.,-12.,+60.,-60.], type=arg_floats, help=': %(default)s, EQ list of parameters [arcsec], format "p1 p2...p9"')
  group.add_argument('--aa-params', dest='aa_params', default=[-60.,+120.,-60.,+60.,-60.,+120.,-120.], type=arg_floats, help=': %(default)s, AltAz list of parameters [arcsec], format "p1 p2...p7"')
  parser.add_argument('--t-point', dest='t_point', action='store_true', default=False, help=': %(default)s, fit EQ model with T-point compatible model')
  parser.add_argument('--step', dest='step', action='store', default=10, type=int,help=': %(default)s, AltAz points: range(0,360,step), range(0,90,step) [deg]')
  parser.add_argument('--utc', dest='utc', action='store', default='2016-10-08 05:00:01', type=str,help=': %(default)s, utc observing time, format [iso]')
  parser.add_argument('--pressure-qfe', dest='pressure_qfe', action='store', default=636., type=float,help=': %(default)s, pressure QFE [hPa], not sea level')
  parser.add_argument('--temperature', dest='temperature', action='store', default=-60., type=float,help=': %(default)s, temperature [degC]')
  parser.add_argument('--humidity', dest='humidity', action='store', default=0.5, type=float,help=': %(default)s, humidity [0.,1.]')
  parser.add_argument('--exposure', dest='exposure', action='store', default=5.5, type=float,help=': %(default)s, CCD exposure time')
            
  args=parser.parse_args()
  
  if args.toconsole:
    args.level='DEBUG'

  filename='/tmp/{}.log'.format(sys.argv[0].replace('.py','')) # ToDo datetime, name of the script
  logformat= '%(asctime)s:%(name)s:%(levelname)s:%(message)s'
  logging.basicConfig(filename=filename, level=args.level.upper(), format= logformat)
  logger = logging.getLogger()
    
  if toconsole:
    # http://www.mglerner.com/blog/?p=8
    soh = logging.StreamHandler(sys.stdout)
    soh.setLevel(args.level)
    logger.addHandler(soh)
  if args.gpoint:
    logger.info('u_simulate: not yet ready for prime time, exiting')
    sys.exit(1)
  if args.t_point:
    args.fit_eq=True
    
  sm= SimulationModel(dbg=args.debug,lg=logger,obs_lng=args.obs_lng,obs_lat=args.obs_lat,obs_height=args.obs_height)

  dt_utc = Time(args.utc,format='iso', scale='utc',location=sm.obs)
  sigma= args.sigma/3600./180.*np.pi
  stars=list()
  df=open(args.mount_data, 'w')
  vn_lon=0.
  vn_lat=0.
  if args.create_eq:
    if args.t_point:
      IH=Parameter(args.eq_params[0]/3600./180.*np.pi)
      ID=Parameter(args.eq_params[1]/3600./180.*np.pi)
      CH=Parameter(args.eq_params[2]/3600./180.*np.pi)
      NP=Parameter(args.eq_params[3]/3600./180.*np.pi)
      MA=Parameter(args.eq_params[4]/3600./180.*np.pi)
      ME=Parameter(args.eq_params[5]/3600./180.*np.pi)
      TF=Parameter(args.eq_params[6]/3600./180.*np.pi)
      FO=Parameter(args.eq_params[7]/3600./180.*np.pi)
      DAF=Parameter(args.eq_params[8]/3600./180.*np.pi)
      d_ha_f=d_ha_t_point
      d_dec_f=d_dec_t_point
    else:
      Dd=Parameter(args.eq_params[0]/3600./180.*np.pi)
      Dt=Parameter(args.eq_params[1]/3600./180.*np.pi)
      ip=Parameter(args.eq_params[2]/3600./180.*np.pi)
      c=Parameter(args.eq_params[3]/3600./180.*np.pi)
      ep=Parameter(args.eq_params[4]/3600./180.*np.pi)
      gamma=Parameter(args.eq_params[5]/3600./180.*np.pi)
      theta=Parameter(args.eq_params[6]/3600./180.*np.pi)
      l=Parameter(args.eq_params[7]/3600./180.*np.pi)
      r=Parameter(args.eq_params[8]/3600./180.*np.pi)
      d_ha_f=d_ha
      d_dec_f=d_dec

    sm.print_hadec(t_point=args.t_point)
    for az in range(0,360-args.step,args.step):
      for alt in range(0,90-args.step,args.step):
        r_az=uniform(0, args.step) 
        r_alt=uniform(0, args.step)
        # catalog AltAz
        cat_aa=SkyCoord(az=az+r_az,alt=alt+r_alt,unit=(u.deg,u.deg),frame='altaz',location=sm.obs,obstime=dt_utc)
        # catalog EQ
        cat_eq=sm.LN_AltAz_to_EQ(az=cat_aa.az.deg,alt=cat_aa.alt.deg,obstime=dt_utc)
        # apparent AltAz (mnt), includes refraction etc.
        mnt_aa=sm.transform_to_altaz(eq=cat_eq,tem=args.temperature,pre=args.pressure_qfe,hum=args.humidity,astropy_f=False,correct_cat_f=True)
        # apparent EQ
        mnt_eq_tmp=sm.LN_AltAz_to_EQ(az=mnt_aa.az.degree,alt=mnt_aa.alt.degree,obstime=dt_utc)
        #
        ha= Longitude(dt_utc.sidereal_time('apparent') - mnt_eq_tmp.ra,u.radian).radian
        #
        # subtract the correction
        #               |                                | 0. here
        vd_ha=Longitude(-d_ha_f(ha,mnt_eq_tmp.dec.radian,0.,sm.obs.latitude.radian),u.radian) # apparent coordinates
        vd_dec=Latitude(-d_dec_f(ha,mnt_eq_tmp.dec.radian,0.,sm.obs.latitude.radian),u.radian)
        # noise
        if sigma!=0.:
          vn_lon=Longitude(np.random.normal(loc=0.,scale=sigma),u.radian)
          vn_lat=Latitude(np.random.normal(loc=0.,scale=sigma),u.radian)
        # mount apparent EQ 
        try:
          mnt_eq=SkyCoord(ra=mnt_eq_tmp.ra+vd_ha+vn_lon,dec=mnt_eq_tmp.dec+vd_dec+vn_lat,unit=(u.radian,u.radian),frame='cirs',location=sm.obs,obstime=dt_utc)
        except Exception as e:
          logger.warn('u_simulate: exception {}, lost one data point'.format(e))
          continue
        if args.gpoint:
          pass
        else:
          df.write('{},{},{},{},{},{},{},{},{}\n'.format(dt_utc, cat_eq.ra.radian,cat_eq.dec.radian,mnt_eq.ra.radian,mnt_eq.dec.radian,args.exposure,args.temperature,args.pressure_qfe,args.humidity))
  else:
    # 
    C1=Parameter(args.aa_params[0]/3600./180.*np.pi)
    C2=Parameter(args.aa_params[1]/3600./180.*np.pi)
    C3=Parameter(args.aa_params[2]/3600./180.*np.pi)
    C4=Parameter(args.aa_params[3]/3600./180.*np.pi)
    C5=Parameter(args.aa_params[4]/3600./180.*np.pi)
    C6=Parameter(args.aa_params[5]/3600./180.*np.pi)
    C7=Parameter(args.aa_params[6]/3600./180.*np.pi)
    sm.print_altaz()
    
    for az in range(0,360-args.step,args.step):
      for alt in range(0,90-args.step,args.step):
        r_az=uniform(0, args.step) 
        r_alt=uniform(0, args.step)
        # catalog AltAz
        cat_aa=SkyCoord(az=az+r_az,alt=alt+r_alt,unit=(u.deg,u.deg),frame='altaz',location=sm.obs,obstime=dt_utc)
        # catalog EQ
        cat_eq=sm.LN_AltAz_to_EQ(az=cat_aa.az.deg,alt=cat_aa.alt.deg,obstime=dt_utc)
        # apparent AltAz (mnt), includes refraction etc.
        mnt_aa_tmp=sm.transform_to_altaz(eq=cat_eq,tem=args.temperature,pre=args.pressure_qfe,hum=args.humidity,astropy_f=False,correct_cat_f=True)
        # subtract the correction
        #               |
        #               |                                                | 0. here
        vd_az=Longitude(-d_az(mnt_aa_tmp.az.radian,mnt_aa_tmp.alt.radian,0.),u.radian)
        vd_alt=Latitude(-d_alt(mnt_aa_tmp.az.radian,mnt_aa_tmp.alt.radian,0.),u.radian)
        # noise
        if sigma!=0.:
          vn_lon=Longitude(np.random.normal(loc=0.,scale=sigma),u.radian)
          vn_lat=Latitude(np.random.normal(loc=0.,scale=sigma),u.radian)
        # mount apparent AltAz
        mnt_aa=SkyCoord(az=mnt_aa_tmp.az+vd_az+vn_lon,alt=mnt_aa_tmp.alt+vd_alt+vn_lat,unit=(u.radian,u.radian),frame='altaz',location=sm.obs,obstime=dt_utc)
        # mount apparent EQ
        mnt_eq=sm.LN_AltAz_to_EQ(az=mnt_aa.az.degree,alt=mnt_aa.alt.degree,obstime=dt_utc)
        if args.gpoint:
          pass
        else:
          df.write('{},{},{},{},{},{},{},{},{}\n'.format(dt_utc, cat_eq.ra.radian,cat_eq.dec.radian,mnt_eq.ra.radian,mnt_eq.dec.radian,args.exposure,args.pressure_qfe,args.temperature,args.humidity))

  df.close()

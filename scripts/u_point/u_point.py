#!/usr/bin/env python3
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
Calculate the parameters for pointing model J.Condon (1992)

AltAz: Astropy N=0,E=pi/2, Libnova S=0,W=pi/1

'''

__author__ = 'wildi.markus@bluewin.ch'

import sys
import argparse
import logging
import os

import numpy as np
import scipy.optimize
import pandas as pd

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
# ln_get_angular_separation (struct ln_equ_posn *posn1, struct ln_equ_posn *posn2)
ln.ln_get_angular_separation.restype = c_double

#ln_equ_posn1=LN_equ_posn()
#ln_equ_posn2=LN_equ_posn()
#def LN_ln_get_angular_separation(ra1=None,dec1=None,ra2=None,dec2=None):
#  ln_equ_posn1.ra=ra1
#  ln_equ_posn1.dec=dec1
#  ln_equ_posn2.ra=ra2
#  ln_equ_posn2.dec=dec2
#  sep=ln.ln_get_angular_separation( byref(ln_equ_posn1), byref(ln_equ_posn2))
#  return sep


ln_pos_eq=LN_equ_posn()
ln_pos_eq_ab=LN_equ_posn()
ln_pos_eq_pm=LN_equ_posn()
ln_pos_eq_app=LN_equ_posn()
ln_pos_eq_pr=LN_equ_posn()
ln_pos_aa_pr=LN_hrz_posn()
ln_hrz_posn=LN_hrz_posn()
# http://scipy-cookbook.readthedocs.io/items/FittingData.html#simplifying-the-syntax
# This is the real big relief

class Parameter:
  def __init__(self, value):
    self.value = value
  def set(self, value):
    self.value = value
  def __call__(self):
    return self.value

# data structure
class Point(object):
  def __init__(self,cat_lon=None,cat_lat=None,mnt_lon=None,mnt_lat=None,df_lat=None,df_lon=None,res_lat=None,res_lon=None):
    self.cat_lon=cat_lon
    self.cat_lat=cat_lat
    self.mnt_lon=mnt_lon
    self.mnt_lat=mnt_lat
    self.df_lat=df_lat
    self.df_lon=df_lon
    self.res_lat=res_lat
    self.res_lon=res_lon

# fit projections with a gaussian
def fit_projection_helper(function, parameters, y, x = None):
  def local_f(params):
    for i,p in enumerate(parameters):
      p.set(params[i])
    return y - function(x)
            
  p = [param() for param in parameters]
  return scipy.optimize.leastsq(local_f, p)

def fit_projection(x):
  return height() * np.exp(-(x-mu())**2/2./sigma()**2) #+ background()
# end fit the projections

# J.Condon 1992
# Hamburg:
# PUBLICATIONS OF THE ASTRONOMICAL SOCIETY OF THE PACIFIC, 120:425–429, 2008 April
# The Temperature Dependence of the Pointing Model of the Hamburg Robotic Telescope
#
def d_az(azs,alts,d_azs):
  #    C1   C2                C3                C4                            C5             
  #                                            NO!! (see C4 below): yes, minus! see paper Hamburg
  #                                            |
  val=(C1()+C2()*np.cos(alts)+C3()*np.sin(alts)+C4()*np.cos(azs)*np.sin(alts)+C5()*np.sin(azs)*np.sin(alts))/np.cos(alts) 
  return val-d_azs

def d_alt_plus_poly(azs,alts,d_alts):
  return A0() + A1() * alts + A2() * alts**2 + A3() * alts**3

def d_alt(azs,alts,d_alts,fit_plus_poly):
  val_plus_poly=0.
  # this the way to expand the pointing model
  if fit_plus_poly:
    val_plus_poly=d_alt_plus_poly(azs,alts,d_alts)
  # Condon 1992, see page 6, Eq. 5: D_N cos(A) - D_W sin(A)
  #              d_alt equation is wrong on p.7
  #   C6   C7                C4               C5          
  #                         minus sign here
  #                         |
  val=C6()+C7()*np.cos(alts)-C4()*np.sin(azs)+C5()*np.cos(azs)+val_plus_poly 
  return val-d_alts
  
def fit_altaz(azs_cat=None,alts_cat=None,d_azs=None, d_alts=None,fit_plus_poly=False):
  faz=d_az(azs_cat,alts_cat,d_azs)
  falt=d_alt(azs_cat,alts_cat,d_alts,fit_plus_poly) 
  # this is the casus cnactus
  # http://stackoverflow.com/questions/23532068/fitting-multiple-data-sets-using-scipy-optimize-with-the-same-parameters
  return np.concatenate((faz,falt))

def fit_altaz_helper(function, parameters, azs_cat=None,alts_cat=None,d_azs=None, d_alts=None,fit_plus_poly=False):
  def local_f(params):
    for i,p in enumerate(parameters):
      p.set(params[i])

    return function(azs_cat=azs_cat,alts_cat=alts_cat,d_azs=d_azs, d_alts=d_alts,fit_plus_poly=fit_plus_poly)
            
  p = [param() for param in parameters]
  return scipy.optimize.leastsq(local_f, p)#,full_output=True)

# T-point
#
# IH ha index error
# ID delta index error
# CH collimation error CH * sec(delta)
# NP ha/delta non-perpendicularity NP*tan(delta)
# MA polar axis left-right misalignment ha: -MA* cos(ha)*tan(delta), dec: MA*sin(ha)
# ME polar axis verticsl misalignment: ha: ME*sin(ha)*tan(delta), dec: ME*cos(ha)
# TF tube flexure, ha: TF*cos(phi)*sin(ha)*sec(delta), dec: TF*(cos(phi)*cos(ha)*sind(delta)-sin(phi)*cos(delta)
# FO fork flexure, dec: FO*cos(ha)
# DAF dec axis flexure, ha: -DAF*(cos(phi)*cos(ha) + sin(phi)*tan(dec)
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
      +FO()*np.cos(has_cat) # see ME()
  return val-d_decs

def fit_hadec_t_point(has_cat=None,decs_cat=None,d_has=None,d_decs=None,phi=None):
  fha=d_ha_t_point(has_cat,decs_cat,d_has,phi)
  fdec=d_dec_t_point(has_cat,decs_cat,d_decs,phi) 
  return np.concatenate((fha,fdec))
  #return libnova.angular_separation(ra1=has_cat+fha,dec1=decs_cat+fdec,ra2=has_cat,dec2=decs_cat)


# Marc W. Buie 2003
global d_ha
def d_ha(has_cat,decs_cat,d_has,phi):
  # ToDo check that:
  #val= Dt()-gamma()*np.sin(has_cat-theta())*np.tan(decs_cat)+c()/np.cos(decs_cat)-ip()*np.tan(decs_cat) +\
  #e()*np.cos(phi)*1./np.cos(decs_cat)*np.sin(has_cat-theta())+l()*(np.sin(phi) *np.tan(decs_cat) + np.cos(decs_cat)* np.cos(has_cat-theta())) + r()*(has_cat-theta())
  val= Dt()\
       -gamma()*np.sin(has_cat-theta())*np.tan(decs_cat)\
       +c()/np.cos(decs_cat)\
       -ip()*np.tan(decs_cat)\
       +e()*np.cos(phi)/np.cos(decs_cat)*np.sin(has_cat)\
       +l()*(np.sin(phi)*np.tan(decs_cat) + np.cos(decs_cat)* np.cos(has_cat))\
       + r()*has_cat
  return val-d_has  #d_has= τ-t. (τ-t)-val (val: right side) 

global d_dec
def d_dec(has_cat,decs_cat,d_decs,phi):
  # ToDo check that:
  #val=Dd()-gamma()*np.cos(has_cat-theta())-e()*(np.sin(phi)*np.cos(decs_cat)-np.cos(phi)*np.sin(decs_cat)*np.cos(has_cat-theta()))
  val=Dd()\
      -gamma()*np.cos(has_cat-theta())\
      -e()*(np.sin(phi)*np.cos(decs_cat)-np.cos(phi)*np.sin(decs_cat)*np.cos(has_cat))
  return val-d_decs


def fit_hadec(has_cat=None,decs_cat=None,d_has=None, d_decs=None,phi=None):
  fha=d_ha(has_cat,decs_cat,d_has,phi) 
  fdec=d_dec(has_cat,decs_cat,d_decs,phi)
  return np.concatenate((fha,fdec))
  #return libnova.angular_separation(ra1=has_cat+fha,dec1=decs_cat+fdec,ra2=has_cat,dec2=decs_cat)

def fit_hadec_helper(function=None, parameters=None, has_cat=None,decs_cat=None,d_has=None, d_decs=None,phi=None):
  def local_f(params):
    for i,p in enumerate(parameters):
      p.set(params[i])

    return function(has_cat=has_cat,decs_cat=decs_cat,d_has=d_has, d_decs=d_decs,phi=phi)

  p = [param() for param in parameters]
  return scipy.optimize.least_squares(local_f, p,method='dogbox',ftol=1.e-8,xtol=1.e-8,max_nfev=10000)


class PointingModel(object):
  def __init__(self, dbg=None,lg=None, base_path=None, obs_lng=None, obs_lat=None, obs_height=None):
    self.dbg=dbg
    self.lg=lg
    self.base_path=base_path
    #
    self.obs=EarthLocation(lon=float(obs_lng)*u.degree, lat=float(obs_lat)*u.degree, height=float(obs_height)*u.m)
    #
    self.ln_obs=LN_lnlat_posn()
    self.ln_obs.lng=obs_lng     # deg
    self.ln_obs.lat=obs_lat     # deg
    self.ln_hght=obs_height     # m, not a libnova quantity

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

  def LN_AltAz_to_HA(self,az=None,alt=None,obstime=None):
    ln_hrz_posn.alt=alt
    ln_hrz_posn.az=az
    ln.ln_get_equ_from_hrz(byref(ln_hrz_posn),byref(self.ln_obs), c_double(obstime.jd),byref(ln_pos_eq))
    # calculate HA
    ra=Longitude(ln_pos_eq.ra,u.deg)
    ha= obstime.sidereal_time('apparent') - ra
    # hm, ra=ha a bit ugly
    pos_ha=SkyCoord(ra=ha, dec=Latitude(ln_pos_eq.dec,u.deg).radian,unit=(u.radian,u.radian),frame='cirs')
    return pos_ha
  
  def transform_to_hadec(self,eq=None,tem=None,pre=None,hum=None,astropy_f=False,correct_cat_f=None):
    pre_qfe=0.
    if correct_cat_f:
      pre_qfe=pre # to make it clear what is used

    if astropy_f:
      pos_aa_ap=pos_aa=eq.transform_to(AltAz(obswl=0.5*u.micron, pressure=pre_qfe*u.hPa,temperature=tem*u.deg_C,relative_humidity=hum))
      # ToDo: check that if correct!
      # debug:
      pos_eqc=pos_aa.transform_to(CIRS())
      ha= eq.obstime.sidereal_time('apparent') - pos_eqc.ra
      pos_ha=pos_ha_ap=SkyCoord(ra=ha,dec=pos_eqc.dec, unit=(u.rad,u.rad), frame='cirs',obstime=eq.obstime,location=self.obs)
      # print('LN: {0:.3f}, {1:.3f}, {2:.3f}, {3:.3f}'.format(pos_ha_ln.ra.degree,pos_ha_ln.dec.degree,pos_aa_ln.az.degree,pos_aa_ln.alt.degree))
      # print('AP: {0:.3f}, {1:.3f}, {2:.3f}, {3:.3f}'.format(pos_ha_ap.ra.degree,pos_ha_ap.dec.degree,pos_aa_ap.az.degree,pos_aa_ap.alt.degree))
    else:
      # debug:
      pos_aa_ln=pos_aa=self.LN_EQ_to_AltAz(ra=Longitude(eq.ra.radian,u.radian).degree,dec=Latitude(eq.dec.radian,u.radian).degree,ln_pressure_qfe=pre_qfe,ln_temperature=tem,ln_humidity=hum,obstime=eq.obstime,correct_cat=correct_cat_f)
      pos_ha=pos_ha_ln=self.LN_AltAz_to_HA(az=pos_aa.az.degree,alt=pos_aa.alt.degree,obstime=eq.obstime)
      
    return pos_ha
  
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

  def fetch_pandas(self, ptfn=None,columns=None):
    pd_cat=None
    if not os.path.isfile(ptfn):
      self.lg.debug('fetch_pandas: {} does not exist, exiting'.format(ptfn))
      sys.exit(1)
    try:
      pd_cat = pd.read_csv(ptfn, sep=',',header=None)
    except ValueError as e:
      self.lg.debug('fetch_pandas: {},{}'.format(ptfn, e))
      return
    except OSError as e:
      self.lg.debug('fetch_pandas: {},{}'.format(ptfn, e))
      return
    except Exception as e:
      self.lg.debug('fetch_pandas: {},{}, exiting'.format(ptfn, e))
      sys.exit(1)
    
    try:
      pd_cat.columns = columns
    except ValueError as e:
      self.lg.debug('fetch_pandas: {},{}, ignoring'.format(ptfn, e))
      return

    return pd_cat

  def fetch_coordinates(self,ptfn=None,astropy_f=False,break_after=None,fit_eq=False):
    '''
    Data structure, one per line:
    date time UTC begin exposure [format iso], catalog RA [rad], catalog DEC [rad], mount position RA [rad], mount position DEC [rad], [exposure time [sec], temperature [deg C], pressure QFE [hPa], humidity [%]]
    
    Mount position coordinates are the apparent telescope coordinates as read at the setting circles.

    Exposure, temperature, pressure and humidity are optional.

    This method uses the optional items in case they are present.
    '''
    cats=list()
    mnts=list()
    if self.base_path in ptfn:
      fn=ptfn
    else:
      fn=os.path.join(self.base_path,ptfn)

    pd_cat = self.fetch_pandas(ptfn=fn,columns=['utc','cat_ra','cat_dc','mnt_ra','mnt_dc','exp'])
    if pd_cat is None:
      pd_cat = self.fetch_pandas(ptfn=fn,columns=['utc','cat_ra','cat_dc','mnt_ra','mnt_dc','exp','tem','pre','hum'])
      
    if pd_cat is None:
      return

    for i,rw in pd_cat.iterrows():
      if i > break_after:
        break
      dt_utc = Time(rw['utc'],format='iso', scale='utc',location=self.obs)
      if pd.notnull(rw['exp']):
        dt_utc=dt_utc - TimeDelta(rw['exp']/2.,format='sec') # exp. time is small

      cat_eq=SkyCoord(ra=rw['cat_ra'],dec=rw['cat_dc'], unit=(u.rad,u.rad), frame='icrs',obstime=dt_utc,location=self.obs)
      if pd.notnull(rw['mnt_ra']) and pd.notnull(rw['mnt_dc']):
        mnt_eq=SkyCoord(ra=rw['mnt_ra'],dec=rw['mnt_dc'], unit=(u.rad,u.rad), frame='icrs',obstime=dt_utc,location=self.obs)
      else:
        self.lg.warn('fetch_coordinates: no analyzed position on line: {},{}'.format(i,rw))
        continue
      
      #if pd.isnull(rw['pre']):
      tem=pre=hum=0.
      #else:
      #  tem=rw['tem']
      #  pre=rw['pre']
      #  hum=rw['hum']
        
      if fit_eq:
        cat_ha=self.transform_to_hadec(eq=cat_eq,tem=tem,pre=pre,hum=hum,astropy_f=astropy_f,correct_cat_f=True)
        mnt_ha=self.transform_to_hadec(eq=mnt_eq,tem=tem,pre=pre,hum=hum,astropy_f=astropy_f,correct_cat_f=False)
        #if self.accept(cat_lon=cat_ha.ra.radian, cat_lat=cat_ha.dec.radian, mnt_lon=mnt_ha.ra.radian, mnt_lat=mnt_ha.dec.radian):
        cats.append(cat_ha)
        mnts.append(mnt_ha)
      else:
        cat_aa=self.transform_to_altaz(eq=cat_eq,tem=tem,pre=pre,hum=hum,astropy_f=astropy_f,correct_cat_f=True)
        mnt_aa=self.transform_to_altaz(eq=mnt_eq,tem=tem,pre=pre,hum=hum,astropy_f=astropy_f,correct_cat_f=False)
        #if self.accept(cat_lon=cat_aa.az.radian, cat_lat=cat_aa.alt.radian, mnt_lon=mnt_aa.az.radian, mnt_lat=mnt_aa.alt.radian):
        cats.append(cat_aa)
        mnts.append(mnt_aa)
    return cats,mnts

  def accept(self, cat_lon=None, cat_lat=None, mnt_lon=None, mnt_lat=None):
    # ToDo: may be a not so good idea
    acc= False
    if -2./60./180.*np.pi < (cat_lon-mnt_lon) < 2./60./180.*np.pi:
      acc=True
    else:
      self.lg.debug('reject lon: {0:.4f}'.format((cat_lon-mnt_lon) * 180. * 60. /np.pi))

    if -10./60./180.*np.pi<(cat_lat-mnt_lat) < 10./60./180.*np.pi:
      acc=True
    else:
      self.lg.debug('reject lat: {0:.4f}'.format((cat_lat-mnt_lat) * 180. * 60. /np.pi))

    return acc

  
  def fit_projection_and_plot(self,vals=None, bins=None, axis=None,fit_title=None,fn_frac=None,prefix=None,plt_no=None,plt=None):
    '''

    Fit the projections of differences (not corrected data) as well as the residues (lat,lon coordinates).

    To compare differences ('1') and residues ('2') the plot titles are labled with a letter and '1' and '2'. 

    '''
    fig = plt.figure()
    ax = fig.add_subplot(111)
    cnt_bins, rbins, patches = ax.hist(vals,bins,normed=True, facecolor='green', alpha=0.75)
    bins=rbins[:-1] #ToDO why?

    # estimator, width
    wd=np.sqrt(np.abs(np.sum((bins-cnt_bins)**2*cnt_bins)/np.sum(cnt_bins)))

    global mu,sigma,height,background
    mu=Parameter(0.)
    sigma=Parameter(wd) #arcsec!
    height=Parameter(cnt_bins.max())
    background=Parameter(0.)
    parameters=[mu,sigma,height]#background
    res,stat=fit_projection_helper(fit_projection,[mu,sigma,height], cnt_bins, bins)
    if stat != 1:
      self.lg.warn('fit projection not converged, status: {}'.format(stat))
      
    y=fit_projection(bins)
    l = ax.plot(bins, y, 'r--', linewidth=2)
    ax.set_xlabel('{} {} [arcsec]'.format(prefix,axis))
    ax.set_ylabel('number of events normalized')
    if prefix in 'difference': # ToDo ugly
      ax.set_title('{0} {1} {2} $\mu$={3:.2f},$\sigma$={4:.2f} [arcsec] \n catalog_not_corrected - star'.format(plt_no,prefix,axis,mu(), sigma()))
      fig.savefig(os.path.join(self.base_path,'{}_catalog_not_corrected_projection_{}.png'.format(prefix,axis)))
    else:
      ax.set_title(r'{0} {1} {2} $\mu$={3:.2f},$\sigma$={4:.2f} [arcsec], fit: {5}'.format(plt_no,prefix,axis,mu(), sigma(),fit_title))
      fig.savefig(os.path.join(self.base_path,'{}_projection_{}_{}.png'.format(prefix,axis,fn_frac)))


  # Condon 1992, fit dAlt and dAz data simultaneously
  def fit_model_altaz(self,cat_aas=None,mnt_aas=None,fit_plus_poly=None):
    # ToDo do not copy yourself
    azs_cat=np.zeros(shape=(len(cat_aas)))
    alts_cat=np.zeros(shape=(len(cat_aas)))
    d_azs=np.zeros(shape=(len(cat_aas)))
    d_alts=np.zeros(shape=(len(cat_aas)))
    # make it suitable for fitting
    for i,ct in enumerate(cat_aas):
      # x: catlog
      azs_cat[i]=ct.az.radian
      alts_cat[i]=ct.alt.radian
      #
      # y:  catalog_apparent-star
      d_azs[i]=ct.az.radian-mnt_aas[i].az.radian
      d_alts[i]=ct.alt.radian-mnt_aas[i].alt.radian
    # not nice but useful
    global C1,C2,C3,C4,C5,C6,C7,A0,A1,A2,A3 
    C1=Parameter(0.)
    C2=Parameter(0.)
    C3=Parameter(0.)
    C4=Parameter(0.)
    C5=Parameter(0.)
    #C6=Parameter(627.47853/3600./180.8 * np.pi)
    #C7=Parameter(-244.3/3600./180.8 * np.pi)
    C6=Parameter(0.)
    C7=Parameter(0.)
    A0=Parameter(0.)
    A1=Parameter(0.)
    A2=Parameter(0.)
    A3=Parameter(0.)
    pars=[C1,C2,C3,C4,C5,C6,C7,A0,A1,A2,A3]

    res,stat=fit_altaz_helper(fit_altaz,pars,azs_cat=azs_cat,alts_cat=alts_cat,d_azs=d_azs,d_alts=d_alts,fit_plus_poly=fit_plus_poly)
    self.lg.info('--------------------------------------------------------')
    if stat != 1:
      if stat==5:
        self.lg.warn('fit not converged, status: {}'.format(stat))
      else:
        self.lg.warn('fit converged with status: {}'.format(stat))
    # output used in telescope driver
    if self.dbg:
      for i,c in enumerate(pars):
        if i == 7 and not fit_plus_poly:
          break
      self.lg.info('C{0:02d}={1};'.format(i+1,res[i]))

    self.lg.info('fitted values:')
    self.lg.info('C1: horizontal telescope collimation:{0:+10.4f} [arcsec]'.format(C1()*180.*3600./np.pi))
    self.lg.info('C2: constant azimuth offset         :{0:+10.4f} [arcsec]'.format(C2()*180.*3600./np.pi))
    self.lg.info('C3: tipping-mount collimation       :{0:+10.4f} [arcsec]'.format(C3()*180.*3600./np.pi))
    self.lg.info('C4: azimuth axis tilt West          :{0:+10.4f} [arcsec]'.format(C4()*180.*3600./np.pi))
    self.lg.info('C5: azimuth axis tilt North         :{0:+10.4f} [arcsec]'.format(C5()*180.*3600./np.pi))
    self.lg.info('C6: vertical telescope collimation  :{0:+10.4f} [arcsec]'.format(C6()*180.*3600./np.pi))
    self.lg.info('C7: gravitational tube bending      :{0:+10.4f} [arcsec]'.format(C7()*180.*3600./np.pi))
    if fit_plus_poly:
      self.lg.info('A0: a0 (polynom)                    :{0:+10.4f}'.format(A0()))
      self.lg.info('A1: a1 (polynom)                    :{0:+10.4f}'.format(A1()))
      self.lg.info('A2: a2 (polynom)                    :{0:+10.4f}'.format(A2()))
      self.lg.info('A3: a3 (polynom)                    :{0:+10.4f}'.format(A3()))
      
    return res
  
  # Marc W. Buie 2003
  def fit_model_hadec(self,cat_has=None,mnt_has=None,t_point=False):

    has_cat=np.zeros(shape=(len(cat_has)))
    decs_cat=np.zeros(shape=(len(cat_has)))
    d_has=np.zeros(shape=(len(cat_has)))
    d_decs=np.zeros(shape=(len(cat_has)))
    # make it suitable for fitting
    for i,ct in enumerate(cat_has):
      # x: catlog
      has_cat[i]= cat_has[i].ra.radian #! it is ha
      decs_cat[i]=cat_has[i].dec.radian
      #
      # y:  catalog_apparent-star
      d_has[i]=cat_has[i].ra.radian-mnt_has[i].ra.radian
      d_decs[i]=cat_has[i].dec.radian-mnt_has[i].dec.radian

    if t_point:
      global IH,ID,CH,NP,MA,ME,TF,FO,DAF
      IH=Parameter(0.)
      ID=Parameter(0.)
      CH=Parameter(0.)
      NP=Parameter(0.)
      MA=Parameter(0.)
      ME=Parameter(0.)
      TF=Parameter(0.)
      FO=Parameter(0.)
      DAF=Parameter(0.)
      pars=[IH,ID,CH,NP,MA,ME,TF,FO,DAF]
      #pars=[MA,ME]

      res=fit_hadec_helper(
        fit_hadec_t_point,
        pars,
        has_cat=has_cat,
        decs_cat=decs_cat,
        d_has=d_has,
        d_decs=d_decs,
        phi=self.obs.latitude.radian)
      print(res)
      print(type(res))
      res=stat=1

      self.lg.info('-------------------------------------------------------------')
      if stat != 1:
        if stat==5:
          self.lg.warn('fit not converged, status: {}'.format(stat))
        else:
          self.lg.warn('fit converged with status: {}'.format(stat))

      self.lg.info('fitted values:')
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
      global Dd,gamma,theta,e,Dt,c,ip,l,r
      Dd=Parameter(0.)
      gamma=Parameter(0.)
      theta=Parameter(0.)
      e=Parameter(0.)
      Dt=Parameter(0.)
      c=Parameter(0.)
      ip=Parameter(0.)
      l=Parameter(0.)
      r=Parameter(0.)
      pars=[Dd,gamma,theta,e,Dt,c,ip,l,r]
      # all other constant:
      pars=[gamma]
      res=fit_hadec_helper(fit_hadec,pars,has_cat=has_cat,decs_cat=decs_cat,d_has=d_has,d_decs=d_decs,phi=self.obs.latitude.radian)
      # ToDo
      print(res)
      print(type(res))
      res=stat=1
      self.lg.info('--------------------------------------------------------------')
      if stat != 1:
        if stat==5:
          self.lg.warn('fit not converged, status: {}'.format(stat))
        else:
          self.lg.warn('fit converged with status: {}'.format(stat))
      self.lg.info('fitted values:')
      self.lg.info('Dd:    declination zero-point offset    :{0:+12.4f} [arcsec]'.format(Dd()*180.*3600./np.pi))
      self.lg.info('Dt:    hour angle zero-point offset     :{0:+12.4f} [arcsec]'.format(Dt()*180.*3600./np.pi))
      self.lg.info('ip:    angle(polar,declination) axis    :{0:+12.4f} [arcsec]'.format(ip()*180.*3600./np.pi))
      self.lg.info('c :    angle(optical,mechanical) axis   :{0:+12.4f} [arcsec]'.format(c()*180.*3600./np.pi))
      self.lg.info('e :    tube flexure away from the zenith:{0:+12.4f} [arcsec]'.format(e()*180.*3600./np.pi))
      self.lg.info('gamma: angle(true,instrumental) pole    :{0:+12.4f} [arcsec]'.format(gamma()*180.*3600./np.pi))
      self.lg.info('theta: hour angle instrumental pole     :{0:+12.4f} [deg]'.format(theta()*180./np.pi))
      self.lg.info('l :    bending of declination axle      :{0:+12.4f} [arcsec]'.format(l()*180.*3600./np.pi))
      self.lg.info('r :    hour angle scale error           :{0:+12.4f} [arcsec]'.format(r()*180.*3600./np.pi))

  def plot_results(self, stars=None,args=None):
    '''
    Plot the differences and residues as a function of lon,lat or dlon,dlat or a combination of them.
    '''
    import matplotlib
    # this varies from distro to distro:
    matplotlib.rcParams["backend"] = "TkAgg"
    import matplotlib.pyplot as plt
    plt.ioff()
    if args.fit_eq:
      fit_title='buie2003, LN'
      fn_frac='buie2003LN'
      if args.astropy:
        fit_title='buie2003, AP'
        fn_frac='buie2003AP'
        
      lon='hour angle'
      lat='declination'
    else:
      fit_title='condon1992, LN'
      fn_frac='condon1992LN'
      if args.astropy:
        fit_title='condon1992, AP'
        fn_frac='condon1992AP'
      
      lon='azimuth'
      lat='altitude'

    if args.fit_plus_poly:
      fit_title='C+PP'
      fn_frac='c_plus_poly'

    az_cat_deg=[x.cat_lon.degree for x in stars]
    alt_cat_deg=[x.cat_lat.degree for x in stars]

    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_title('A1 difference: catalog_not_corrected - star')
    ax.set_xlabel('d({}) [arcmin]'.format(lon))
    ax.set_ylabel('d({}) [arcmin]'.format(lat))
    ax.scatter([x.df_lon.arcmin for x in stars],[x.df_lat.arcmin for x in stars])
    fig.savefig(os.path.join(self.base_path,'difference_catalog_not_corrected_star.png'))

    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_title('B1 difference {}: catalog_not_corrected - star'.format(lon))
    ax.set_xlabel('{} [deg]'.format(lon))
    ax.set_ylabel('d({}) [arcmin]'.format(lon))
    ax.scatter(az_cat_deg ,[x.df_lon.arcmin for x in stars])
    fig.savefig(os.path.join(self.base_path,'difference_{0}_d{0}_catalog_not_corrected_star.png'.format(lon)))

    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_title('C1 difference {}: catalog_not_corrected - star'.format(lat))
    ax.set_xlabel('{} [deg]'.format(lon))
    ax.set_ylabel('d({}) [arcmin]'.format(lat))
    ax.scatter(az_cat_deg ,[x.df_lat.arcmin for x in stars])
    fig.savefig(os.path.join(self.base_path,'difference_{0}_d{1}_catalog_not_corrected_star.png'.format(lon,lat)))

    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_title('D1 difference {}: catalog_not_corrected - star'.format(lon))
    ax.set_xlabel('{} [deg]'.format(lat))
    ax.set_ylabel('d({}) [arcmin]'.format(lon))
    ax.scatter(alt_cat_deg ,[x.df_lon.arcmin for x in stars])
    # ToDo: think about that:
    #ax.scatter(alt_cat_deg ,[x.df_lon.arcmin/ np.tan(x.mnt_lat.radian) for x in stars])
    fig.savefig(os.path.join(self.base_path,'difference_{0}_d{1}_catalog_not_corrected_star.png'.format(lat,lon)))

    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_title('E1 difference {}: catalog_not_corrected - star'.format(lat))
    ax.set_xlabel('{} [deg]'.format(lat))
    ax.set_ylabel('d({}) [arcmin]'.format(lat))
    ax.scatter(alt_cat_deg ,[x.df_lat.arcmin for x in stars])
    fig.savefig(os.path.join(self.base_path,'difference_{0}_d{0}_catalog_not_corrected_star.png'.format(lat)))

    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_title('A2 residuum: catalog_corrected - star {}'.format(fit_title))
    ax.set_xlabel('d({}) [arcmin]'.format(lon))
    ax.set_ylabel('d({}) [arcmin]'.format(lat))
    ax.scatter([x.res_lon.arcmin for x in stars],[x.res_lat.arcmin for x in stars])
    fig.savefig(os.path.join(self.base_path,'residuum_catalog_corrected_star_{}.png'.format(fn_frac)))

    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_title('B2 residuum {} catalog_corrected - star, fit: {}'.format(lon,fit_title))
    ax.set_xlabel('{} [deg]'.format(lon))
    ax.set_ylabel('d({}) [arcmin]'.format(lon))
    ax.scatter(az_cat_deg,[x.res_lon.arcmin for x in stars])
    fig.savefig(os.path.join(self.base_path,'residuum_{0}_d{0}_catalog_corrected_star_{1}.png'.format(lon,fn_frac)))

    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_title('D2 residuum {} catalog_corrected - star, fit: {}'.format(lon,fit_title))
    ax.set_xlabel('{} [deg]'.format(lat))
    ax.set_ylabel('d({}) [arcmin]'.format(lon))
    ax.scatter(alt_cat_deg,[x.res_lon.arcmin for x in stars])
    fig.savefig(os.path.join(self.base_path,'residuum_{0}_d{1}_catalog_corrected_star_{2}.png'.format(lat,lon,fn_frac)))

    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_title('C2 residuum {} catalog_corrected - star, fit: {}'.format(lat,fit_title))
    ax.set_xlabel('{} [deg]'.format(lon))
    ax.set_ylabel('d({}) [arcmin]'.format(lat))
    ax.scatter(az_cat_deg,[x.res_lat.arcmin for x in stars])
    fig.savefig(os.path.join(self.base_path,'residuum_{0}_d{1}_catalog_corrected_star_{2}.png'.format(lon,lat,fn_frac)))
          
    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_title('E2 residuum {} catalog_corrected - star, fit: {}'.format(lat,fit_title))
    ax.set_xlabel('{}  [deg]'.format(lat))
    ax.set_ylabel('d({}) [arcmin]'.format(lat))
    ax.scatter(alt_cat_deg,[x.res_lat.arcmin for x in stars])
    fig.savefig(os.path.join(self.base_path,'residuum_{0}_d{0}_catalog_corrected_star_{1}.png'.format(lat,fn_frac)))

    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_title('K measurement locations catalog')
    ax.set_xlabel('{} [deg]'.format(lon))
    ax.set_ylabel('{} [deg]'.format(lat))
    ax.scatter([x.cat_lon.degree for x in stars],[x.cat_lat.degree for x in stars])
    fig.savefig(os.path.join(self.base_path,'measurement_locations_catalog.png'))


    self.fit_projection_and_plot(vals=[x.df_lon.arcsec for x in stars], bins=args.bins,axis='{}'.format(lon), fit_title=fit_title,fn_frac=fn_frac,prefix='difference',plt_no='P1',plt=plt)
    self.fit_projection_and_plot(vals=[x.df_lat.arcsec for x in stars], bins=args.bins,axis='{}'.format(lat),fit_title=fit_title,fn_frac=fn_frac,prefix='difference',plt_no='P2',plt=plt)
    self.fit_projection_and_plot(vals=[x.res_lon.arcsec for x in stars],bins=args.bins,axis='{}'.format(lon), fit_title=fit_title,fn_frac=fn_frac,prefix='residuum',plt_no='Q1',plt=plt)
    self.fit_projection_and_plot(vals=[x.res_lat.arcsec for x in stars],bins=args.bins,axis='{}'.format(lat),fit_title=fit_title,fn_frac=fn_frac,prefix='residuum',plt_no='Q2',plt=plt)

    plt.show()
# really ugly!
def arg_floats(value):
  return list(map(float, value.split()))

def arg_float(value):
  if 'm' in value:
    return -float(value[1:])
  else:
    return float(value)
    
if __name__ == "__main__":

  parser= argparse.ArgumentParser(prog=sys.argv[0], description='Fit an AltAz or EQ pointing model')
  parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
  parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
  parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
  parser.add_argument('--break_after', dest='break_after', action='store', default=10000000, type=int, help=': %(default)s, read max. positions, mostly used for debuging')

  parser.add_argument('--obs-longitude', dest='obs_lng', action='store', default=123.2994166666666,type=arg_float, help=': %(default)s [deg], observatory longitude + to the East [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-latitude', dest='obs_lat', action='store', default=-75.1,type=arg_float, help=': %(default)s [deg], observatory latitude [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-height', dest='obs_height', action='store', default=3237.,type=arg_float, help=': %(default)s [m], observatory height above sea level [m], negative value: m10. equals to -10.')
  parser.add_argument('--mount-data', dest='mount_data', action='store', default='./mount_data.txt', help=': %(default)s, mount data filename')
  parser.add_argument('--fit-eq', dest='fit_eq', action='store_true', default=False, help=': %(default)s, True fit EQ model, else AltAz')
  parser.add_argument('--t-point', dest='t_point', action='store_true', default=False, help=': %(default)s, fit EQ model with T-point compatible model')
  parser.add_argument('--fit-plus-poly', dest='fit_plus_poly', action='store_true', default=False, help=': %(default)s, True: Condon 1992 with polynom')
  parser.add_argument('--astropy', dest='astropy', action='store_true', default=False, help=': %(default)s,True: calculate apparent position with astropy, default libnova')
  parser.add_argument('--bins', dest='bins', action='store', default=40,type=int, help=': %(default)s, number of bins used in the projection histograms')
  parser.add_argument('--plot', dest='plot', action='store_true', default=False, help=': %(default)s, plot results')
  parser.add_argument('--base-path', dest='base_path', action='store', default='./u_point_data/',type=str, help=': %(default)s , directory where images are stored')
            
  args=parser.parse_args()
  
  filename='/tmp/{}.log'.format(sys.argv[0].replace('.py','')) # ToDo datetime, name of the script
  logformat= '%(asctime)s:%(name)s:%(levelname)s:%(message)s'
  logging.basicConfig(filename=filename, level=args.level.upper(), format= logformat)
  logger = logging.getLogger()
    
  if args.level in 'DEBUG' or args.level in 'INFO':
    toconsole=True
  else:
    toconsole=args.toconsole

  if toconsole:
    # http://www.mglerner.com/blog/?p=8
    soh = logging.StreamHandler(sys.stdout)
    soh.setLevel(args.level)
    logger.addHandler(soh)

  # ToDo: do this with argparse
  if args.fit_eq and args.fit_plus_poly:
    logger.error('--fit-plus-poly can not be specified with --fit-eq, drop either')
    sys.exit(1)
  
  pm= PointingModel(dbg=args.debug,lg=logger,base_path=args.base_path,obs_lng=args.obs_lng,obs_lat=args.obs_lat,obs_height=args.obs_height)

  if not os.path.exists(args.base_path):
    os.makedirs(args.base_path)

  if args.t_point:
    args.fit_eq=True
  stars=list()
  if args.fit_eq:
    # cat_eqs,mnt_eqs: HA, Dec coordinates
    cat_has,mnt_has=pm.fetch_coordinates(ptfn=args.mount_data,astropy_f=args.astropy,break_after=args.break_after,fit_eq=args.fit_eq)
    res=pm.fit_model_hadec(cat_has=cat_has,mnt_has=mnt_has,t_point=args.t_point) 
    if not args.plot:
      sys.exit(1)
    # create data structure more suitable for plotting
    for i, cat_ha in enumerate(cat_has):
      mnt_ha=mnt_has[i] # readability
          
      df_dec= Latitude(cat_ha.dec.radian-mnt_ha.dec.radian,u.radian)
      df_ha= Longitude(cat_ha.ra.radian-mnt_ha.ra.radian,u.radian,wrap_angle=Angle(np.pi,u.radian))
      #  residuum: difference st.cat(fit corrected) - st.star      
      d_ha_f=d_ha
      d_dec_f=d_dec
      if args.t_point:
        d_ha_f=d_ha_t_point
        d_dec_f=d_dec_t_point
      
      res_ha=Longitude(        
        # ToDo! sign
        #float(d_ha_f(cat_ha.ra.radian,cat_ha.dec.radian,cat_ha.ra.radian-mnt_ha.ra.radian,pm.obs.latitude.radian)),
        float(d_ha_f(cat_ha.ra.radian,cat_ha.dec.radian,(cat_ha.ra.radian-mnt_ha.ra.radian),pm.obs.latitude.radian)),
        u.radian,
        wrap_angle=Angle(np.pi,u.radian))

      res_dec=Latitude(
        # ToDo! sign
        #float(d_dec_f(cat_ha.ra.radian,cat_ha.dec.radian,cat_ha.dec.radian-mnt_ha.dec.radian,pm.obs.latitude.radian)
        float(d_dec_f(cat_ha.ra.radian,cat_ha.dec.radian,-(cat_ha.dec.radian-mnt_ha.dec.radian),pm.obs.latitude.radian)),
        u.radian)

      st=Point(
        cat_lon=cat_ha.ra,cat_lat=cat_ha.dec,
        mnt_lon=mnt_ha.ra,mnt_lat=mnt_ha.dec,
        df_lat=df_dec,df_lon=df_ha,
        res_lat=res_dec,res_lon=res_ha
      )
      stars.append(st)
  else:
    # cat_aas,mnt_aas: AltAz coordinates
    cat_aas,mnt_aas=pm.fetch_coordinates(ptfn=args.mount_data,astropy_f=args.astropy,break_after=args.break_after,fit_eq=args.fit_eq)
    res=pm.fit_model_altaz(cat_aas=cat_aas,mnt_aas=mnt_aas,fit_plus_poly=args.fit_plus_poly)

    if not args.plot:
      sys.exit(1)
    for i, cat_aa in enumerate(cat_aas):
      mnt_aa=mnt_aas[i] # readability
          
      df_alt= Latitude(cat_aa.alt.radian-mnt_aas[i].alt.radian,u.radian)
      df_az= Longitude(cat_aa.az.radian-mnt_aa.az.radian,u.radian, wrap_angle=Angle(np.pi,u.radian))
      #if df_alt.radian < 0./60./180.*np.pi:
      #  pass
      #elif df_alt.radian > 20./60./180.*np.pi:
      #  pass
      #else:
      #  continue
      #  residuum: difference st.cat(fit corrected) - st.star
      #
      res_az=Longitude(        
        float(d_az(cat_aa.az.radian,cat_aa.alt.radian,cat_aa.az.radian-mnt_aa.az.radian)),
        u.radian,
        wrap_angle=Angle(np.pi,u.radian))

      res_alt=Latitude(
        float(d_alt(cat_aa.az.radian,cat_aa.alt.radian,cat_aa.alt.radian-mnt_aa.alt.radian,args.fit_plus_poly)),u.radian)

      st=Point(
        cat_lon=cat_aa.az,cat_lat=cat_aa.alt,
        mnt_lon=mnt_aa.az,mnt_lat=mnt_aa.alt,
        df_lat=df_alt,df_lon=df_az,
        res_lat=res_alt,res_lon=res_az
      )
      stars.append(st)

  if args.plot:
    pm.plot_results(stars=stars,args=args)

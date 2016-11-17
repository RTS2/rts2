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
import importlib

import numpy as np
import scipy.optimize
import pandas as pd

from astropy import units as u
from astropy.time import Time,TimeDelta
from astropy.coordinates import SkyCoord,EarthLocation
from astropy.coordinates import AltAz,CIRS,ITRS
from astropy.coordinates import Longitude,Latitude,Angle
from astropy.coordinates.representation import SphericalRepresentation

from structures import Point,Parameter

# find out how caching works
#from astropy.utils import iers
#iers.IERS.iers_table = iers.IERS_A.open(iers.IERS_A_URL)

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
      # ... but not for the star position as measured in mount frame
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
    if correct_cat_f: # ToDo: again
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
    pre_qfe=0.
    if correct_cat_f: # ToDo: again
      pre_qfe=pre # to make it clear what is used
      
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
      pd_cat = self.fetch_pandas(ptfn=fn,columns=['utc','cat_ra','cat_dc','mnt_ra','mnt_dc','exp','pre','tem','hum'])
      
    if pd_cat is None:
      return None,None

    for i,rw in pd_cat.iterrows():
      if i > break_after:
        break
      dt_utc = Time(rw['utc'],format='iso', scale='utc',location=self.obs)
      if pd.notnull(rw['exp']):
        dt_utc=dt_utc - TimeDelta(rw['exp']/2.,format='sec') # exp. time is small

      cat_eq=SkyCoord(ra=rw['cat_ra'],dec=rw['cat_dc'], unit=(u.rad,u.rad), frame='icrs',obstime=dt_utc,location=self.obs)
      cat_eq_sp=cat_eq.represent_as(SphericalRepresentation)

      if pd.notnull(rw['mnt_ra']) and pd.notnull(rw['mnt_dc']):
        # replace icrs by cirs (intermediate frame, mount apparent coordinates)
        mnt_eq=SkyCoord(ra=rw['mnt_ra'],dec=rw['mnt_dc'], unit=(u.rad,u.rad), frame='cirs',obstime=dt_utc,location=self.obs)
      else:
        self.lg.warn('fetch_coordinates: no analyzed position on line: {}\n{}'.format(i,rw))
        continue
      
      if pd.isnull(rw['pre']):
        pre=tem=hum=0.
      else:
        pre=rw['pre']
        tem=rw['tem']
        hum=rw['hum']
        
      if fit_eq:
        cat_ha=self.transform_to_hadec(eq=cat_eq,tem=tem,pre=pre,hum=hum,astropy_f=astropy_f,correct_cat_f=True)
        
        # to be sure :-))
        pre=tem=hum=0.
        mnt_ha=self.transform_to_hadec(eq=mnt_eq,tem=tem,pre=pre,hum=hum,astropy_f=astropy_f,correct_cat_f=False)
        cats.append(cat_ha)
        mnts.append(mnt_ha)
      else:
        cat_aa=self.transform_to_altaz(eq=cat_eq,tem=tem,pre=pre,hum=hum,astropy_f=astropy_f,correct_cat_f=True)
        pre=tem=hum=0.
        mnt_aa=self.transform_to_altaz(eq=mnt_eq,tem=tem,pre=pre,hum=hum,astropy_f=astropy_f,correct_cat_f=False)
        cats.append(cat_aa)
        mnts.append(mnt_aa)
        
    return cats,mnts

  
  # fit projections with a gaussian
  def fit_projection_helper(self,function=None, parameters=None, y=None, x = None):
    def local_f(params):
      for i,p in enumerate(parameters):
        p.set(params[i])
      return y - function(x)
            
    p = [param() for param in parameters]
    return scipy.optimize.leastsq(local_f, p)

  def fit_projection(self,x):
    return self.height() * np.exp(-(x-self.mu())**2/2./self.sigma()**2) #+ background()

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

    self.mu=Parameter(0.)
    self.sigma=Parameter(wd) #arcsec!
    self.height=Parameter(cnt_bins.max())
    background=Parameter(0.)
    parameters=[self.mu,self.sigma,self.height]# ToDo a bit a contradiction
    res,stat=self.fit_projection_helper(function=self.fit_projection,parameters=parameters, y=cnt_bins, x=bins)
    if stat != 1:
      self.lg.warn('fit projection not converged, status: {}'.format(stat))
      
    y=self.fit_projection(bins)
    l = ax.plot(bins, y, 'r--', linewidth=2)
    ax.set_xlabel('{} {} [arcsec]'.format(prefix,axis))
    ax.set_ylabel('number of events normalized')
    if prefix in 'difference': # ToDo ugly
      ax.set_title('{0} {1} {2} $\mu$={3:.2f},$\sigma$={4:.2f} [arcsec] \n catalog_not_corrected - star'.format(plt_no,prefix,axis,self.mu(), self.sigma()))
      fig.savefig(os.path.join(self.base_path,'{}_catalog_not_corrected_projection_{}.png'.format(prefix,axis)))
    else:
      ax.set_title(r'{0} {1} {2} $\mu$={3:.2f},$\sigma$={4:.2f} [arcsec], fit: {5}'.format(plt_no,prefix,axis,self.mu(), self.sigma(),fit_title))
      fig.savefig(os.path.join(self.base_path,'{}_projection_{}_{}.png'.format(prefix,axis,fn_frac)))

  def prepare_plot(self, cats=None,mnts=None,selected=None,model=None):
    stars=list()
    for i, ct in enumerate(cats):
      if not i in selected:
        self.lg.debug('star: {} dropped'.format(i))
        continue
      
      mt=mnts[i] # readability
      # ToDo may not the end of the story
      cts=ct.represent_as(SphericalRepresentation)
      mts=mt.represent_as(SphericalRepresentation)
      df_lat= Latitude(cts.lat.radian-mts.lat.radian,u.radian)
      df_lat= Latitude(cts.lat.radian-mts.lat.radian,u.radian)
      df_lon= Longitude(cts.lon.radian-mts.lon.radian,u.radian, wrap_angle=Angle(np.pi,u.radian))
      #if df_lat.radian < 0./60./180.*np.pi:
      #  pass
      #elif df_lat.radian > 20./60./180.*np.pi:
      #  pass
      #else:
      #  continue
      #  residuum: difference st.cats(fit corrected) - st.star
      #
      res_lon=Longitude(        
        float(model.d_lon(cts.lon.radian,cts.lat.radian,cts.lon.radian-mts.lon.radian)),
        u.radian,
        wrap_angle=Angle(np.pi,u.radian))

      res_lat=Latitude(
        float(model.d_lat(cts.lon.radian,cts.lat.radian,cts.lat.radian-mts.lat.radian)),u.radian)

      st=Point(
        cat_lon=cts.lon,cat_lat=cts.lat,
        mnt_lon=mts.lon,mnt_lat=mts.lat,
        df_lat=df_lat,df_lon=df_lon,
        res_lat=res_lat,res_lon=res_lon
      )
      stars.append(st)
                      
    return stars

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

  def select_stars(self, stars=None):

    slected=list()
    dropped=list()
    for i,st in enumerate(stars):
      #st=Point(
      #    cat_lon=cat_aa.az,cat_lat=cat_aa.alt,
      #    mnt_lon=mnt_aa.az,mnt_lat=mnt_aa.alt,
      #    df_lat=df_alt,df_lon=df_az,
      #    res_lat=res_alt,res_lon=res_az
      if True:
        if abs(st.res_lat.radian)> 30./3600./180.*np.pi:
          selected.append(i)
        else:
          dropped.append(i)
        if abs(st.res_lon.radian)> 30./3600./180.*np.pi:
          selected.append(i)
        else:
          dropped.append(i)

      else:
        if abs(st.res_lat.radian)> 30./3600./180.*np.pi:
          if abs(st.res_lon.radian)> 30./3600./180.*np.pi:
            selected.append(i)
          else:
            dropped.append(i)
        else:
          dropped.append(i)
          
    self.lg.debug('Number of selected stars: {} '.format(len(selected)))

    return selected, dropped


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
  parser.add_argument('--model-class', dest='model_class', action='store', default='model_altaz', help=': %(default)s, specify your model, see e.g. model_altaz.py')
            
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
  
  # cat,mnt: AltAz, or HA,dec coordinates
  cats,mnts=pm.fetch_coordinates(ptfn=args.mount_data,astropy_f=args.astropy,break_after=args.break_after,fit_eq=args.fit_eq)
  if cats is None:
    logger.error('nothing to analyze, exiting')
    sys.exit(1)
  # now load model class
  md = importlib.import_module(args.model_class)
  logger.info('loaded: {}'.format(args.model_class))
  # required methods: fit_model, d_lon, d_lat
  mdl=md.Model(lg=logger)

  selected=list(range(0,len(cats))) # all  
  if args.fit_eq:
    res=mdl.fit_model(cats=cats,mnts=mnts,selected=selected,obs=pm.obs)
  else:
    res=mdl.fit_model(cats=cats,mnts=mnts,selected=selected,fit_plus_poly=args.fit_plus_poly)
    
  stars=pm.prepare_plot(cats=cats,mnts=mnts,selected=selected,model=mdl)
    
  if args.plot:
    pm.plot_results(stars=stars,args=args)

  #selected,dropped=pm.select_stars(stars=stars)
  #stars=pm.prepare_plot(cat=cat,mnt=mnt,selected=selected)
  #res=pm.fit_model_altaz(cat_aas=cat,mnt_aas=mnt,fit_plus_poly=args.fit_plus_poly,selected=selected)

  #pm.plot_results(stars=stars,args=args)
  

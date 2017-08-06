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
from astropy.coordinates import SkyCoord,EarthLocation
from astropy.coordinates import Longitude,Latitude,Angle
from astropy.coordinates.representation import SphericalRepresentation
from astropy.utils import iers
# astropy pre 1.2.1 may not work correctly
#  wget http://maia.usno.navy.mil/ser7/finals2000A.all
# together with IERS_A_FILE
try:
  iers.IERS.iers_table = iers.IERS_A.open(iers.IERS_A_FILE)
#                                               ###########
except:
  print('download:')
  print('wget http://maia.usno.navy.mil/ser7/finals2000A.all')
  sys.exit(1)

from u_point.structures import Point,Parameter
from u_point.callback import AnnoteFinder
from u_point.callback import  AnnotatedPlot
from u_point.script import Script

# find out how caching works
#from astropy.utils import iers
#iers.IERS.iers_table = iers.IERS_A.open(iers.IERS_A_URL)


class PointingModel(Script):
  def __init__(self, lg=None,break_after=None, base_path=None, obs=None,analyzed_positions=None,fit_sxtr=None):
    Script.__init__(self,lg=lg,break_after=break_after,base_path=base_path,obs=obs,analyzed_positions=analyzed_positions)
    #
    self.fit_sxtr=fit_sxtr
    self.transform_name=None
    self.refraction_method=None

  def fetch_coordinates(self,ptfn=None):
    
    self.fetch_positions(sys_exit=True,analyzed=True)
    # old irait data do not enable
    #self.fetch_mount_meteo(sys_exit=True,analyzed=True,with_nml_id=False)

    cats=list()
    mnts=list()
    imgs=list()
    nmls=list()

    if len(self.sky_anl)==0:
      self.lg.error('fetch_coordinates: nothing to analyze, exiting')
      sys.exit(1)
      
    self.eq_mount=False
    if self.sky_anl[0].eq_mount:
      self.eq_mount=True
      
    self.transform_name=None
    if self.sky_anl[0].transform_name:
      self.transform_name=self.sky_anl[0].transform_name      
    logger.info('transformation done with: {}'.format(self.transform_name))

    self.refraction_method=None
    if self.sky_anl[0].refraction_method:
      self.refraction_method=self.sky_anl[0].refraction_method
    # ToDo 
    #logger.info('refraction_method: {}'.format(self.refraction_method))

    for i,sky in enumerate(self.sky_anl):
      if i > self.break_after:
        break

      if self.fit_sxtr:
        if sky.mnt_ll_sxtr is None:
          continue
        mnt_ll=sky.mnt_ll_sxtr
      else:
        if sky.mnt_ll_astr is None:
          continue        
        mnt_ll=sky.mnt_ll_astr
      
      cats.append(sky.cat_ll_ap)
      mnts.append(mnt_ll)
      
      imgs.append(sky.image_fn)
      nmls.append(sky.nml_id)
    
    return cats,mnts,imgs,nmls

  
  # fit projections with a gaussian
  def fit_projection_helper(self,function=None, parameters=None, y=None, x = None):
    def local_f(params):
      for i,p in enumerate(parameters):
        p.set(params[i])
      return y - function(x)
            
    p = [param() for param in parameters]
    return scipy.optimize.leastsq(local_f, p)

  def fit_gaussian(self,x):
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
    res,stat=self.fit_projection_helper(function=self.fit_gaussian,parameters=parameters, y=cnt_bins, x=bins)
    if stat != 1:
      self.lg.warn('fit projection not converged, status: {}'.format(stat))
      
    y=self.fit_gaussian(bins)
    l = ax.plot(bins, y, 'r--', linewidth=2)
    ax.set_xlabel('{} {} [arcsec]'.format(prefix,axis))
    ax.set_ylabel('number of events normalized')
    if prefix in 'difference': # ToDo ugly
      # as of 2017-03-18 TeX does not work
      #ax.set_title(r'{0} {1} {2} $\mu$={3:.2f},$\sigma$={4:.2f} [arcsec] \n catalog_not_corrected - star'.format(plt_no,prefix,axis,self.mu(), self.sigma()))
      ax.set_title(r'{0} {1} {2} mu={3:.2f},sigma={4:.2f} [arcsec] \n catalog_not_corrected - star'.format(plt_no,prefix,axis,self.mu(), self.sigma()))
      fn_ftmp=fn_frac.replace(' ','_').replace('+','_')
      axis_tmp=axis.replace(' ','_').replace('+','_')
      fig.savefig(os.path.join(self.base_path,'{}_catalog_not_corrected_projection_{}_{}.png'.format(prefix,axis_tmp,fn_ftmp)))
    else:
      #ax.set_title(r'{0} {1} {2} $\mu$={3:.2f},$\sigma$={4:.2f} [arcsec], fit: {5}'.format(plt_no,prefix,axis,self.mu(), self.sigma(),fit_title))
      ax.set_title(r'{0} {1} {2} mu={3:.2f},sigma={4:.2f} [arcsec], fit: {5}'.format(plt_no,prefix,axis,self.mu(), self.sigma(),fit_title))
      #ToDo ugly
      fn_ftmp=fn_frac.replace(' ','_').replace('+','_')
      axis_tmp=axis.replace(' ','_').replace('+','_')
      fig.savefig(os.path.join(self.base_path,'{}_projection_{}_{}.png'.format(prefix,axis_tmp,fn_ftmp)))

  def prepare_plot(self, cats=None,mnts=None,imgs=None,nmls=None,selected=None,model=None):
    stars=list()
    for i, ct in enumerate(cats):
      if not i in selected:
        #self.lg.debug('star: {} dropped'.format(i))
        continue
      
      mt=mnts[i] # readability
      mts=mt.represent_as(SphericalRepresentation)
      # ToDo may not the end of the story
      cts=ct.represent_as(SphericalRepresentation)
      df_lon= Longitude(cts.lon.radian-mts.lon.radian,u.radian, wrap_angle=Angle(np.pi,u.radian))
      df_lat= Latitude(cts.lat.radian-mts.lat.radian,u.radian)
      #print(df_lat,df_lon)
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

      try:
        image_fn=imgs[i]
      except:
        image_fn='no image file'
      try:
        nml_id=nmls[i]
      except:
        nml_id='no nml_id'
      st=Point(
        cat_lon=cts.lon,
        cat_lat=cts.lat,
        mnt_lon=mts.lon,
        mnt_lat=mts.lat,
        df_lat=df_lat,
        df_lon=df_lon,
        res_lat=res_lat,
        res_lon=res_lon,
        image_fn=image_fn,
        nml_id=nml_id,
      )
      stars.append(st)
                      
    return stars

  def annotate_plot(self,fig=None,ax=None,aps=None,ds9_display=None,delete=None):
    af =  AnnoteFinder(
      ax=ax, # leading plot, put it on the list
      aps=aps,
      xtol=1.,
      ytol=1.,
      ds9_display=ds9_display,
      lg=self.lg,
      annotate_fn=False,
      analyzed=True,
      delete_one=self.delete_one_position,)
    return af

  def create_plot(self,fig=None,ax=None,title=None,xlabel=None,ylabel=None,lon=None,lat=None,fn=None):
    ax.set_title(title)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.grid(True)
    ax.scatter(lon,lat,picker=20)
    fig.savefig(os.path.join(self.base_path,fn.replace(' ','_').replace('+','_')))
  
  def plot_results(self, stars=None,args=None):
    '''
    Plot the differences and residues as a function of lon,lat or dlon,dlat or a combination of them.
    '''
    import matplotlib
    # this varies from distro to distro:
    matplotlib.rcParams["backend"] = "TkAgg"
    import matplotlib.pyplot as plt
    #matplotlib.rc('text', usetex = True)
    #matplotlib.rc('font', **{'family':"sans-serif"})
    #params = {'text.latex.preamble': [r'\usepackage{siunitx}',
    #                                  r'\usepackage{sfmath}', r'\sisetup{detect-family = true}',
    #                                  r'\usepackage{amsmath}']}
    #plt.rcParams.update(params)
    
    plt.ioff()
    
    fit_title=model.fit_title
    if args.fit_plus_poly:
      fit_title +='C+PP'
      
    frag='_' + self.transform_name.upper()[0:2]
    fit_title += frag
    
    sx='_AS'
    if args.fit_sxtr:
      sx='_SX'
    fit_title += sx

    frag= '_'+ self.refraction_method.upper()[0:2]
    fit_title += frag

    fn_frac=fit_title
    if args.fit_plus_poly:
      fn_frac+='c_plus_poly'
      
    if self.eq_mount:
      lon_label='hour angle'
      lat_label='declination'
    else:
      lat_label='altitude'
      lon_label='azimuth N=0,E=90'
      if 'nova' in self.transform_name:
        lon_label='azimuth S=0,W=90'
      

    az_cat_deg=[x.cat_lon.degree for x in stars]
    alt_cat_deg=[x.cat_lat.degree for x in stars]
    
    plots=list()
    elements=list()
    elements.append('A1 difference: catalog_not_corrected - star')
    elements.append('d({}) [arcmin]'.format(lon_label))
    elements.append('d({}) [arcmin]'.format(lat_label))
    lon=[x.df_lon.arcmin for x in stars]
    lat=[x.df_lat.arcmin for x in stars]
    elements.append(lon)
    elements.append(lat)
    elements.append('difference_catalog_not_corrected_star{0}.png'.format(fn_frac))
    elements.append(['{0:.1f},{1:.1f}: {2}'.format(x.df_lon.arcmin,x.df_lat.arcmin,x.image_fn) for x in stars])
    plots.append(elements)
    
    elements=list()
    elements.append('B1 difference {}: catalog_not_corrected - star'.format(lon_label))
    elements.append('{} [deg]'.format(lon_label))
    elements.append('d({}) [arcmin]'.format(lon_label))
    lon=az_cat_deg
    lat=[x.df_lon.arcmin for x in stars]
    elements.append(lon)
    elements.append(lat)
    elements.append('difference_{0}_d{0}_catalog_not_corrected_star{1}.png'.format(lon_label,fn_frac))
    elements.append(['{0:.1f},{1:.1f}: {2}'.format(x.cat_lon.degree,x.df_lon.arcmin,x.image_fn) for x in stars])
    plots.append(elements)

    elements=list()
    elements.append('C1 difference {}: catalog_not_corrected - star'.format(lat_label))
    elements.append('{} [deg]'.format(lon_label))
    elements.append('d({}) [arcmin]'.format(lat_label))
    lon=az_cat_deg
    lat=[x.df_lat.arcmin for x in stars]
    elements.append(lon)
    elements.append(lat)
    elements.append('difference_{0}_d{1}_catalog_not_corrected_star{2}.png'.format(lon_label,lat_label,fn_frac))
    elements.append(['{0:.1f},{1:.1f}: {2}'.format(x.cat_lon.degree,x.df_lat.arcmin,x.image_fn) for x in stars])
    plots.append(elements)

    elements=list()
    elements.append('D1 difference {}: catalog_not_corrected - star'.format(lon_label))
    elements.append('{} [deg]'.format(lat_label))
    elements.append('d({}) [arcmin]'.format(lon_label))
    lon=alt_cat_deg
    lat=[x.df_lon.arcmin for x in stars]
    elements.append(lon)
    elements.append(lat)
    # ToDo: think about that:
    #ax.scatter(alt_cat_deg ,[x.df_lon.arcmin/ np.tan(x.mnt_lat.radian) for x in stars])
    elements.append('difference_{0}_d{1}_catalog_not_corrected_star{2}.png'.format(lat_label,lon_label,fn_frac))
    elements.append(['{0:.1f},{1:.1f}: {2}'.format(x.cat_lat.degree,x.df_lon.arcmin,x.image_fn) for x in stars])
    plots.append(elements)

    elements=list()
    elements.append('E1 difference {}: catalog_not_corrected - star'.format(lat_label))
    elements.append('{} [deg]'.format(lat_label))
    elements.append('d({}) [arcmin]'.format(lat_label))
    lon=alt_cat_deg
    lat=[x.df_lat.arcmin for x in stars]
    elements.append(lon)
    elements.append(lat)
    elements.append('difference_{0}_d{0}_catalog_not_corrected_star{1}.png'.format(lat_label,fn_frac))
    elements.append(['{0:.1f},{1:.1f}: {2}'.format(x.cat_lat.degree,x.df_lat.arcmin,x.image_fn) for x in stars])
    plots.append(elements)
    
    elements=list()
    ## residuum, ax05 is below
    elements.append('A2 residuum: catalog_corrected - star {}'.format(fit_title))
    elements.append('d({}) [arcmin]'.format(lon_label))
    elements.append('d({}) [arcmin]'.format(lat_label))
    lon=[x.res_lon.arcmin for x in stars]
    lat=[x.res_lat.arcmin for x in stars]
    elements.append(lon)
    elements.append(lat)
    elements.append('residuum_catalog_corrected_star_{}.png'.format(fn_frac))
    elements.append(['{0:.1f},{1:.1f}: {2}'.format(x.res_lon.arcmin,x.res_lat.arcmin,x.image_fn) for x in stars])
    plots.append(elements)
    
    elements=list()
    elements.append('B2 residuum {} catalog_corrected - star, fit: {}'.format(lon_label,fit_title))
    elements.append('{} [deg]'.format(lon_label))
    elements.append('d({}) [arcmin]'.format(lon_label))
    lon=az_cat_deg
    lat=[x.res_lon.arcmin for x in stars]
    elements.append(lon)
    elements.append(lat)
    elements.append('residuum_{0}_d{0}_catalog_corrected_star_{1}.png'.format(lon_label,fn_frac))
    elements.append(['{0:.1f},{1:.1f}: {2}'.format(x.cat_lon.degree,x.res_lon.arcmin,x.image_fn) for x in stars])
    plots.append(elements)

    elements=list()
    elements.append('D2 residuum {} catalog_corrected - star, fit: {}'.format(lon_label,fit_title))
    elements.append('{} [deg]'.format(lat_label))
    elements.append('d({}) [arcmin]'.format(lon_label))
    lon=alt_cat_deg
    lat=[x.res_lon.arcmin for x in stars]
    elements.append(lon)
    elements.append(lat)
    elements.append('residuum_{0}_d{1}_catalog_corrected_star_{2}.png'.format(lat_label,lon_label,fn_frac))
    elements.append(['{0:.1f},{1:.1f}: {2}'.format(x.cat_lat.degree,x.res_lon.arcmin,x.image_fn) for x in stars])
    plots.append(elements)

    elements=list()
    elements.append('C2 residuum {} catalog_corrected - star, fit: {}'.format(lat_label,fit_title))
    elements.append('{} [deg]'.format(lon_label))
    elements.append('d({}) [arcmin]'.format(lat_label))
    lon=az_cat_deg
    lat=[x.res_lat.arcmin for x in stars]
    elements.append(lon)
    elements.append(lat)
    elements.append('residuum_{0}_d{1}_catalog_corrected_star_{2}.png'.format(lon_label,lat_label,fn_frac))
    elements.append(['{0:.1f},{1:.1f}: {2}'.format(x.cat_lon.degree,x.res_lat.arcmin,x.image_fn) for x in stars])
    plots.append(elements)
          
    elements=list()
    elements.append('E2 residuum {} catalog_corrected - star, fit: {}'.format(lat_label,fit_title))
    elements.append('{}  [deg]'.format(lat_label))
    elements.append('d({}) [arcmin]'.format(lat_label))
    lon=alt_cat_deg
    lat=[x.res_lat.arcmin for x in stars]
    elements.append(lon)
    elements.append(lat)
    elements.append('residuum_{0}_d{0}_catalog_corrected_star_{1}.png'.format(lat_label,fn_frac))
    elements.append(['{0:.1f},{1:.1f}: {2}'.format(x.cat_lat.degree,x.res_lat.arcmin,x.image_fn) for x in stars])
    plots.append(elements)

    elements=list()
    elements.append('K measurement locations catalog')
    elements.append('{} [deg]'.format(lon_label))
    elements.append('{} [deg]'.format(lat_label))
    lon=[x.cat_lon.degree for x in stars]
    lat=[x.cat_lat.degree for x in stars]
    elements.append(lon)
    elements.append(lat)
    elements.append('measurement_locations_catalog_{0}.png'.format(fn_frac))
    elements.append(['{0:.1f},{1:.1f}: {2}'.format(x.cat_lon.degree,x.cat_lat.degree,x.image_fn) for x in stars])
    plots.append(elements)
    
    annotes_skycoords=['{0:.1f},{1:.1f}: {2}'.format(x.cat_lon.degree,x.cat_lat.degree,x.image_fn) for x in stars]
    figs=list()
    axs=list()
    aps=list()
    nml_ids=[x.nml_id for x in stars]
    # aps must be complete
    for elements in plots:
      lon=elements[3]
      lat=elements[4]
      fig = plt.figure()
      ax = fig.add_subplot(111)
      self.create_plot(fig=fig,ax=ax,title=elements[0],xlabel=elements[1],ylabel=elements[2],lon=lon,lat=lat,fn=elements[5])
      figs.append(fig)
      axs.append(ax)
      # it deppends what is needed:
      #annotes=elements[6]
      annotes=annotes_skycoords
      aps.append(AnnotatedPlot(xx=ax,nml_id=nml_ids,lon=lon,lat=lat,annotes=annotes))
      
    afs=list()
    for i,ax in enumerate(axs):
      af=self.annotate_plot(fig=figs[i],ax=axs[i],aps=aps,ds9_display=args.ds9_display,delete=args.delete)
      # ToDo ??removing this list inhibits call back on all but one plot
      afs.append(af)
      figs[i].canvas.mpl_connect('button_press_event',af.mouse_event)
      if args.delete:
        figs[i].canvas.mpl_connect('key_press_event',af.keyboard_event)

    # ToDo why that?
    #figs[0].canvas.mpl_connect('button_press_event',afs[0].mouse_event)
    #if args.delete:
    #  figs[0].canvas.mpl_connect('key_press_event',afs[0].keyboard_event)

    self.fit_projection_and_plot(vals=[x.df_lon.arcsec for x in stars], bins=args.bins,axis='{}'.format(lon_label), fit_title=fit_title,fn_frac=fn_frac,prefix='difference',plt_no='P1',plt=plt)
    self.fit_projection_and_plot(vals=[x.df_lat.arcsec for x in stars], bins=args.bins,axis='{}'.format(lat_label),fit_title=fit_title,fn_frac=fn_frac,prefix='difference',plt_no='P2',plt=plt)
    self.fit_projection_and_plot(vals=[x.res_lon.arcsec for x in stars],bins=args.bins,axis='{}'.format(lon_label), fit_title=fit_title,fn_frac=fn_frac,prefix='residuum',plt_no='Q1',plt=plt)
    self.fit_projection_and_plot(vals=[x.res_lat.arcsec for x in stars],bins=args.bins,axis='{}'.format(lat_label),fit_title=fit_title,fn_frac=fn_frac,prefix='residuum',plt_no='Q2',plt=plt)
    
    plt.show()

  def select_stars(self, stars=None):

    slctd=list()
    drppd=list()
    for i,st in enumerate(stars):
      #st=Point(
      #    cat_lon=cat_aa.az,cat_lat=cat_aa.alt,
      #    mnt_lon=mnt_aa.az,mnt_lat=mnt_aa.alt,
      #    df_lat=df_alt,df_lon=df_az,
      #    res_lat=res_alt,res_lon=res_az
      dist2 = st.res_lat.radian**2 + st.res_lon.radian**2
      
      if dist2> (30./3600./180.*np.pi)**2:
        slctd.append(i)
      else:
        drppd.append(i)
    # ToDo not yet drop set()
    selected=list(set(slctd))
    dropped=list(set(drppd))
    self.lg.debug('Number of selected stars: {} '.format(len(selected)))
    return selected, dropped


# really ugly!
def arg_float(value):
  if 'm' in value:
    return -float(value[1:])
  else:
    return float(value)
    
if __name__ == "__main__":

  parser= argparse.ArgumentParser(prog=sys.argv[0], description='Fit an AltAz or EQ pointing model')
  parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
  parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
  parser.add_argument('--break_after', dest='break_after', action='store', default=10000000, type=int, help=': %(default)s, read max. positions, mostly used for debuging')
  parser.add_argument('--base-path', dest='base_path', action='store', default='/tmp/u_point/',type=str, help=': %(default)s , directory where images are stored')
  parser.add_argument('--analyzed-positions', dest='analyzed_positions', action='store', default='analyzed_positions.anl', help=': %(default)s, already observed positions')
  #
  parser.add_argument('--obs-longitude', dest='obs_lng', action='store', default=123.2994166666666,type=arg_float, help=': %(default)s [deg], observatory longitude + to the East [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-latitude', dest='obs_lat', action='store', default=-75.1,type=arg_float, help=': %(default)s [deg], observatory latitude [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-height', dest='obs_height', action='store', default=3237.,type=arg_float, help=': %(default)s [m], observatory height above sea level [m], negative value: m10. equals to -10.')
  #
  parser.add_argument('--fit-sxtr', dest='fit_sxtr', action='store_true', default=False, help=': %(default)s, True fit SExtractor results')
  # group model
  parser.add_argument('--model-class', dest='model_class', action='store', default='u_upoint', help=': %(default)s, specify your model, see e.g. model/altaz.py')
  parser.add_argument('--fit-plus-poly', dest='fit_plus_poly', action='store_true', default=False, help=': %(default)s, True: Condon 1992 with polynom')
  # group plot
  parser.add_argument('--plot', dest='plot', action='store_true', default=False, help=': %(default)s, plot results')
  parser.add_argument('--bins', dest='bins', action='store', default=40,type=int, help=': %(default)s, number of bins used in the projection histograms')
  parser.add_argument('--ds9-display', dest='ds9_display', action='store_true', default=False, help=': %(default)s, inspect image and region with ds9')
  parser.add_argument('--delete', dest='delete', action='store_true', default=False, help=': %(default)s, True: click on data point followed by keyboard <Delete> deletes selected ananlyzed point from file --analyzed-positions')
        
  args=parser.parse_args()
  
  if args.toconsole:
    args.level='DEBUG'
    
  if not os.path.exists(args.base_path):
    os.makedirs(args.base_path)

  pth, fn = os.path.split(sys.argv[0])
  filename=os.path.join(args.base_path,'{}.log'.format(fn.replace('.py',''))) # ToDo datetime, name of the script
  logformat= '%(asctime)s:%(name)s:%(levelname)s:%(message)s'
  logging.basicConfig(filename=filename, level=args.level.upper(), format= logformat)
  logger = logging.getLogger()

  if args.toconsole:
    # http://www.mglerner.com/blog/?p=8
    soh = logging.StreamHandler(sys.stdout)
    soh.setLevel(args.level)
    logger.addHandler(soh)

    
  if not os.path.exists(args.base_path):
    os.makedirs(args.base_path)

  obs=EarthLocation(lon=float(args.obs_lng)*u.degree, lat=float(args.obs_lat)*u.degree, height=float(args.obs_height)*u.m)  
  # now load model class
  md = importlib.import_module('model.'+args.model_class)
  logger.info('model loaded: {}'.format(args.model_class))
  # required methods: fit_model, d_lon, d_lat
  model=md.Model(lg=logger)
      

  pm= PointingModel(lg=logger,break_after=args.break_after,base_path=args.base_path,obs=obs,analyzed_positions=args.analyzed_positions,fit_sxtr=args.fit_sxtr)

  # cat,mnt: AltAz, or HA,dec coordinates
  cats,mnts,imgs,nmls=pm.fetch_coordinates()

  # check model type, mount type
  if pm.eq_mount:
    if 'hadec' not in model.model_type():
      logger.error('u_model: model: {}, type: {}'.format(args.model_class, model.model_type()))
      logger.error('u_model: specify hadec model type, exiting')
      sys.exit(1)
  else:
    if 'altaz' not in model.model_type():
      logger.error('u_model: model: {}, type: {}'.format(args.model_class, model.model_type()))
      logger.error('u_model: specify altaz model type, exiting')
      sys.exit(1)

  if cats is None or len(cats)==0:
    logger.error('u_model: nothing to analyze, exiting')
    sys.exit(1)

  selected=list(range(0,len(cats))) # all  
  if pm.eq_mount:
    res=model.fit_model(cats=cats,mnts=mnts,selected=selected,obs=pm.obs)
  else:
    try:
      res=model.fit_model(cats=cats,mnts=mnts,selected=selected,fit_plus_poly=args.fit_plus_poly)
    except TypeError as e:
      ptfn=pm.expand_base_path(fn=args.analyzed_positions)
      logger.error('u_model: presumably empty file: {}, exception: {},exiting'.format(ptfn,e))
      sys.exit(1)
  stars=pm.prepare_plot(cats=cats,mnts=mnts,imgs=imgs,nmls=nmls,selected=selected,model=model)
    
  if args.plot:
    pm.plot_results(stars=stars,args=args)

  # for the moment
  sys.exit(1)
  selected,dropped=pm.select_stars(stars=stars)
  logger.info('number of selected: {}, dropped: {} '.format(len(selected),len(dropped)))

  if pm.eq_mount:
    res=model.fit_model(cats=cats,mnts=mnts,selected=selected,obs=pm.obs)
  else:
    res=model.fit_model(cats=cats,mnts=mnts,selected=selected,fit_plus_poly=args.fit_plus_poly)
    
  stars=pm.prepare_plot(cats=cats,mnts=mnts,nmls=nmls,selected=selected,model=model)
  pm.plot_results(stars=stars,args=args)
  
  logger.info('number of selected: {}, dropped: {} '.format(len(selected),len(dropped)))
  if pm.eq_mount:
    res=model.fit_model(cats=cats,mnts=mnts,selected=dropped,obs=pm.obs)
  else:
    res=model.fit_model(cats=cats,mnts=mnts,selected=dropped,fit_plus_poly=args.fit_plus_poly)
  
  stars=pm.prepare_plot(cats=cats,mnts=mnts,nmls=nmls,selected=dropped,model=model)
  pm.plot_results(stars=stars,args=args)
  sys.exit(0)

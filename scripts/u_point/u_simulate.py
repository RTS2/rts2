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
import os
import argparse
import logging
import importlib
import numpy as np
from random import uniform
from astropy import units as u
from astropy.time import Time
from astropy.coordinates import SkyCoord,EarthLocation
from astropy.coordinates import AltAz,ICRS
from astropy.coordinates import Longitude,Latitude

from u_point.structures import SkyPosition


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
  parser.add_argument('--base-path', dest='base_path', action='store', default='/tmp/u_point/',type=str, help=': %(default)s , directory where images are stored')
  parser.add_argument('--break_after', dest='break_after', action='store', default=10000000, type=int, help=': %(default)s, read max. positions, mostly used for debuging')
  parser.add_argument('--simulation-data', dest='simulation_data', action='store', default='./simulation_data.txt', help=': %(default)s,  data filename (output)')
  parser.add_argument('--lon-step', dest='lon_step', action='store', default=10, type=int,help=': %(default)s,  longitute step size: range(0,360,step) [deg]')
  parser.add_argument('--lat-step', dest='lat_step', action='store', default=10, type=int,help=': %(default)s, latitude step size: range(0,90,step) [deg]')
  parser.add_argument('--lat-minimum', dest='lat_minimum', action='store', default=30, type=int,help=': %(default)s, latitude minimum [deg]')
  parser.add_argument('--lat-maximum', dest='lat_maximum', action='store', default=80, type=int,help=': %(default)s, latitude maximum [deg]')
  #
  parser.add_argument('--obs-longitude', dest='obs_lng', action='store', default=123.2994166666666,type=arg_float, help=': %(default)s [deg], observatory longitude + to the East [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-latitude', dest='obs_lat', action='store', default=-75.1,type=arg_float, help=': %(default)s [deg], observatory latitude [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-height', dest='obs_height', action='store', default=3237.,type=arg_float, help=': %(default)s [m], observatory height above sea level [m], negative value: m10. equals to -10.')


  parser.add_argument('--model-class', dest='model_class', action='store', default='u_altaz', help=': %(default)s, specify your model, see e.g. model/u_altaz.py')
  parser.add_argument('--transform-class', dest='transform_class', action='store', default='u_astropy', help=': %(default)s, one of (u_sofa|u_astropy|u_libnova|u_pyephem)')
  # ToDo only astro.py supported refraction is built_in
  #parser.add_argument('--refraction-method', dest='refraction_method', action='store', default='built_in', help=': %(default)s, one of (bennett|saemundsson|stone), see refraction.py')
  #parser.add_argument('--refractive-index-method', dest='refractive_index_method', action='store', default='owens', help=': %(default)s, one of (owens|ciddor|edlen) if --refraction-method stone is specified, see refraction.py')
  group = parser.add_mutually_exclusive_group()
  # here: NO  nargs=1!!
  group.add_argument('--eq-params', dest='eq_params', default=[-60.,+12.,-60.,+60.,+30.,+12.,-12.,+60.,-60.], type=arg_floats, help=': %(default)s, EQ list of parameters [arcsec], format "p1 p2...p9"')
  group.add_argument('--aa-params', dest='aa_params', default=[-60.,+120.,-60.,+60.,-60.,+120.,-120.], type=arg_floats, help=': %(default)s, AltAz list of parameters [arcsec], format "p1 p2...p7"')
  parser.add_argument('--all-params-zero', dest='all_params_zero', action='store_true', default=False, help=': %(default)s, set: --eq-params,--aa-params are all set to 0.')
  #
  parser.add_argument('--utc', dest='utc', action='store', default='2016-10-08 05:00:01', type=str,help=': %(default)s, utc observing time, format [iso]')
  parser.add_argument('--sigma', dest='sigma', action='store', default=5., type=float,help=': %(default)s, gaussian noise on mount apparent position [arcsec]')
  parser.add_argument('--pressure-qfe', dest='pressure_qfe', action='store', default=636., type=float,help=': %(default)s, pressure QFE [hPa], not sea level')
  parser.add_argument('--temperature', dest='temperature', action='store', default=-60., type=float,help=': %(default)s, temperature [degC]')
  parser.add_argument('--humidity', dest='humidity', action='store', default=0.5, type=float,help=': %(default)s, humidity [0.,1.]')
  parser.add_argument('--exposure', dest='exposure', action='store', default=5.5, type=float,help=': %(default)s, CCD exposure time')
  parser.add_argument('--data-point', dest='data_points', action='store', default=1, type=int,help=': %(default)s, number of data points per grid point, specify a non zero value for --sigma')
            
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

  obs=EarthLocation(lon=float(args.obs_lng)*u.degree, lat=float(args.obs_lat)*u.degree, height=float(args.obs_height)*u.m)
  dt_utc = Time(args.utc,format='iso', scale='utc',location=obs)
  sigma= args.sigma/3600./180.*np.pi
  simulation_data=os.path.join(args.base_path,args.simulation_data)
  wfl=open(simulation_data, 'w')
  vn_lon=0.
  vn_lat=0.

  #rf_m=None
  #ri_m=None
  #if 'built_in' not in args.refraction_method:
  #  if 'ciddor' in  args.refractive_index_method or 'edlen' in  args.refractive_index_method:
  #    if import_message is not None:
  #      self.lg.error('u_analyze: {}'.format(import_message))
  #      sys.exit(1)
  #
  #  # refraction index method is set within Refraction
  #  rf=Refraction(lg=logger,obs=obs,refraction_method=args.refraction_method,refractive_index_method=args.refractive_index_method)
  #  rf_m=getattr(rf, 'refraction_'+args.refraction_method)
  #  logger.info('refraction method loaded: {}'.format(args.refraction_method))

  tf = importlib.import_module('transform.'+args.transform_class)
  logger.info('transformation loaded: {}'.format(args.transform_class))
  # ToDo
  #transform=tf.Transformation(lg=logger,obs=obs,refraction_method=rf_m)
  transform=tf.Transformation(lg=logger,obs=obs,refraction_method=None)
  if args.model_class in 'altaz':
    tr_t_tf=transform.transform_to_altaz
  else:
    tr_t_tf=transform.transform_to_hadec

  # load model class
  md = importlib.import_module('model.'+args.model_class)
  logger.info('model loaded: {}'.format(args.model_class))
  # required methods: fit_model, d_lon, d_lat
  if args.model_class in 'altaz':
    parameters=args.aa_params
    eq_mount=False # eq_mount=True, aa=False,
  else:
    parameters=args.eq_params
    eq_mount=True 

  if args.sigma == 0. and args.data_points > 1:
    logger.info('--sigma {}) and  --data-points {} do not make sense, exiting'.format(args.sigma, args.data_points))
    sys.exit(1)
    
  if args.all_params_zero:
    # quick and dirty
    parameters=[0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.]
  # ToDo very ugly
  if args.model_class in 'altaz':
    model=md.Model(lg=logger,parameters=parameters,fit_plus_poly=None, parameters_plus_poly=None)
  else:
    model=md.Model(lg=logger,parameters=parameters, obs_lat=obs.latitude.radian)
    
  # log inital settings
  model.log_parameters()
  
  # ToDo ugly
  meteo_only_sky=SkyPosition(
    pressure=args.pressure_qfe,
    temperature=args.temperature,
    humidity=args.humidity,
  )
  # ToDo toxic
  mnt_ra_rdb=SkyCoord(ra=0.,dec=0.,unit=(u.radian,u.radian),frame='icrs')
  mnt_aa_rdb=SkyCoord(az=0.,alt=0.,unit=(u.radian,u.radian),frame='altaz')

  #sys.exit(1)
  nml_id=0
  for az in range(0,360-args.lon_step,args.lon_step):
    for alt in range(args.lat_minimum,args.lat_maximum-args.lat_step,args.lat_step):
      r_lon=uniform(0, args.lon_step) 
      r_lat=uniform(0, args.lat_step)
      # nominal AltAz
      nml_aa=SkyCoord(az=az+r_lon,alt=alt+r_lat,unit=(u.deg,u.deg),frame='altaz',location=obs,obstime=dt_utc,pressure=0.*u.hPa,temperature=0.*u.deg_C,relative_humidity=0.)
      # nominal EQ
      cat_ic=nml_aa.transform_to(ICRS())
      # apparent AltAz (mnt), includes refraction etc.
      # only SOFA/ERFA built in refraction is supported
      #cat_ll_ap=cat_ic.transform_to(AltAz(pressure=args.pressure_qfe*u.hPa,temperature=args.temperature*u.deg_C,relative_humidity=args.humidity))
      # cat_ll_ap is either cat_aa or cat_ha
      cat_ll_ap=tr_t_tf(tf=cat_ic,sky=meteo_only_sky)
      if args.model_class in 'altaz':
        #
        # subtract the correction
        #                |
        #                |                                                     | 0. here
        vd_lon=Longitude(-model.d_lon(cat_ll_ap.az.radian,cat_ll_ap.alt.radian,0.),u.radian)
        vd_lat=Latitude(-model.d_lat(cat_ll_ap.az.radian,cat_ll_ap.alt.radian,0.),u.radian)
      else:
        vd_lon=Longitude(-model.d_lon(cat_ll_ap.ra.radian,cat_ll_ap.dec.radian,0.),u.radian)
        vd_lat=Latitude(-model.d_lat(cat_ll_ap.ra.radian,cat_ll_ap.dec.radian,0.),u.radian)
        
      for i in range(0,args.data_points):
        # noise
        if sigma!=0.:
          vn_lon=Longitude(np.random.normal(loc=0.,scale=sigma),u.radian)
          vn_lat=Latitude(np.random.normal(loc=0.,scale=sigma),u.radian)
          # mount apparent AltAz (HA/Dec is not required to find corrections for EQ mount)
          
        if args.model_class in 'altaz':
          mnt_ll=SkyCoord(az=cat_ll_ap.az+vd_lon+vn_lon,alt=cat_ll_ap.alt+vd_lat+vn_lat,unit=(u.radian,u.radian),frame='altaz',location=obs,obstime=dt_utc)
        else:
          # it is HA!
          mnt_ll=SkyCoord(ra=cat_ll_ap.ra+vd_lon+vn_lon,dec=cat_ll_ap.dec+vd_lat+vn_lat,unit=(u.radian,u.radian),frame='gcrs',location=obs,obstime=dt_utc)
          
        s_sky=SkyPosition(
          nml_id=nml_id, 
          cat_no=None,
          nml_aa=nml_aa,
          cat_ic=cat_ic,
          dt_begin=dt_utc,
          dt_end=dt_utc,
          dt_end_query=dt_utc,
          JD=dt_utc.jd,
          mnt_ra_rdb=mnt_ra_rdb,
          mnt_aa_rdb=mnt_aa_rdb, 
          image_fn=None,
          exp=args.exposure,
          pressure=args.pressure_qfe,
          temperature=args.temperature,
          humidity=args.humidity,
          eq_mount=eq_mount,
          transform_name='u_astropy',
          # ToDo
          refraction_method='built_in', 
          cat_ll_ap=cat_ll_ap,
          mnt_ll_sxtr=mnt_ll, 
          mnt_ll_astr=mnt_ll, 
        )
        nml_id +=1
        wfl.write('{0}\n'.format(s_sky))
        
  wfl.close()

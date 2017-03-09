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
  parser.add_argument('--break_after', dest='break_after', action='store', default=10000000, type=int, help=': %(default)s, read max. positions, mostly used for debuging')
  parser.add_argument('--simulation-data', dest='simulation_data', action='store', default='./simulation_data.txt', help=': %(default)s,  data filename (output)')
  parser.add_argument('--step', dest='step', action='store', default=10, type=int,help=': %(default)s, AltAz points: range(0,360,step), range(0,90,step) [deg]')
  #
  parser.add_argument('--obs-longitude', dest='obs_lng', action='store', default=123.2994166666666,type=arg_float, help=': %(default)s [deg], observatory longitude + to the East [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-latitude', dest='obs_lat', action='store', default=-75.1,type=arg_float, help=': %(default)s [deg], observatory latitude [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-height', dest='obs_height', action='store', default=3237.,type=arg_float, help=': %(default)s [m], observatory height above sea level [m], negative value: m10. equals to -10.')


  parser.add_argument('--model-class', dest='model_class', action='store', default='altaz', help=': %(default)s, specify your model, see e.g. model/altaz.py')
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
            
  args=parser.parse_args()
  
  if args.toconsole:
    args.level='DEBUG'

  filename='/tmp/{}.log'.format(sys.argv[0].replace('.py','')) # ToDo datetime, name of the script
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
  wfl=open(args.simulation_data, 'w')
  vn_lon=0.
  vn_lat=0.

  # now load model class
  md = importlib.import_module('model.'+args.model_class)
  logger.info('model loaded: {}'.format(args.model_class))
  # required methods: fit_model, d_lon, d_lat
  if args.model_class in 'altaz':
    parameters=args.aa_params
  else:
    parameters=args.eq_params

  if args.all_params_zero:
    # quick and dirty
    parameters=[0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.]

  model=md.Model(lg=logger,parameters=parameters,fit_plus_poly=None, parameters_plus_poly=None)  
  model.log_parameters()
  

  nml_id=0
  for az in range(0,360-args.step,args.step):
    for alt in range(30,90-args.step,args.step):
      r_lon=uniform(0, args.step) 
      r_lat=uniform(0, args.step)
      # nominal AltAz
      nml_aa=SkyCoord(az=az+r_lon,alt=alt+r_lat,unit=(u.deg,u.deg),frame='altaz',location=obs,obstime=dt_utc,pressure=0.*u.hPa,temperature=0.*u.deg_C,relative_humidity=0.)
      # nominal EQ
      cat_ic=nml_aa.transform_to(ICRS())
      # apparent AltAz (mnt), includes refraction etc.
      cat_ll_ap=cat_ic.transform_to(AltAz(pressure=args.pressure_qfe*u.hPa,temperature=args.temperature*u.deg_C,relative_humidity=args.humidity))
      #
      # subtract the correction
      #               |
      #               |                                                   | 0. here
      vd_lon=Longitude(-model.d_lon(cat_ll_ap.az.radian,cat_ll_ap.alt.radian,0.),u.radian)
      vd_lat=Latitude(-model.d_lat(cat_ll_ap.az.radian,cat_ll_ap.alt.radian,0.),u.radian)
      # noise
      if sigma!=0.:
        vn_lon=Longitude(np.random.normal(loc=0.,scale=sigma),u.radian)
        vn_lat=Latitude(np.random.normal(loc=0.,scale=sigma),u.radian)
      # mount apparent AltAz
      mnt_ll=SkyCoord(az=cat_ll_ap.az+vd_lon+vn_lon,alt=cat_ll_ap.alt+vd_lat+vn_lat,unit=(u.radian,u.radian),frame='altaz',location=obs,obstime=dt_utc)

      s_sky=SkyPosition(
        nml_id=nml_id, 
        cat_no=None,
        nml_aa=nml_aa,
        cat_ic=cat_ic,
        dt_begin=dt_utc,
        dt_end=dt_utc,
        dt_end_query=dt_utc,
        JD=dt_utc.jd,
        mnt_ra_rdb=cat_ic,
        mnt_aa_rdb=cat_ll_ap, 
        image_fn=None,
        exp=args.exposure,
        pressure=args.pressure_qfe,
        temperature=args.temperature,
        humidity=args.humidity,
        mount_type_eq=False, # mount_type_eq=True, aa=False,
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

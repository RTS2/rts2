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

Acquire sky positions for a pointing model

'''

__author__ = 'wildi.markus@bluewin.ch'

import sys,os
import argparse
import logging
import numpy as np
from random import randrange, uniform

from datetime import datetime
from astropy import units as u
from astropy.time import Time,TimeDelta
from astropy.coordinates import SkyCoord,EarthLocation
from astropy.coordinates import AltAz,CIRS,ITRS
from astropy.coordinates import Longitude,Latitude,Angle
import astropy.coordinates as coord

# python 3 version
import scriptcomm
r2c= scriptcomm.Rts2Comm()

class CatPosition(object):
  def __init__(self, cat_eq=None,mag_v=None):
    self.cat_eq=cat_eq 
    self.mag_v=mag_v
# ToDo may be only a helper
class NmlPosition(object):
  def __init__(self, acq_aa_nml=None):
    self.acq_aa_nml=acq_aa_nml # nominal position (grid created with store_nominal_altaz())

class AcqPosition(object):
  def __init__(self, acq_aa_nml=None,acq_aa=None,cnt=None):
    self.acq_aa_nml=acq_aa_nml # nominal position (grid created with store_nominal_altaz())
    self.acq_aa=acq_aa # list of acquired star positions
    self.cnt=cnt
    
class Acquisition(object):
  def __init__(self, dbg=None,lg=None, obs_lng=None, obs_lat=None, obs_height=None,break_after=None):
    self.dbg=dbg
    self.lg=lg
    self.break_after=break_after
    #
    self.obs=EarthLocation(lon=float(obs_lng)*u.degree, lat=float(obs_lat)*u.degree, height=float(obs_height)*u.m)
    self.dt_utc = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date')
    self.cat=list()
    self.nml=list()
    self.acq=list()
    
  def now_observable(self,cat_eq=None, altitude_interval=None):
    cat_aa=cat_eq.transform_to(AltAz(location=self.obs, pressure=0.)) # no refraction here, UTC is in cat_eq
    if altitude_interval[0]<cat_aa.alt.radian<altitude_interval[1]:
      return cat_aa
    else:
      return None

  def to_altaz(self,cat_eq=None):
    return cat_eq.transform_to(AltAz(location=self.obs, pressure=0.)) # no refraction here, UTC is in cat_eq

  def fetch_observable_catalog(self, ptfn=None):
    lns=list()
    with  open(ptfn, 'r') as lfl:
      lns=lfl.readlines()

    for i,ln in enumerate(lns):
      if i > self.break_after:
        break
      try:
        ras,decs,mag_vs=ln.split(',')
        ra=float(ras)
        dec=float(decs)
        mag_v=float(mag_vs)
      except ValueError:
        self.lg.warn('value error on line: {}'.format(ln[:-1]))
        continue
      except Exception as e:
        self.lg.error('error on line: {}'.format(e))
        sys.exit(1)
      #not yet if brightness[0]<mag_v<brighness[1]:
      cat_eq=SkyCoord(ra=ra,dec=dec, unit=(u.radian,u.radian), frame='icrs',obstime=self.dt_utc,location=self.obs)
      cat=CatPosition(cat_eq=cat_eq,mag_v=mag_v) # other members will be defined later
      self.cat.append(cat)
      
###        if self.now_observable(cat_eq=cat_eq,altitude_interval=altitude_interval):

  def store_nominal_altaz(self,step=None,azimuth_interval=None,altitude_interval=None,ptfn=None):
    # ToDo from pathlib import Path, fp=Path(ptfb),if fp.is_file())
    # format az_nml,alt_nml
    if os.path.isfile(ptfn):
      a=input('overwriting existing file: {} [N/y]'.format(ptfn))
      if a not in 'y':
        self.lg.info('exiting')
        sys.exit(0)

    with  open(ptfn, 'w') as wfl:
      # ToDo input as int?
      for az in range(int(azimuth_interval[0]),int(azimuth_interval[1]),args.step):
        for alt in range(int(altitude_interval[0]),int(altitude_interval[1]),step):
          wfl.write('{},{}\n'.format(az/180.*np.pi,alt/180.*np.pi))

  def fetch_nominal_altaz(ptfn=None):
    #self.nml=list()
    pass

  def drop_nominal_altaz(self):
    # comparison self.nml, self.acq
    # compare and drop 
    pass
  
  def fetch_acquired_altaz(self,ptfn=None):
    # format: az_nml,alt_nml,dt_utc,cat_eq,[acq_eq]
    lns=list()
    with  open(ptfn, 'r') as rfl:
      lns=rfl.readlines()

    for i,ln in enumerate(lns):
      try:
        azs,alts,cnts=ln.split(',')
        az=float(azs)
        alt=float(alts)
        cnt=int(cnts)
      except ValueError:
        self.lg.warn('value error on line: {}'.format(ln[:-1]))
        continue
      except Exception as e:
        self.lg.error('error on line: {}'.format(e))
        sys.exit(1)
      
      acq_aa_nml=SkyCoord(az=az,alt=alt,unit=(u.radian,u.radian),frame='altaz',location=self.obs)
      # ToDo test only
      acq_aa=None
      dc=randrange(0, 3)
      if dc>=2:
        acq_aa=SkyCoord(az=az+uniform(-0.05,.05),alt=alt+uniform(-0.05,.05),unit=(u.radian,u.radian),frame='altaz',location=self.obs)
      acq=AcqPosition(acq_aa=acq_aa,acq_aa_nml=acq_aa_nml,cnt=cnt)# for the sake of clarity
      self.acq.append(acq)
  
  def store_acquired_altaz(self,cat_aa=None,ptfn=None):
    # format: az_nml,alt_nml,dt_utc,cat_eq,[acq_eq]
    # append, one by one
    with  open(ptfn, 'a') as wfl:
      for i,o in enumerate(self.cat):
        wfl.write('{},{},{}\n'.format(o.cat_aa_nml.az.radian,cat_aa_nml.alt.radian,o.cnt))

  def plot(self,title=None): #AltAz 
    acq_az = coord.Angle([x.acq_aa.az.radian for x in self.acq if x.acq_aa is not None], unit=u.radian)
    acq_az = acq_az.wrap_at(180*u.degree)
    acq_alt = coord.Angle([x.acq_aa.alt.radian for x in self.acq if x.acq_aa is not None],unit=u.radian)
      
    acq_nml_az = coord.Angle([x.acq_aa_nml.az.radian for x in self.acq], unit=u.radian)
    acq_nml_az = acq_nml_az.wrap_at(180*u.degree)
    acq_nml_alt = coord.Angle([x.acq_aa_nml.alt.radian for x in self.acq],unit=u.radian)

    import matplotlib
    # this varies from distro to distro:
    matplotlib.rcParams["backend"] = "TkAgg"
    import matplotlib.pyplot as plt
    plt.ioff()
    fig = plt.figure(figsize=(8,6))
    ax = fig.add_subplot(111, projection="polar")
    ax.set_title(title)
    # eye candy
    #npa=np.asarray([np.exp(x)/10. for x in self.dt['mag_v']])
    ax.scatter(acq_nml_az, acq_nml_alt,color='red')#,s=npa)
    ax.scatter(acq_az, acq_alt,color='blue')#,s=npa)
    #ax.scatter()#,s=npa)

    ax.grid(True)
    plt.show()


  def acquire(self,ptfn=None,bgrightness=None,altitude_interval=None,azimuth_interval=None):
    pass
  
  
# really ugly!
def arg_floats(value):
  return list(map(float, value.split()))

def arg_float(value):
  if 'm' in value:
    return -float(value[1:])
  else:
    return float(value)

if __name__ == "__main__":

  parser= argparse.ArgumentParser(prog=sys.argv[0], description='Acquire not yet observed positions')
  parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
  parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
  parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
  parser.add_argument('--break_after', dest='break_after', action='store', default=10000000, type=int, help=': %(default)s, read max. positions, mostly used for debuging')

  parser.add_argument('--obs-longitude', dest='obs_lng', action='store', default=123.2994166666666,type=arg_float, help=': %(default)s [deg], observatory longitude + to the East [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-latitude', dest='obs_lat', action='store', default=-75.1,type=arg_float, help=': %(default)s [deg], observatory latitude [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-height', dest='obs_height', action='store', default=3237.,type=arg_float, help=': %(default)s [m], observatory height above sea level [m], negative value: m10. equals to -10.')
  parser.add_argument('--plot', dest='plot', action='store_true', default=False, help=': %(default)s, plot results')
  parser.add_argument('--brightness-interval', dest='brightness_interval', default=[0.,7.0], type=arg_floats, help=': %(default)s, visual star brightness [mag], format "p1 p2"')
  parser.add_argument('--altitude-interval',   dest='altitude_interval',   default=[10.,80.],type=arg_floats,help=': %(default)s,  allowed altitude [deg], format "p1 p2"')
  parser.add_argument('--azimuth-interval',   dest='azimuth_interval',   default=[0.,360.],type=arg_floats,help=': %(default)s,  allowed azimuth [deg], format "p1 p2"')
  parser.add_argument('--observable-catalog', dest='observable_catalog', action='store', default='observable.cat', help=': %(default)s, retrieve the observable objects')
  parser.add_argument('--acquired-positions', dest='acquired_positions', action='store', default='acquired_positions.cat', help=': %(default)s, already observed positions (AltAz coordinates)')
  parser.add_argument('--create-plan', dest='create_plan', action='store_true', default=False, help=': %(default)s, create positions to be observed, attention: file specified with --observed-positions')
  parser.add_argument('--step', dest='step', action='store', default=10, type=int,help=': %(default)s, AltAz points: step is used as: range(0,360,step), range(0,90,step) [deg]')
  parser.add_argument('--now-observable', dest='now_observable', action='store_true', default=False, help=': %(default)s, created positions are now above horizon')
    
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

  altitude_interval=list()
  for v in args.altitude_interval:
    altitude_interval.append(v/180.*np.pi)

  ac= Acquisition(dbg=args.debug,lg=logger,obs_lng=args.obs_lng,obs_lat=args.obs_lat,obs_height=args.obs_height,break_after=args.break_after)

  if args.create_plan:
    ac.store_nominal_altaz(step=args.step,azimuth_interval=args.azimuth_interval,altitude_interval=args.altitude_interval,ptfn=args.acquired_positions)
    sys.exit(1)
  # the book keeping
  ac.fetch_acquired_altaz(ptfn=args.acquired_positions)
  # candidate objects, predefined with u_select.py
  ac.fetch_observable_catalog(ptfn=args.observable_catalog)

  ac.plot(title='progress report') #AltAz 

  #ac.acquire(ptfn=args.acquired_positions,bgrightness=args.brightness_interval,altitude_interval=altitude_interval,azimuth_interval=azimuth_interval)


  if args.plot:
    ct.plot()

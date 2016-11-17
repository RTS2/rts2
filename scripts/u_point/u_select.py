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
Catalog of visible bright and lonely stars 

Yale Bright Star Catalog

http://tdc-www.harvard.edu/catalogs/bsc5.html

'''

__author__ = 'wildi.markus@bluewin.ch'

import sys
import os
import argparse
import logging
import numpy as np

from datetime import datetime
from astropy import units as u
from astropy.time import Time,TimeDelta
from astropy.coordinates import SkyCoord,EarthLocation
from astropy.coordinates import AltAz,CIRS,ITRS
from astropy.coordinates import Longitude,Latitude,Angle
import astropy.coordinates as coord

from structures import CatPosition

class Catalog(object):
  def __init__(self, dbg=None,lg=None, obs_lng=None, obs_lat=None, obs_height=None,break_after=None,base_path=None):
    self.dbg=dbg
    self.lg=lg
    self.break_after=break_after
    self.base_path=base_path
    #
    self.obs=EarthLocation(lon=float(obs_lng)*u.degree, lat=float(obs_lat)*u.degree, height=float(obs_height)*u.m)
    self.dt_utc = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date')

  def observable(self,cat_eq=None):
    if self.obs.latitude.radian >= 0.:
      if -(np.pi/2.-self.obs.latitude.radian) < cat_eq.dec.radian:
        return True
    else:
      if -(-np.pi/2.-self.obs.latitude.radian) > cat_eq.dec.radian:
        return True

    return False
    
  def fetch_catalog(self, ptfn=None,bgrightness=None):
    if self.base_path in ptfn:
      fn=ptfn
    else:
      fn=os.path.join(self.base_path,ptfn)

    lns=list()
    with  open(fn, 'r') as lfl:
      lns=lfl.readlines()

    cats=list()
    for i,ln in enumerate(lns):
      if i > self.break_after:
        break
      try:
        cat_no=int(ln[0:4])
        ra_h=int(ln[75:77])
        ra_m=int(ln[77:79])
        ra_s=float(ln[79:83])
        self.lg.debug('{0:4d} RA: {1}:{2}:{3}'.format(i,ra_h,ra_m,ra_s)) 
        dec_n=ln[83:84]
        dec_d=int(ln[84:86])
        dec_m=int(ln[86:88])
        dec_s=float(ln[89:90])
        self.lg.debug('{0:4d} DC: {1}{2}:{3}:{4}'.format(i,dec_n,dec_d,dec_m,dec_s)) 
        mag_v=float(ln[102:107])
      except ValueError:
        self.lg.warn('fetch_catalog: value error on line: {}, {}, {}'.format(i,ln[:-1],ptfn))
        continue
      except Exception as e:
        self.lg.error('fetch_catalog: error on line: {}, {}, {}'.format(i,e,ptfn))
        sys.exit(1)
        
      if bgrightness[0]<mag_v<bgrightness[1]:
        ra='{} {} {} hours'.format(ra_h,ra_m,ra_s)
        dec='{}{} {} {} '.format(dec_n,dec_d,dec_m,dec_s)
        cat_eq=SkyCoord(ra=ra,dec=dec, unit=(u.hour,u.deg), frame='icrs',obstime=self.dt_utc,location=self.obs)

        if self.observable(cat_eq=cat_eq):
          cat=CatPosition(cat_no=cat_no,cat_eq=cat_eq,mag_v=mag_v)
          cats.append(cat)
    # prepare for u_acquire.py
    self.cats=sorted(cats, key=lambda x: x.cat_eq.ra.radian)
    
  def remove_objects(self,min_sep=None):
    # remove too close objects
    rng=range(len(self.cats)-1,0,-1) # self.cats is ra sorted
    min_sep_r= min_sep/180.*np.pi
    to_delete=list()
    for i in rng:
      if i == 0:
        break
      # lazy
      if abs(self.cats[i].cat_eq.ra.radian-self.cats[i-1].cat_eq.ra.radian) < min_sep_r:
        if abs(self.cats[i].cat_eq.dec.radian-self.cats[i-1].cat_eq.dec.radian) < min_sep_r:
          to_delete.append(i)
          to_delete.append(i-1)
  
    dlt=sorted(set(to_delete), reverse=True)
    self.lg.info('number of objects too close: {}'.format(len(dlt)))
    for i in dlt:
      del self.cats[i]
    self.lg.info('number of remaining objects: {}'.format(len(self.cats)))
          
  def store_observable_catalog(self,ptfn=None):
    if self.base_path in ptfn:
      fn=ptfn
    else:
      fn=os.path.join(self.base_path,ptfn)
    with  open(fn, 'w') as wfl:
      for i,o in enumerate(self.cats):
        wfl.write('{0},{1},{2},{3}\n'.format(o.cat_no,o.cat_eq.ra.radian,o.cat_eq.dec.radian,o.mag_v))

  def plot(self):
    ra = coord.Angle([x.cat_eq.ra.radian for x in self.cats], unit=u.radian)
    ra = ra.wrap_at(180*u.degree)
    dec = coord.Angle([x.cat_eq.dec.radian for x in self.cats],unit=u.radian)

    import matplotlib.mlab as mlab
    import matplotlib
    # this varies from distro to distro:
    matplotlib.rcParams["backend"] = "TkAgg"
    import matplotlib.mlab as mlab
    import matplotlib.pyplot as plt
    plt.ioff()
    fig = plt.figure(figsize=(8,6))
    ax = fig.add_subplot(111, projection="mollweide")
    # eye candy
    npa=np.asarray([np.exp(x.mag_v)/80. for x in self.cats])
    
    ax.set_title('selected stars for observatory latitude: {0:.2f} deg'.format(self.obs.latitude.degree))
    ax.scatter(ra.radian, dec.radian,s=npa)
    ax.set_xticklabels(['14h','16h','18h','20h','22h','0h','2h','4h','6h','8h','10h'])
    ax.grid(True)
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

  parser= argparse.ArgumentParser(prog=sys.argv[0], description='Select objects for pointing model observations')
  parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
  parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
  parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
  parser.add_argument('--break_after', dest='break_after', action='store', default=10000000, type=int, help=': %(default)s, read max. positions, mostly used for debuging')

  parser.add_argument('--obs-longitude', dest='obs_lng', action='store', default=123.2994166666666,type=arg_float, help=': %(default)s [deg], observatory longitude + to the East [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-latitude', dest='obs_lat', action='store', default=-75.1,type=arg_float, help=': %(default)s [deg], observatory latitude [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-height', dest='obs_height', action='store', default=3237.,type=arg_float, help=': %(default)s [m], observatory height above sea level [m], negative value: m10. equals to -10.')
  parser.add_argument('--yale-catalog', dest='yale_catalog', action='store', default='/usr/share/stardata/yale/catalog.dat', help=': %(default)s, Ubuntu apt install yale')
  parser.add_argument('--plot', dest='plot', action='store_true', default=False, help=': %(default)s, plot results')
  parser.add_argument('--brightness-interval', dest='brightness_interval', default=[0.,7.0], type=arg_floats, help=': %(default)s, visual star brightness [mag], format "p1 p2"')
  parser.add_argument('--observable-catalog', dest='observable_catalog', action='store', default='observable.cat', help=': %(default)s, store the  observable objects')
  parser.add_argument('--minimum-separation', dest='minimum_separation', action='store', default=1.,type=arg_float, help=': %(default)s [deg], minimum separation between catalog stars')
  parser.add_argument('--base-path', dest='base_path', action='store', default='./u_point_data/',type=str, help=': %(default)s , directory where images are stored')
  
  args=parser.parse_args()
  
  filename='/tmp/{}.log'.format(sys.argv[0].replace('.py','')) # ToDo datetime, name of the script
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

  ct= Catalog(dbg=args.debug,lg=logger,obs_lng=args.obs_lng,obs_lat=args.obs_lat,obs_height=args.obs_height,break_after=args.break_after,base_path=args.base_path)
  ct.fetch_catalog(ptfn=args.yale_catalog, bgrightness=args.brightness_interval)

  ct.remove_objects(min_sep=args.minimum_separation)
  
  ct.store_observable_catalog(ptfn=args.observable_catalog)
  if args.plot:
    ct.plot()

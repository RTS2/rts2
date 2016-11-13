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

Determine sky positions for a pointing model, SExtractor, astrometry.net

'''

__author__ = 'wildi.markus@bluewin.ch'

import sys,os
import argparse
import logging
import socket
import numpy as np
import requests
import pandas as pd

from multiprocessing import Lock, Process, Queue, current_process, cpu_count, Event

from collections import OrderedDict
from datetime import datetime
from astropy import units as u
from astropy.time import Time,TimeDelta
from astropy.coordinates import SkyCoord,EarthLocation
from astropy.coordinates import AltAz,CIRS,ITRS,ICRS
from astropy.coordinates import Longitude,Latitude,Angle
import astropy.coordinates as coord


import ds9region
import sextractor_3
import astrometry_3
from position import CatPosition,NmlPosition,AcqPosition,AnlPosition,cl_nms_acq,cl_nms_anl


class Worker(Process):
  def __init__(self, work_queue=None,ds9_queue=None,next_queue=None,lock=None, dbg=None, lg=None, anl=None):
    Process.__init__(self)
    self.exit=Event()
    self.work_queue=work_queue
    self.ds9_queue=ds9_queue
    self.next_queue=next_queue
    self.anl=anl
    self.lock=lock
    self.dbg=dbg
    self.lg=lg

  def store_analyzed_position(self,acq=None,sxtr_ra=None,sxtr_dec=None,astr_mnt_eq=None):
    lock.acquire()
    if astr_mnt_eq is None and sxtr_ra is None and sxtr_dec is None:
      pass
    elif astr_mnt_eq is None:
      self.anl.store_analyzed_position(acq=acq,sxtr_ra=sxtr_ra,sxtr_dec=sxtr_dec,astr_ra=np.nan,astr_dec=np.nan)
    else:
      self.anl.store_analyzed_position(acq=acq,sxtr_ra=sxtr_ra,sxtr_dec=sxtr_dec,astr_ra=astr_mnt_eq.ra.radian,astr_dec=astr_mnt_eq.dec.radian)
    lock.release()

  def run(self):
    acq=acq_image_fn=None
    while not self.exit.is_set():
      if ds9_queue is not None:
        try:
          cmd=self.ds9_queue.get()
        except Queue.Empty:
          self.lg.info('{}: ds9 queue empty, returning'.format(current_process().name))
          return
        # 'q'
        if isinstance(cmd, str):
          print('cmd: {}'.format(cmd))
          if 'dl' in cmd:
            self.lg.info('{}: got {}, delete last: {}'.format(current_process().name, cmd,acq.image_fn))
            acq_image_fn=acq.image_fn
            acq=None
          elif 'q' in cmd:
            self.store_analyzed_position(acq=acq,sxtr_ra=sxtr_ra,sxtr_dec=sxtr_dec,astr_mnt_eq=astr_mnt_eq)
            self.lg.error('{}: got {}, call shutdown'.format(current_process().name, cmd))
            self.shutdown()
            return
          else:
            self.lg.error('{}: got {}, continue'.format(current_process().name, cmd))
        if acq:
          self.store_analyzed_position(acq=acq,sxtr_ra=sxtr_ra,sxtr_dec=sxtr_dec,astr_mnt_eq=astr_mnt_eq)
        elif acq_image_fn is not None:
          self.lg.info('{}: not storing {}'.format(current_process().name, acq_image_fn))
          acq_image_fn=None
      acq=None
      try:
        acq=self.work_queue.get()
      except Queue.Empty:
        self.lg.info('{}: queue empty, returning'.format(current_process().name))
        return
      except Exception as  e:
        self.lg.error('{}: queue error: {}, returning'.format(current_process().name, e))
        return
      # 'STOP'
      if isinstance(acq, str):
        self.lg.error('{}: got {}, call shutdown_on_STOP'.format(current_process().name, acq))
        self.shutdown_on_STOP()
        return
      # analysis part
      x,y=self.anl.sextract(acq=acq,pcn=current_process().name)
      sxtr_ra=sxtr_dec=None
      if x is not None and y is not None:
        sxtr_ra,sxtr_dec=self.anl.xy2lonlat_appr(px=x,py=y,ln0=acq.eq.ra.radian,lt0=acq.eq.dec.radian,acq=acq,pcn=current_process().name)
      
      astr_mnt_eq=self.anl.astrometry(acq=acq,pcn=current_process().name)
      if self.ds9_queue is None:
        self.store_analyzed_position(acq=acq,sxtr_ra=sxtr_ra,sxtr_dec=sxtr_dec,astr_mnt_eq=astr_mnt_eq)
      else:
        self.next_queue.put('c')
      # end analysis part

    self.lg.info('{}: got shutdown event, exiting'.format(current_process().name))

  def shutdown(self):
    if next_queue is not None:
      self.next_queue.put('c')
    self.lg.info('{}: shutdown event, initiate exit'.format(current_process().name))
    self.exit.set()
    return

  def shutdown_on_STOP(self):
    if next_queue is not None:
      self.next_queue.put('c')
    self.lg.info('{}: shutdown on STOP, initiate exit'.format(current_process().name))
    self.exit.set()
    return
                                                        
class SolverResult():
  """Results of astrometry.net including necessary fits headers"""
  def __init__(self, ra=None, dec=None, fn=None):
    self.ra= ra
    self.dec=dec
    self.fn= fn

class SolveField():
  """Solve a field with astrometry.net """
  def __init__(self, lg=None, fn=None, blind=False, ra=None, dec=None, scale=None, radius=None, replace=None, verbose=None):
    self.lg = lg
    self.fn= fn
    self.blind= blind
    self.ra= ra
    self.dec= dec
    self.scale  = scale
    self.radius = radius
    self.replace= replace
    self.verbose= verbose

  def solveField(self):
    try:
      self.solver = astrometry_3.AstrometryScript(lg=self.lg,fits_file=self.fn)
    except Exception as e:
      self.lg.debug('SolveField: solver died, file: {}, exception: {}'.format(self.fn, e))
      return None
      
    if self.blind:
      center=self.solver.run(scale=self.scale, replace=self.replace,verbose=self.verbose)
    else:
      # ToDo
      center=self.solver.run(scale=self.scale,ra=self.ra,dec=self.dec,radius=self.radius,replace=self.replace,verbose=self.verbose)

      if center!=None:
        if len(center)==2:
          return SolverResult(ra=center[0],dec=center[1],fn=self.fn)
      return None

class Analysis(object):
  def __init__(
      self, dbg=None,
      lg=None,
      obs_lng=None,
      obs_lat=None,
      obs_height=None,
      px_scale=None,
      radius=None,
      ccd_size=None,
      acquired_positions=None,
      analyzed_positions=None,
      u_point_positions_base=None,
      break_after=None,
      do_not_use_astrometry=None,
      verbose_astrometry=None,
      ds9_display=None,
      base_path=None,

  ):
    self.dbg=dbg
    self.lg=lg
    self.break_after=break_after
    self.do_not_use_astrometry=do_not_use_astrometry
    self.verbose_astrometry=verbose_astrometry
    self.ds9_display=ds9_display
    self.px_scale=px_scale
    self.radius=radius
    self.ccd_size=ccd_size
    self.base_path=base_path
    #
    self.acquired_positions=acquired_positions
    self.analyzed_positions=analyzed_positions
    self.u_point_positions_base=u_point_positions_base
    self.obs=EarthLocation(lon=float(obs_lng)*u.degree, lat=float(obs_lat)*u.degree, height=float(obs_height)*u.m)
    self.dt_utc=Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date')
    self.acq=list()
    self.anl=list()

  def fetch_pandas(self, ptfn=None,columns=None,sys_exit=True):
    pd_cat=None
    if not os.path.isfile(ptfn):
      self.lg.debug('fetch_pandas:{} does not exist, exiting'.format(ptfn))
      if sys_exit:
        sys.exit(1)
      return None

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
    pd_cat.columns = columns
    return pd_cat

  def fetch_positions(self,fn=None,fetch_acq=False,sys_exit=False):
    cln= cl_nms_anl
    if fetch_acq:
      cln= cl_nms_acq
    if self.base_path in fn:
      ptfn=fn
    else:
      ptfn=os.path.join(self.base_path,fn)
      
    pd_cat = self.fetch_pandas(ptfn=ptfn,columns=cln,sys_exit=sys_exit)
    if pd_cat is None:
      return
      
    for i,rw in pd_cat.iterrows():
      # ToDo why not out_subfmt='fits'
      dt_begin=Time(rw['dt_begin'],format='iso', scale='utc',location=self.obs,out_subfmt='date_hms')
      dt_end=Time(rw['dt_end'],format='iso', scale='utc',location=self.obs,out_subfmt='date_hms')
      dt_end_query=Time(rw['dt_end_query'],format='iso', scale='utc',location=self.obs,out_subfmt='date_hms')

      # ToDo set best time point
      aa_nml=SkyCoord(az=rw['aa_nml_az'],alt=rw['aa_nml_alt'],unit=(u.radian,u.radian),frame='altaz',location=self.obs,obstime=dt_end)
      acq_eq=SkyCoord(ra=rw['eq_ra'],dec=rw['eq_dec'], unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
      acq_eq_woffs=SkyCoord(ra=rw['eq_woffs_ra'],dec=rw['eq_woffs_dec'], unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
      acq_eq_mnt=SkyCoord(ra=rw['eq_mnt_ra'],dec=rw['eq_mnt_dec'], unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
      acq_aa_mnt=SkyCoord(az=rw['aa_mnt_az'],alt=rw['aa_mnt_alt'],unit=(u.radian,u.radian),frame='altaz',location=self.obs,obstime=dt_end)

      if fetch_acq:
        acq=AcqPosition(
          nml_id=rw['nml_id'],
          cat_no=rw['cat_no'],
          aa_nml=aa_nml,
          eq=acq_eq,
          dt_begin=dt_begin,
          dt_end=dt_end,
          dt_end_query=dt_end_query,
          JD=rw['JD'],
          eq_woffs=acq_eq_woffs,
          eq_mnt=acq_eq_mnt,
          aa_mnt=acq_aa_mnt,
          image_fn=rw['image_fn'],
          exp=rw['exp'],
          pressure=rw['pressure'],
          temperature=rw['temperature'],
          humidity=rw['humidity'],
        )
        self.acq.append(acq)
      else:
        if pd.notnull(rw['sxtr_ra']) and pd.notnull(rw['sxtr_dec']):
          sxtr_eq=SkyCoord(ra=rw['sxtr_ra'],dec=rw['sxtr_dec'], unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
        else:
          sxtr_eq=None
          self.lg.debug('fetch_positions: sxtr None')
        if pd.notnull(rw['astr_ra']) and pd.notnull(rw['astr_dec']):
          astr_eq=SkyCoord(ra=rw['astr_ra'],dec=rw['astr_dec'], unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
        else:
          astr_eq=None
          self.lg.debug('fetch_positions: astr None')

        anls=AnlPosition(
          nml_id=rw['nml_id'],
          cat_no=rw['cat_no'],
          aa_nml=aa_nml,
          eq=acq_eq,
          dt_begin=dt_begin,
          dt_end=dt_end,
          dt_end_query=dt_end_query,
          JD=rw['JD'],
          eq_woffs=acq_eq_woffs,
          eq_mnt=acq_eq_mnt,
          aa_mnt=acq_aa_mnt,
          image_fn=rw['image_fn'],
          exp=rw['exp'],
          pressure=rw['pressure'],
          temperature=rw['temperature'],
          humidity=rw['humidity'],
          sxtr= sxtr_eq,
          astr= astr_eq,
        )
        self.anl.append(anls)
      
  def store_u_point(self,fn=None,acq=None,ra=None,dec=None):
    if self.base_path in fn:
      ptfn=fn
    else:
      ptfn=os.path.join(self.base_path,fn)

    with  open(ptfn, 'a') as wfl:
      wfl.write('{0},{1},{2},{3},{4},{5},{6},{7},{8}\n'.format(
        acq.dt_end,#1
        acq.eq.ra.radian,
        acq.eq.dec.radian,
        ra,
        dec,
        acq.exp,
        acq.pressure[0], # ToDo why tupple
        acq.temperature[0],
        acq.humidity[0],#9
      ))

  def store_analyzed_position(self,acq=None,sxtr_ra=None,sxtr_dec=None,astr_ra=None,astr_dec=None):
    if self.base_path in self.analyzed_positions:
      ptfn=self.analyzed_positions
    else:
      ptfn=os.path.join(self.base_path,self.analyzed_positions)
    
    # append, one by one
    with  open(ptfn, 'a') as wfl:
      wfl.write('{},{},{},{},{}\n'.format(acq,sxtr_ra,sxtr_dec,astr_ra,astr_dec))
    
    # SExtractor
    self.store_u_point(fn=self.u_point_positions_base+'sxtr.anl',acq=acq,ra=sxtr_ra,dec=sxtr_dec)
    if not self.do_not_use_astrometry:
      # astrometry.net              
      self.store_u_point(fn=self.u_point_positions_base+'astr.anl',acq=acq,ra=astr_ra,dec=astr_dec)
                    
  def xy2lonlat_appr(self,px=None,py=None,ln0=None,lt0=None,acq=None,pcn=None):
    # small angle approximation for inverse gnomonic projection
    # ln0,lt0: field center
    # scale: angle/pixel [radian]
    lon=ln0 + px * self.px_scale/np.cos(lt0)
    lat=lt0 + py * self.px_scale
    if self.base_path in acq.image_fn:
      fn=acq.image_fn
    else:
      fn=os.path.join(self.base_path,acq.image_fn)
    self.lg.debug('{0}: sextract   result: {1:.6f} {2:.6f}, file: {3}'.format(pcn,lon*180/np.pi,lat*180/np.pi,fn))
    
    return lon,lat
            
  def display_fits(self,fn=None, x=None,y=None,color=None):
    ds9=ds9region.Ds9DisplayThread(debug=True,logger=self.lg)
    # Todo: ugly
    ds9.run(fn,x=x,y=y,color=color)
      
  def sextract(self,acq=None,pcn='single'):
    if self.ds9_display:
      self.lg.debug('sextract: Yale catalog number: {}'.format(int(acq.cat_no[0])))# ToDo why tupple
      
    if self.base_path in acq.image_fn:
      fn=acq.image_fn
    else:
      fn=os.path.join(self.base_path,acq.image_fn)
    sx = sextractor_3.Sextractor(['EXT_NUMBER','X_IMAGE','Y_IMAGE','MAG_BEST','FLAGS','CLASS_STAR','FWHM_IMAGE','A_IMAGE','B_IMAGE'],sexpath='/usr/bin/sextractor',sexconfig='/usr/share/sextractor/default.sex',starnnw='/usr/share/sextractor/default.nnw')
    try:
      sx.runSExtractor(filename=fn)
    except Exception as e:
      self.lg.error('exception: {}'.format(e))
      return None,None
    if len(sx.objects)==0:
      return None,None
    
    sx.sortObjects(sx.get_field('MAG_BEST'))
    i_x = sx.get_field('X_IMAGE')
    i_y = sx.get_field('Y_IMAGE')
    i_m = sx.get_field('MAG_BEST')
    try:
      brst = sx.objects[0]
    except:
      self.lg.warn('{0}: no sextract result for: {} '.format(pcn,fn))
      return None,None
    # relative to the image center
    # Attention: AltAz of in x
    x=brst[i_x]-self.ccd_size[0]/2.
    y=brst[i_y]-self.ccd_size[1]/2.

    self.lg.debug('{0}: sextract relative to center: {1:4.1f} px,{2:4.1f} px,{3:4.3f} mag'.format(pcn,x,y,brst[i_m]))
    if self.ds9_display:
      # absolute
      self.display_fits(fn=fn, x=brst[i_x],y=brst[i_y],color='red')
    return x,y
    
  def astrometry(self,acq=None,pcn='single'):
    if self.do_not_use_astrometry:
      return None
    if self.base_path in acq.image_fn:
      fn=acq.image_fn
    else:
      fn=os.path.join(self.base_path,acq.image_fn)

    self.lg.debug('{0}:         mount set: {1:.6f} {2:.6f}, file: {3}'.format(pcn,acq.eq.ra.degree,acq.eq.dec.degree,fn))
    if acq.eq_mnt.ra.radian != 0. and acq.eq_mnt.dec.radian != 0.:
      self.lg.debug('{0}:   mount read back: {1:.6f} {2:.6f}, file: {3}'.format(pcn,acq.eq_mnt.ra.degree,acq.eq_mnt.dec.degree,fn))
    astr_pixel_scale=self.px_scale/np.pi*3600.*180. # solve-field -u app arcsec/pixel
    sf= SolveField(lg=self.lg, fn=fn, blind=False, ra=acq.eq.ra.degree, dec=acq.eq.dec.degree,scale=astr_pixel_scale, radius=self.radius, replace=False, verbose=self.verbose_astrometry)
    sr= sf.solveField()
    if sr is not None:
      self.lg.debug('{0}: astrometry result: {1:.6f} {2:.6f}, file: {3}'.format(pcn,sr.ra,sr.dec,fn))
      astr_mnt_eq=SkyCoord(ra=sr.ra,dec=sr.dec, unit=(u.degree,u.degree), frame='icrs',location=self.obs)
      return astr_mnt_eq
    else:
      self.lg.debug('{}: no astrometry result: file: {}'.format(pcn,fn))
      return None

  def plot(self,title=None,projection='mollweide'):
    acq_eq_ra = coord.Angle([x.eq.ra.radian for x in self.acq if x.eq is not None], unit=u.radian)
    acq_eq_ra = acq_eq_ra.wrap_at(180*u.degree)
    acq_eq_dec = coord.Angle([x.eq.dec.radian for x in self.acq if x.eq is not None],unit=u.radian)
    #
    anl_sxtr_eq_ra = coord.Angle([x.sxtr.ra.radian for x in self.anl if x.eq is not None], unit=u.radian)
    #anl_sxtr_eq_ra = anl_sxtr_eq_ra.wrap_at(180*u.degree)
    anl_sxtr_eq_dec = coord.Angle([x.sxtr.dec.radian for x in self.anl if x.sxtr is not None],unit=u.radian)
    anl_astr_eq_ra = coord.Angle([x.astr.ra.radian for x in self.anl if x.astr is not None], unit=u.radian)
    #anl_astr_eq_ra = anl_astr_eq_ra.wrap_at(180*u.degree)
    anl_astr_eq_dec = coord.Angle([x.astr.dec.radian for x in self.anl if x.astr is not None],unit=u.radian)

    import matplotlib
    # this varies from distro to distro:
    matplotlib.rcParams["backend"] = "TkAgg"
    import matplotlib.pyplot as plt
    plt.ioff()
    fig = plt.figure(figsize=(8,6))
    ax = fig.add_subplot(111, projection=projection)
    ax.set_title(title)
    ax.scatter(acq_eq_ra, acq_eq_dec,color='blue',s=120.)
    ax.scatter(anl_sxtr_eq_ra, anl_sxtr_eq_dec,color='red',s=40.)
    ax.scatter(anl_astr_eq_ra, anl_astr_eq_dec,color='yellow',s=10.)
    ax.set_xticklabels(['14h','16h','18h','20h','22h','0h','2h','4h','6h','8h','10h',])

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

  parser= argparse.ArgumentParser(prog=sys.argv[0], description='Acquire not yet observed positions')
  parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
  parser.add_argument('--level', dest='level', default='WARN', help=': %(default)s, debug level')
  parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
  parser.add_argument('--break_after', dest='break_after', action='store', default=10000000, type=int, help=': %(default)s, read max. positions, mostly used for debuging')

  parser.add_argument('--obs-longitude', dest='obs_lng', action='store', default=123.2994166666666,type=arg_float, help=': %(default)s [deg], observatory longitude + to the East [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-latitude', dest='obs_lat', action='store', default=-75.1,type=arg_float, help=': %(default)s [deg], observatory latitude [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-height', dest='obs_height', action='store', default=3237.,type=arg_float, help=': %(default)s [m], observatory height above sea level [m], negative value: m10. equals to -10.')
  parser.add_argument('--plot', dest='plot', action='store_true', default=False, help=': %(default)s, plot results')
  #parser.add_argument('--observable-catalog', dest='observable_catalog', action='store', default='observable.cat', help=': %(default)s, retrieve the observable objects')
  #parser.add_argument('--nominal-positions', dest='nominal_positions', action='store', default='nominal_positions.cat', help=': %(default)s, to be observed positions (AltAz coordinates)')
  parser.add_argument('--acquired-positions', dest='acquired_positions', action='store', default='acquired_positions.acq', help=': %(default)s, already observed positions')
  parser.add_argument('--analyzed-positions', dest='analyzed_positions', action='store', default='analyzed_positions.anl', help=': %(default)s, already observed positions')
  parser.add_argument('--u-point-positions-base', dest='u_point_positions_base', action='store', default='u_point_positions_', help=': %(default)s, base path for u_point.py input (SExtractor, astrometry.net)')
  parser.add_argument('--pixel-scale', dest='pixel_scale', action='store', default=1.7,type=arg_float, help=': %(default)s [arcsec/pixel], arcmin/pixel of the CCD camera')
  parser.add_argument('--radius', dest='radius', action='store', default=1.,type=arg_float, help=': %(default)s [deg], astrometry search radius')
  parser.add_argument('--ccd-size', dest='ccd_size', default=[862.,655.], type=arg_floats, help=': %(default)s, ccd pixel size x,y[px], format "p1 p2"')
  parser.add_argument('--base-path', dest='base_path', action='store', default='./u_point_data/',type=str, help=': %(default)s , directory where images are stored')
  parser.add_argument('--ds9-display', dest='ds9_display', action='store_true', default=False, help=': %(default)s, inspect image and region with ds9')
  parser.add_argument('--do-not-use-astrometry', dest='do_not_use_astrometry', action='store_true', default=False, help=': %(default)s, use astrometry')
  parser.add_argument('--verbose-astrometry', dest='verbose_astrometry', action='store_true', default=False, help=': %(default)s, use astrometry in verbose mode')

  args=parser.parse_args()
  
  filename='/tmp/{}.log'.format(sys.argv[0].replace('.py','')) # ToDo datetime, name of the script
  logformat= '%(asctime)s:%(name)s:%(levelname)s:%(message)s'
  logging.basicConfig(filename=filename, level=args.level.upper(), format= logformat)
  logger=logging.getLogger()
    
  if args.toconsole:
    # http://www.mglerner.com/blog/?p=8
    soh=logging.StreamHandler(sys.stdout)
    soh.setLevel(args.level)
    logger.addHandler(soh)
  px_scale=args.pixel_scale/3600./180.*np.pi
  anl= Analysis(
    dbg=args.debug,
    lg=logger,
    obs_lng=args.obs_lng,
    obs_lat=args.obs_lat,
    obs_height=args.obs_height,
    px_scale=px_scale,
    radius=args.radius,
    ccd_size=args.ccd_size,
    acquired_positions=args.acquired_positions,
    analyzed_positions=args.analyzed_positions,
    u_point_positions_base=args.u_point_positions_base,
    break_after=args.break_after,
    do_not_use_astrometry=args.do_not_use_astrometry,
    verbose_astrometry=args.verbose_astrometry,
    ds9_display=args.ds9_display,
    base_path=args.base_path,
  )

  if not os.path.exists(args.base_path):
    os.makedirs(args.base_path)

  if args.plot:
    anl.fetch_positions(fn=anl.acquired_positions,fetch_acq=True,sys_exit=True)
    anl.fetch_positions(fn=anl.analyzed_positions,fetch_acq=False,sys_exit=False)
    anl.plot(title='acquired (blue), sextracted (red), astrometry (yellow) positions')
    sys.exit(0)

  lock=Lock()
  work_queue=Queue()
  
  ds9_queue=None    
  next_queue=None    
  cpus=int(cpu_count())
  if args.ds9_display:
    ds9_queue=Queue()
    next_queue=Queue()
    cpus=2 # one worker

  anl.fetch_positions(fn=anl.acquired_positions,fetch_acq=True,sys_exit=True)
  anl.fetch_positions(fn=anl.analyzed_positions,fetch_acq=False,sys_exit=False)
  analyzed=[x.image_fn for x in anl.anl]
  #if len(anl.anl)==len(analyzed) and len(anl.anl)>0:
  #  logger.info('all position analyzed, exiting')
  #  sys.exit(1)
    
  for o in anl.acq:
    if o.image_fn in analyzed:
      logger.debug('skiping analyzed position: {}'.format(o.image_fn))
      continue
    else:
      logger.debug('adding position: {}'.format(o.image_fn))
      
    work_queue.put(o)

  if len(anl.anl) and args.ds9_display:
    logger.warn('deleted positions will appear again, these are deliberately not stored file: {}'.format(args.analyzed_positions))
    
  processes = list()
  for w in range(1,cpus,1):
    p=Worker(work_queue=work_queue,ds9_queue=ds9_queue,next_queue=next_queue,lock=lock,dbg=args.debug,lg=logger,anl=anl)
    logger.debug('starting process: {}'.format(p.name))
    p.start()
    processes.append(p)
    work_queue.put('STOP')

  logger.debug('number of processes started: {}'.format(len(processes)))

  if args.ds9_display:
    ds9_queue.put('c')
    while True:
      next_queue.get()
      y = input('<RETURN> for next, dl for delete last, q for quit\n')
      if 'q' in y:
        for p in processes:
          ds9_queue.put('q')
          p.shutdown()
        break
      elif 'dl' in y:
        ds9_queue.put('dl')
      else:
        ds9_queue.put('c')
  
  for p in processes:
    logger.debug('waiting for process: {} to join'.format(p.name))
    p.join()
                                                            
  logger.debug('DONE')

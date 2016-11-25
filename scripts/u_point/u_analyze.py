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
import pyinotify

from multiprocessing import Lock, Queue, cpu_count

from collections import OrderedDict
from datetime import datetime
from astropy import units as u
from astropy.time import Time,TimeDelta
from astropy.coordinates import SkyCoord,EarthLocation
from astropy.coordinates import AltAz
from astropy.coordinates import Longitude,Latitude,Angle
import astropy.coordinates as coord
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

import ds9region
import sextractor_3
from structures import CatPosition,NmlPosition,AcqPosition,AnlPosition,cl_nms_acq,cl_nms_anl
from callback import AnnoteFinder
from notify import EventHandler 
from worker import Worker
from solver import SolverResult,Solver
from script import Script

class Analysis(Script):
  def __init__(
      self, dbg=None,
      lg=None,
      break_after=None,
      base_path=None,
      obs=None,
      acquired_positions=None,
      analyzed_positions=None,
      acq_e_h=None,
      px_scale=None,
      ccd_size=None,
      ccd_angle=None,
      mount_type_eq=None,
      u_point_positions_base=None,
      verbose_astrometry=None,
      ds9_display=None,
      solver=None,
  ):
    Script.__init__(self,lg=lg,break_after=break_after,base_path=base_path,obs=obs,acquired_positions=acquired_positions,analyzed_positions=analyzed_positions,acq_e_h=acq_e_h)
    #
    self.px_scale=px_scale
    self.ccd_size=ccd_size
    self.ccd_angle=args.ccd_angle
    self.mount_type_eq=mount_type_eq
    self.u_point_positions_base=u_point_positions_base
    self.verbose_astrometry=verbose_astrometry
    self.ds9_display=ds9_display
    self.solver=solver
    #
    self.ccd_rotation=self.rot(self.ccd_angle)
    self.acquired_positions=acquired_positions
    self.analyzed_positions=analyzed_positions
    self.dt_utc=Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date')
    self.acq=list()
    self.anl=list()

                    
  def rot(self,rads):
    s=np.sin(rads)
    c=np.cos(rads)
    return np.matrix([[c, -s],[s,  c]])
    
  def xy2lonlat_appr(self,px=None,py=None,acq=None,pcn=None):
    # mnt_eq: read back from mount
    # in case of RTS2 it includes any OFFS, these are manual corrections
    if self.mount_type_eq:
      ln0=acq.eq_mnt.ra.radian
      lt0=acq.eq_mnt.dec.radian
    else:
      aa=self.to_altaz(eq=acq.eq_mnt)
      ln0=aa.az.radian
      lt0=aa.alt.radian
    
    # ccd angle relative to +dec, +alt
    p=np.array([px,py])
    p_r= self.ccd_rotation.dot(p)               
    px_r=p_r[0,0]
    py_r=p_r[0,1]
    self.lg.debug('{0}: xy2lonlat_appr: px: {1}, py: {2}, px_r: {3}, py_r: {4}'.format(pcn,int(px),int(py),int(px_r),int(py_r)))
    # small angle approximation for inverse gnomonic projection
    # ln0,lt0: field center
    # scale: angle/pixel [radian]
    # px,py from SExtractor are relative to the center equals x,y physical
    # FITS with astrometry:
    #  +x: -ra
    #  +y: +dec
    if self.mount_type_eq:
      lon=ln0 - (px_r * self.px_scale/np.cos(lt0))
      lat=lt0 + py_r * self.px_scale
      #self.lg.debug('{0}: sextract   center: {1:.6f} {2:.6f}'.format(pcn,ln0*180./np.pi,lt0*180./np.pi))
      #self.lg.debug('{0}: sextract   star  : {1:.6f} {2:.6f}'.format(pcn,lon*180./np.pi,lat*180./np.pi))
    else:
      # astropy AltAz frame is right handed
      # +x: +az
      # +y: +alt
      az=ln0 + px_r * self.px_scale/np.cos(lt0)
      alt=lt0 + py_r * self.px_scale
      #self.lg.debug('{0}: sextract   center: {1:.6f} {2:.6f}'.format(pcn,ln0*180./np.pi,lt0*180./np.pi))
      #self.lg.debug('{0}: sextract   star  : {1:.6f} {2:.6f}'.format(pcn,az*180./np.pi,alt*180./np.pi))
      
      # ToDo exptime: dt_end - exp/2.
      aa=SkyCoord(az=az,alt=alt,unit=(u.radian,u.radian),frame='altaz',location=self.obs,obstime=acq.dt_end)
      eq=self.to_eq(aa=aa)
      lon=eq.ra.radian
      lat=eq.dec.radian
      
    if self.base_path in acq.image_fn:
      fn=acq.image_fn
    else:
      fn=os.path.join(self.base_path,acq.image_fn)
    self.lg.debug('{0}: sextract   result: {1:.6f} {2:.6f}, file: {3}'.format(pcn,lon*180./np.pi,lat*180./np.pi,fn))
    
    return lon,lat
  
  def sextract(self,acq=None,pcn='single'):
    #if self.ds9_display:
    #  self.lg.debug('sextract: Yale catalog number: {}'.format(int(acq.cat_no)))
      
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
    
  def astrometry(self,acq=None,pcn=None):
    if self.solver is None:
      return

    if self.base_path in acq.image_fn:
      fn=acq.image_fn
    else:
      fn=os.path.join(self.base_path,acq.image_fn)

    self.lg.debug('{0}:         mount set: {1:.6f} {2:.6f}, file: {3}'.format(pcn,acq.eq.ra.degree,acq.eq.dec.degree,fn))
    if acq.eq_mnt.ra.radian != 0. and acq.eq_mnt.dec.radian != 0.:
      self.lg.debug('{0}:   mount read back: {1:.6f} {2:.6f}, file: {3}'.format(pcn,acq.eq_mnt.ra.degree,acq.eq_mnt.dec.degree,fn))

    sr= self.solver.solve_field(fn=acq.image_fn,ra=acq.eq.ra.degree,dec=acq.eq.dec.degree,)
    if sr is not None:
      self.lg.debug('{0}: astrometry result: {1:.6f} {2:.6f}, file: {3}'.format(pcn,sr.ra,sr.dec,fn))
      astr_mnt_eq=SkyCoord(ra=sr.ra,dec=sr.dec, unit=(u.degree,u.degree), frame='icrs',location=self.obs)
      return astr_mnt_eq
    else:
      self.lg.debug('{}: no astrometry result: file: {}'.format(pcn,fn))
      return None

  def re_plot(self,i=0,animate=None):
    
    self.fetch_acquired_positions(sys_exit=True)
    self.fetch_analyzed_positions(sys_exit=False)
    #                                                               if self.anl=[]
    anl_sxtr_eq_ra=[x.sxtr.ra.degree for i,x in enumerate(self.anl) if x is not None and x.sxtr is not None]
    anl_sxtr_eq_dec=[x.sxtr.dec.degree for x in self.anl if x is not None and x.sxtr is not None]

    anl_astr_eq_ra=[x.astr.ra.degree for i,x in enumerate(self.anl) if x is not None and x.astr is not None]  
    anl_astr_eq_dec=[x.astr.dec.degree for x in self.anl if x is not None and x.astr is not None]

    acq_eq_ra =[x.eq.ra.degree for x in self.acq]
    acq_eq_dec = [x.eq.dec.degree for x in self.acq]
    #annotes=['{0:.1f},{1:.1f}: {2}'.format(x.eq.ra.degree, x.eq.dec.degree,x.image_fn) for x in self.acq]
    # ToDo: was nu? debug?
    annotes=['{0:.1f},{1:.1f}: {2}'.format(x.aa_mnt.az.degree, x.aa_mnt.alt.degree,x.image_fn) for x in self.acq]
    nml_ids=[x.nml_id for x in self.acq if x.aa_mnt is not None]

    self.ax.clear()
    self.ax.scatter(acq_eq_ra, acq_eq_dec,color='blue',s=120.)
    self.ax.scatter(anl_sxtr_eq_ra, anl_sxtr_eq_dec,color='red',s=40.)
    self.ax.scatter(anl_astr_eq_ra, anl_astr_eq_dec,color='yellow',s=10.)
    # mark last positions
    if len(anl_sxtr_eq_ra) > 0:
      self.ax.scatter(anl_sxtr_eq_ra[-1],anl_sxtr_eq_dec[-1],color='green',facecolors='none', edgecolors='green',s=300.)
    if len(anl_astr_eq_ra) > 0:
      self.ax.scatter(anl_astr_eq_ra[-1],anl_astr_eq_dec[-1],color='green',facecolors='none', edgecolors='magenta',s=400.)

    self.ax.set_xlim([0.,360.]) 

    if animate:
      self.ax.set_title(self.title, fontsize=10)
      now=str(Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date'))[:-7]
      self.ax.set_xlabel('RA [deg], at: {0} [UTC]'.format(now))
    else:
      self.ax.set_title(self.title)
      self.ax.set_xlabel('RA [deg]')
    
    self.ax.annotate('positions:',color='black', xy=(0.03, 0.05), xycoords='axes fraction')
    self.ax.annotate('acquired',color='blue', xy=(0.16, 0.05), xycoords='axes fraction')
    self.ax.annotate('sxtr',color='red', xy=(0.30, 0.05), xycoords='axes fraction')
    self.ax.annotate('astr: yellow',color='black', xy=(0.43, 0.05), xycoords='axes fraction')
    self.ax.annotate('last sxtr',color='green', xy=(0.63, 0.05), xycoords='axes fraction')
    self.ax.annotate('last astr',color='magenta', xy=(0.83, 0.05), xycoords='axes fraction')

    self.ax.set_ylabel('declination [deg]')  
    self.ax.grid(True)
      
    # does not exits at the beginning
    try:
      self.af.data = list(zip(nml_ids,acq_eq_ra,acq_eq_dec,annotes))
    except AttributeError:
      return nml_ids,acq_eq_ra,acq_eq_dec,annotes

    
  def plot(self,title=None,animate=None,delete=None):

    import matplotlib
    import matplotlib.animation as animation
    # this varies from distro to distro:
    matplotlib.rcParams["backend"] = "TkAgg"
    import matplotlib.pyplot as plt
    plt.ioff()
    fig = plt.figure(figsize=(8,6))
    self.ax = fig.add_subplot(111)
    self.title=title

    if animate:
      ani = animation.FuncAnimation(fig, self.re_plot, fargs=(animate,),interval=5000)

    (nml_ids,acq_eq_ra,acq_eq_dec,annotes)=self.re_plot(animate=animate)
    
    self.af = AnnoteFinder(nml_ids,acq_eq_ra,acq_eq_dec, annotes, ax=self.ax,xtol=5., ytol=5., ds9_display=self.ds9_display,lg=self.lg,annotate_fn=True,delete_one=self.delete_one_acquired_position)
    fig.canvas.mpl_connect('button_press_event',self.af.mouse_event)
    if delete:
      fig.canvas.mpl_connect('key_press_event',self.af.keyboard_event)

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

  parser= argparse.ArgumentParser(prog=sys.argv[0], description='Analyze observed positions')
  parser.add_argument('--level', dest='level', default='WARN', help=': %(default)s, debug level')
  parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
  parser.add_argument('--break_after', dest='break_after', action='store', default=10000000, type=int, help=': %(default)s, read max. positions, mostly used for debuging')

  parser.add_argument('--obs-longitude', dest='obs_lng', action='store', default=123.2994166666666,type=arg_float, help=': %(default)s [deg], observatory longitude + to the East [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-latitude', dest='obs_lat', action='store', default=-75.1,type=arg_float, help=': %(default)s [deg], observatory latitude [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-height', dest='obs_height', action='store', default=3237.,type=arg_float, help=': %(default)s [m], observatory height above sea level [m], negative value: m10. equals to -10.')
  parser.add_argument('--acquired-positions', dest='acquired_positions', action='store', default='acquired_positions.acq', help=': %(default)s, already observed positions')
  parser.add_argument('--base-path', dest='base_path', action='store', default='./u_point_data/',type=str, help=': %(default)s , directory where images are stored')
  parser.add_argument('--analyzed-positions', dest='analyzed_positions', action='store', default='analyzed_positions.anl', help=': %(default)s, already observed positions')
  # group plot
  parser.add_argument('--plot', dest='plot', action='store_true', default=False, help=': %(default)s, plot results')
  parser.add_argument('--ds9-display', dest='ds9_display', action='store_true', default=False, help=': %(default)s, inspect image and region with ds9')
  parser.add_argument('--animate', dest='animate', action='store_true', default=False, help=': %(default)s, True: plot will be updated whil acquisition is in progress')
  parser.add_argument('--delete', dest='delete', action='store_true', default=False, help=': %(default)s, True: click on data point followed by keyboard <Delete> deletes selected acquired measurements from file --acquired-positions')
  #
  parser.add_argument('--pixel-scale', dest='pixel_scale', action='store', default=1.7,type=float, help=': %(default)s [arcsec/pixel], arcmin/pixel of the CCD camera')
  parser.add_argument('--ccd-size', dest='ccd_size', default=[862.,655.], type=arg_floats, help=': %(default)s [px], ccd pixel size x,y[px], format "p1 p2"')
  # angle is defined relative to the positive dec or alt direction
  # the rotation is anti clock wise (right hand coordinate system)
  parser.add_argument('--ccd-angle', dest='ccd_angle', default=0., type=float, help=': %(default)s [deg], ccd angle measured anti clock wise relative to positive Alt or Dec axis, rotation of 180. ')
  parser.add_argument('--mount-type-eq', dest='mount_type_eq', action='store_true',default=False, help=': %(default)s, True: equatorial mount, False: altaz. Only used together with SExtractor.')
  # group SExtractor, astrometry.net
  parser.add_argument('--u-point-positions-base', dest='u_point_positions_base', action='store', default='u_point_positions_', help=': %(default)s, base file name for SExtractor, astrometry.net output files')
  parser.add_argument('--timeout', dest='timeout', action='store', default=120,type=int, help=': %(default)s [sec], astrometry timeout for finding a solution')
  parser.add_argument('--radius', dest='radius', action='store', default=1.,type=float, help=': %(default)s [deg], astrometry search radius')
  parser.add_argument('--do-not-use-astrometry', dest='do_not_use_astrometry', action='store_true', default=False, help=': %(default)s, use astrometry')
  parser.add_argument('--verbose-astrometry', dest='verbose_astrometry', action='store_true', default=False, help=': %(default)s, use astrometry in verbose mode')

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
    
  px_scale=args.pixel_scale/3600./180.*np.pi
  solver=None
  if not args.do_not_use_astrometry: # double neg
    solver= Solver(lg=logger,blind=False,scale=px_scale,radius=args.radius,replace=False,verbose=args.verbose_astrometry,timeout=args.timeout)

  acq_e_h=None
  if args.delete:
    wm=pyinotify.WatchManager()
    wm.add_watch(args.base_path,pyinotify.ALL_EVENTS, rec=True)
    acq_e_h=EventHandler(lg=logger,fn=args.acquired_positions)
    nt=pyinotify.ThreadedNotifier(wm,acq_e_h)
    nt.start()

  obs=EarthLocation(lon=float(args.obs_lng)*u.degree, lat=float(args.obs_lat)*u.degree, height=float(args.obs_height)*u.m)
  anl= Analysis(
    lg=logger,
    obs=obs,
    px_scale=px_scale,
    ccd_size=args.ccd_size,
    ccd_angle=args.ccd_angle/180. * np.pi,
    mount_type_eq=args.mount_type_eq,
    acquired_positions=args.acquired_positions,
    analyzed_positions=args.analyzed_positions,
    u_point_positions_base=args.u_point_positions_base,
    break_after=args.break_after,
    ds9_display=args.ds9_display,
    base_path=args.base_path,
    solver=solver,
    acq_e_h=acq_e_h,
  )

  if not os.path.exists(args.base_path):
    os.makedirs(args.base_path)

  anl.fetch_acquired_positions(sys_exit=True)
  anl.fetch_analyzed_positions(sys_exit=False)

  if args.plot:
    title='progress: analyzed positions'
    if args.ds9_display:
      title += ':\n click on blue dots to watch image (DS9)'
    if args.delete:
      title += '\n then press <Delete> to remove from the list of acquired positions'

    anl.plot(title=title,animate=args.animate,delete=args.delete)
    sys.exit(1)
    
  lock=Lock()
  work_queue=Queue()
  
  ds9_queue=None    
  next_queue=None    
  cpus=int(cpu_count())
  if args.ds9_display:
    ds9_queue=Queue()
    next_queue=Queue()
    cpus=2 # one worker

  sxtr_analyzed=[x.nml_id for x in anl.anl if x.sxtr is not None]
  astr_analyzed=[x.nml_id for x in anl.anl if x.astr is not None]
    
  for i,o in enumerate(anl.acq):
    if args.do_not_use_astrometry: # double neg
      if o.nml_id in sxtr_analyzed:
        logger.debug('skiping analyzed position: {}'.format(o.image_fn))
        continue
      else:
        logger.debug('xxx adding position: {},{}'.format(i,o.image_fn))
    else:
      if o.nml_id in sxtr_analyzed and o.nml_id in astr_analyzed:
        continue
      else:
        logger.debug('adding position: {}, {}'.format(i,o.image_fn))
        work_queue.put(o)
        continue
    
    work_queue.put(o)
    
  if len(anl.anl) and args.ds9_display:
    logger.warn('deleted positions will appear again, these are deliberately not stored file: {}'.format(args.analyzed_positions))
    
  processes = list()
  for w in range(1,cpus,1):
    p=Worker(work_queue=work_queue,ds9_queue=ds9_queue,next_queue=next_queue,lock=lock,lg=logger,anl=anl)
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
  sys.exit(0)

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
import socket
import numpy as np
import importlib
import queue
import time
# ToDo revisit:
import pyinotify

from datetime import datetime
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

from astropy import units as u
from astropy.time import Time
from astropy.coordinates import SkyCoord,Angle,EarthLocation,get_sun
from astropy.coordinates import AltAz
# ToDo not yet ready for prime time
#from u_point.notify import ImageEventHandler 
#from u_point.notify import EventHandler 
from u_point.callback import AnnoteFinder
from u_point.callback import  AnnotatedPlot
from u_point.script import Script
from u_point.quick_analysis import QuickAnalysis

class Acquisition(Script):
  def __init__(
      self,
      lg=None,
      break_after=None,
      base_path=None,
      obs=None,
      acquired_positions=None,
      acq_e_h=None,
      acq_queue=None,
      mode_watchdog=None,
      meteo=None,
      device=None,
      use_bright_stars=None,
      sun_separation=None,
      exposure_start=None,
  ):

    Script.__init__(self,lg=lg,break_after=break_after,base_path=base_path,obs=obs,acquired_positions=acquired_positions,acq_e_h=acq_e_h)
    self.acq_queue=acq_queue
    self.mode_watchdog=mode_watchdog
    self.meteo=meteo
    self.device=device
    self.use_bright_stars=use_bright_stars
    self.sun_separation=sun_separation
    self.exposure_start=exposure_start
    #
    #self.dt_utc = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date')
    
  

  def expose(self,nml_id=None,cat_no=None,cat_ic=None,exp=None):

    self.device.store_mount_data(cat_ic=cat_ic,nml_id=nml_id,cat_no=cat_no)
    exp=self.device.ccd_expose(exp=exp)
    # ToDo
    # whatch out for image_fn absolute relative path
    if self.mode_watchdog:
      self.lg.info('expose: waiting for image_fn from acq_queue')
      image_fn=self.acq_queue.get()
      self.lg.info('expose: image_fn from queue acq_queue: {}'.format(image_fn))

    sky=self.device.fetch_mount_data()
    pressure=temperature=humidity=None
    if self.meteo is not None:
      pressure,temperature,humidity=meteo.retrieve()
       
    sky.pressure=pressure
    sky.temperature=temperature
    sky.humidity=humidity

    self.append_position(sky=sky,analyzed=False)

  def now_observable(self,cat_ic=None, altitude_interval=None):
    cat_aa=cat_ic.transform_to(AltAz(location=self.obs, pressure=0.)) # no refraction here, UTC is in cat_ic
    #self.lg.debug('now_observable: altitude: {0:.2f},{1},{2}'.format(cat_aa.alt.degree, altitude_interval[0]*180./np.pi,altitude_interval[1]*180./np.pi))
    if altitude_interval[0]<cat_aa.alt.radian<altitude_interval[1]:
      return cat_aa
    else:
      #self.lg.debug('now_observable: out of altitude range {0:.2f},{1},{2}'.format(cat_aa.alt.degree, altitude_interval[0]*180./np.pi,altitude_interval[1]*180./np.pi))
      return None
  
  def find_near_neighbor(self,mnl_ic=None,altitude_interval=None,max_separation=None):
    il=iu=None
    max_separation2= max_separation * max_separation # lazy
    for i,o in enumerate(self.cat_observable): # catalog is RA sorted
      if il is None and o.cat_ic.ra.radian > mnl_ic.ra.radian - max_separation:
        il=i
      if iu is None and o.cat_ic.ra.radian > mnl_ic.ra.radian + max_separation:
        iu=i
        break
    else:
      iu=-1

    dist=list()
    for o in self.cat_observable[il:iu]:
      dra2=pow(o.cat_ic.ra.radian-mnl_ic.ra.radian,2)
      ddec2=pow(o.cat_ic.dec.radian-mnl_ic.dec.radian,2)

      cat_aa=self.now_observable(cat_ic=o.cat_ic,altitude_interval=altitude_interval)
      if cat_aa is None:
        val= 2.* np.pi
      else:
        val=dra2 + ddec2
      dist.append(val)

    try:
      dist_min=min(dist)
    except ValueError as e:
      self.lg.warn('find_near_neighbor: NO object found, maximal distance: {0:.3f} deg'.format(max_separation*180./np.pi))
      return None,None
      
    if dist_min> max_separation2:
      self.lg.warn('find_near_neighbor: NO suitable object found, {0:.3f} deg, maximal distance: {1:.3f} deg'.format(np.sqrt(dist_min)*180./np.pi,max_separation*180./np.pi))
      return None,None
    
    i_min=dist.index(dist_min)

    return self.cat_observable[il+i_min].cat_no,self.cat_observable[il+i_min].cat_ic

  def drop_nml_due_to_suns_position(self):
    now=Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date_hms')
    sun_aa= get_sun(now).transform_to(AltAz(obstime=now,location=self.obs))
    #
    sun_max_alt=-2.
    if sun_aa.alt.degree < sun_max_alt: # ToDo
      self.lg.error('drop_nml_due_to_suns_position: sun altitude: {} deg, below max: {} deg'.format(sun_aa.alt.degree,sun_max_alt))
      return
    
    #nml_aa_max_alt=max([x.nml_aa.alt.radian for x in self.nml if x.nml_aa is not None])
    for nml in self.nml:
      if nml.nml_aa is None:
        self.lg.debug('drop_nml_due_to_suns_position: sun altitude: {} deg, below max: {} deg'.format(sun_aa.alt.degree,sun_max_alt))
        continue
              
      nml_aa=SkyCoord(az=nml.nml_aa.az.radian,alt=nml.nml_aa.alt.radian,unit=(u.radian,u.radian),frame='altaz',location=self.obs,obstime=now)
      sep= sun_aa.separation(nml_aa)
      # drop all within
      if sep.radian < self.sun_separation.radian:
        nml.nml_aa=None
        #self.lg.dbug('set None: {}, {}'.format(nml.nml_id,sep.degree))
        continue
      
      # drop all below sun
      if sun_aa.az.radian - self.sun_separation.radian < nml_aa.az.radian  < sun_aa.az.radian + self.sun_separation.radian:
        if nml_aa.alt.radian <  sun_aa.alt.radian:
          self.lg.debug('set None below sun altitude: {}, {}'.format(nml.nml_id,nml_aa.alt.degree))
          nml.nml_aa=None
        
  def acquire(self,altitude_interval=None,max_separation=None):
    #
    self.device.ccd_init()  
    # self.nml contains only positions which need to be observed
    trgt_nml_aa=strt_nml_aa=None
    # do not remove:
    self.drop_nml_due_to_suns_position()
    last_nml_aa=None
    dt_last=Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date_hms')
    for nml in self.nml:
      if nml.nml_aa is None:
        continue
      dt_now=Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date_hms')
      diff= dt_now - dt_last
      if diff.sec > 900: # 15 minutes
        self.lg.debug('acquire: calling drop_nml_due_to_suns_position')
        self.drop_nml_due_to_suns_position()
        dt_last=dt_now
        
      # astropy N=0,E=90
      now=Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date_hms')
      nml_aa=SkyCoord(az=nml.nml_aa.az.radian,alt=nml.nml_aa.alt.radian,unit=(u.radian,u.radian),frame='altaz',location=self.obs,obstime=now)
      sun_aa= get_sun(now).transform_to(AltAz(obstime=now,location=self.obs))        
      mnl_ic=self.to_ic(aa=nml_aa)
      if sun_aa.alt > 0.:
        sep= sun_aa.separation(nml_aa)
        
        if sep.radian < self.sun_separation.radian:
          self.lg.warn('acquire: {0}, {1:6.2f} too close, continuing'.format(nml.nml_id,sep.degree))
          continue
        # if there are no target between current az, sun_aa.az+self.sun_separation
        # above th sun_alt + self.sun_separation
        # move the mount in two -120 deg steps to the other side
        #
        # first time we are not to close to sun (see above)
        if last_nml_aa is None:
          last_nml_aa=nml_aa
        # conditions:
        # was last position on the other side 
        # does the mount cross sun az AND
        if last_nml_aa.az.radian < sun_aa.az.radian-self.sun_separation.radian and \
          nml_aa.az.radian > sun_aa.az.radian+self.sun_separation.radian and \
          nml_aa.alt.radian < sun_aa.alt.radian+self.sun_separation.radian:
            
          self.lg.info('acquire: moving mount to: {0:6.2f},{1:6.2f}'.format(sun_aa.az.degree+self.sun_separation.degree,sun_aa.alt.degree+self.sun_separation.degree))
          i=-2
          # move from current az to az -120
          # -120 deg to avoid shortest path
          
          aa_plus_alt=SkyCoord(
            az=sun_aa.az.radian - 2 * np.pi/3. -8.33/180.*np.pi, # make it visible 
            alt=last_nml_aa.alt.radian,
            unit=(u.radian,u.radian),
            frame='altaz',
            location=self.obs,obstime=now)
          sep= sun_aa.separation(aa_plus_alt)
          # last resort
          if sep.radian < self.sun_separation.radian:
            self.lg.warn('acquire: first move, target:{0} to close while increasing altitude: {1:6.2f}, {2:6.2f} too close, exiting'.format(nml.nml_id,sep.degree,self.sun_separation.degree))
            #sys.exit(1)
          self.lg.warn('acquire: make -120 deg turn, setting azimuth: {0:6.2f}, {1:6.2f}'.format(aa_plus_alt.az.degree,aa_plus_alt.alt.degree))
          #
          nml_ic_tmp=self.to_ic(aa=aa_plus_alt)
          # RTS2 synchronizes (waits until the mount moved)
          # ToDo check e.g. libindi if it does it in the same way
          self.expose(nml_id=i,cat_no=i,cat_ic=nml_ic_tmp,exp=self.exposure_start)
          i=-1
          # move from current az to -120 deg
          aa_plus_az_plus_alt=SkyCoord(
            az=sun_aa.az.radian - 2 * np.pi/3. - 2 * np.pi/3. -8.33/180.*np.pi,  # again -120 deg to avoid shortest path,  make it visible
            alt=last_nml_aa.alt.radian,
            unit=(u.radian,u.radian),
            frame='altaz',
            location=self.obs,obstime=now)
          sep= sun_aa.separation(aa_plus_az_plus_alt)
          # last resort
          if sep.radian < self.sun_separation.radian:
            self.lg.error('acquire: second move, target: {0} to close while increasing azimuth: {1:6.2f}, lower limit {2:6.2f} too close, exiting'.format(nml.nml_id,sep.degree,self.sun_separation.degree))
            #sys.exit(1)
          self.lg.error('acquire: again make -120 deg turn, setting azimuth: {0:6.2f}, {1:6.2f}'.format(aa_plus_az_plus_alt.az.degree,aa_plus_az_plus_alt.alt.degree))
          nml_ic_tmp=self.to_ic(aa=aa_plus_az_plus_alt)
          self.expose(nml_id=i,cat_no=i,cat_ic=nml_ic_tmp,exp=self.exposure_start)

      cat_no=None
      # only catalog coordinates are needed here
      if self.use_bright_stars:
        cat_no,cat_ic=self.find_near_neighbor(mnl_ic=mnl_ic,altitude_interval=altitude_interval,max_separation=max_separation)
        if cat_ic is None:
          continue # no suitable object found, try next
      else:
        cat_ic=mnl_ic # observe at the current nominal position
        
      self.expose(nml_id=nml.nml_id,cat_no=cat_no,cat_ic=cat_ic,exp=self.exposure_start)  
      last_nml_aa=nml_aa

   
  def re_plot(self,i=0,ptfn=None,lon_step=None,animate=None):
    self.fetch_nominal_altaz(fn=ptfn,sys_exit=False)
    self.fetch_positions(sys_exit=False,analyzed=False)
    self.drop_nominal_altaz()
    self.drop_nml_due_to_suns_position()
    dt_now=Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date_hms')
    #              ugly
    diff= dt_now - self.dt_last
    if diff.sec > 900: # 15 minutes
      self.lg.error('acquire:re_plot: calling drop_nml_due_to_suns_position')
      try:
        self.drop_nml_due_to_suns_position()
        self.dt_last=dt_now
      except AttributeError as e:
        self.lg.debug('re_plot: data not yet ready')
        return
    
    mnt_aa_rdb_az = [x.mnt_aa_rdb.az.degree for x in self.sky_acq if x.mnt_aa_rdb is not None]
    mnt_aa_rdb_alt= [x.mnt_aa_rdb.alt.degree for x in self.sky_acq if x.mnt_aa_rdb is not None]
    nml_aa_az = [x.nml_aa.az.degree for x in self.nml if x.nml_aa is not None]
    nml_aa_alt= [x.nml_aa.alt.degree for x in self.nml if x.nml_aa is not None]
    # attention: clears annotations in plot too
    self.ax.clear()
    AnnoteFinder.drawnAnnotations ={} # a bit brutal
    
    self.ax.set_title(self.title, fontsize=10)
    self.ax.set_xlim([-lon_step,360.+lon_step]) # ToDo use args.lon_step for that
    self.ax.scatter(nml_aa_az, nml_aa_alt,color='red')

    if len(mnt_aa_rdb_az) > 0:
      self.ax.scatter(mnt_aa_rdb_az[-1], mnt_aa_rdb_alt[-1],color='magenta',s=240.)
      self.ax.scatter(mnt_aa_rdb_az, mnt_aa_rdb_alt,color='blue')
      
    now=Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date')
    sun_aa= get_sun(now).transform_to(AltAz(obstime=now,location=self.obs))
    if sun_aa.alt > 0.:
      mnt_aa_120_az = [x.nml_aa.az.degree for x in self.sky_acq if  x.nml_id < 0]
      mnt_aa_120_alt= [x.nml_aa.alt.degree for x in self.sky_acq if x.nml_id < 0]
      self.ax.scatter(sun_aa.az.degree,sun_aa.alt.degree,color='yellow',s=240.)
      self.ax.scatter(sun_aa.az.degree,sun_aa.alt.degree,color='red',s=100.)
      self.ax.scatter(sun_aa.az.degree,sun_aa.alt.degree,color='yellow',s=30.)
      # plot the -120 deg movements
      self.ax.scatter(mnt_aa_120_az,mnt_aa_120_alt,s=150.,facecolors='none', edgecolors='g')
      
      # ToDo very ugly!
      self.ax.add_patch(self.patches.Circle((sun_aa.az.degree,sun_aa.alt.degree), self.sun_separation.degree,fill=False))

    if animate:        
      mnt_ic=self.device.fetch_mount_position()
      if mnt_ic is None:
        #self.lg.debug('re_plot: mount position is None')
        # shorten it
        now_str=str(now)[:-7]
        self.ax.set_xlabel('azimuth [deg] (N=0,E=90), at: {0} [UTC]'.format(now_str))
      else:
        # cross hair current mount position
        mnt_aa_rdb=self.to_altaz(ic=mnt_ic)
        self.lg.debug('re_plot: mount position: {0:.2f}, {1:.2f} [deg]'.format(mnt_aa_rdb.az.degree,mnt_aa_rdb.alt.degree))
        self.ax.scatter(mnt_aa_rdb.az.degree,mnt_aa_rdb.alt.degree,color='green',facecolors='none', edgecolors='g',s=240.)
        self.ax.scatter(mnt_aa_rdb.az.degree,mnt_aa_rdb.alt.degree,color='green',marker='+',s=600.)
        self.ax.set_xlabel('azimut [deg] (N=0,E=90), pos: {0:.1f}, {1:.1f} [deg], at: {2} [UTC]'.format(mnt_aa_rdb.az.degree,mnt_aa_rdb.alt.degree,str(mnt_ic.obstime)[:-7]))

      self.ax.text(0., 0., 'positions:',color='black', fontsize=12,verticalalignment='bottom')
      self.ax.text(80., 0., 'nominal,',color='red', fontsize=12,verticalalignment='bottom')
      self.ax.text(140., 0., 'acquired,',color='blue', fontsize=12,verticalalignment='bottom')
      self.ax.text(210., 0., 'current,',color='magenta', fontsize=12,verticalalignment='bottom')
      self.ax.text(270., 0., 'mount',color='green', fontsize=12,verticalalignment='bottom')

    else:
      self.ax.set_xlabel('azimuth [deg] (N=0,E=90)')
      
    self.ax.set_ylabel('altitude [deg]')  
    self.ax.grid(True)
    # same if expression as above
    
    annotes=['{0:.1f},{1:.1f}: {2}'.format(x.mnt_aa_rdb.az.degree, x.mnt_aa_rdb.alt.degree,x.image_fn) for x in self.sky_acq if x.mnt_aa_rdb is not None]
    nml_ids=[x.nml_id for x in self.sky_acq if x.mnt_aa_rdb is not None]
    ##annotes=['{0:.1f},{1:.1f}: {2}'.format(x.aa_nml.az.radian, x.aa_nml.alt.radian,x.nml_id) for x in self.nml]
    # does not exits at the beginning    
    aps=[AnnotatedPlot(xx=self.ax,nml_id=nml_ids,lon=mnt_aa_rdb_az,lat=mnt_aa_rdb_alt,annotes=annotes)]
    try:
      self.af.aps=aps
      return
    except AttributeError:
      return aps

  def plot(self,title=None,ptfn=None,lon_step=None,ds9_display=None,animate=None,delete=None):
    import matplotlib
    import matplotlib.animation as animation
    # this varies from distro to distro:
    matplotlib.rcParams["backend"] = "TkAgg"
    import matplotlib.pyplot as plt
    import matplotlib.patches as patches
    # ToDo very ugly!
    self.patches=patches
    plt.ioff()
    fig = plt.figure(figsize=(8,6))
    self.ax = fig.add_subplot(111)
    self.title=title # ToDo could be passed via animation.F...
    
    if animate:
      ani = animation.FuncAnimation(fig,self.re_plot,fargs=(ptfn,lon_step,animate),interval=3000)
      #while True:
      #  self.fetch_positions(sys_exit=False,analyzed=False)
      #  if self.sky_acq is not None and len(self.sky_acq)>0:
      #    break
      #  self.lg.debug('plot: waiting for data')
      #  time.sleep(1)
    # ugly
    self.dt_last=Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date_hms')
    try:
      self.drop_nml_due_to_suns_position()
    except AttributeError as e:
      self.lg.debug('plot: data not yet ready')

    aps=self.re_plot(ptfn=ptfn,lon_step=lon_step,animate=animate)

    self.af = AnnoteFinder(ax=self.ax,aps=aps,xtol=5.,ytol=5.,ds9_display=ds9_display,lg=self.lg,annotate_fn=True,analyzed=False,delete_one=self.delete_one_position)
    ##self.af =  SimpleAnnoteFinder(acq_nml_az,acq_nml_alt, annotes, ax=self.ax,xtol=5., ytol=5., ds9_display=False,lg=self.lg)
    fig.canvas.mpl_connect('button_press_event',self.af.mouse_event)
    if delete:
      fig.canvas.mpl_connect('key_press_event',self.af.keyboard_event)
      
    #plt.show(block=False)
    plt.show()

# Ja, ja,..
# ToDo test it!

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
  # basic options
  parser.add_argument('--level', dest='level', default='WARN', help=': %(default)s, debug level')
  parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
  parser.add_argument('--break_after', dest='break_after', action='store', default=10000000, type=int, help=': %(default)s, read max. positions, mostly used for debuging')
  #
  parser.add_argument('--obs-longitude', dest='obs_lng', action='store', default=123.2994166666666,type=arg_float, help=': %(default)s [deg], observatory longitude + to the East [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-latitude', dest='obs_lat', action='store', default=-75.1,type=arg_float, help=': %(default)s [deg], observatory latitude [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-height', dest='obs_height', action='store', default=3237.,type=arg_float, help=': %(default)s [m], observatory height above sea level [m], negative value: m10. equals to -10.')
  parser.add_argument('--base-path', dest='base_path', action='store', default='/tmp/u_point/',type=str, help=': %(default)s , directory where images are stored')
  parser.add_argument('--acquired-positions', dest='acquired_positions', action='store', default='acquired_positions.acq', help=': %(default)s, already observed positions')
  # group plot
  parser.add_argument('--plot', dest='plot', action='store_true', default=False, help=': %(default)s, plot results')
  parser.add_argument('--ds9-display', dest='ds9_display', action='store_true', default=False, help=': %(default)s, True: and if --plot is specified: click on data point opens FITS with DS9')
  parser.add_argument('--animate', dest='animate', action='store_true', default=False, help=': %(default)s, True: plot will be updated whil acquisition is in progress')
  parser.add_argument('--delete', dest='delete', action='store_true', default=False, help=': %(default)s, True: click on data point followed by keyboard <Delete> deletes selected acquired measurements from file --acquired-positions')
  # ToDO not yet ready fro prime time
  parser.add_argument('--mode-watchdog', dest='mode_watchdog', action='store_true', default=False, help=': %(default)s, True: an RTS2 external CCD camera must be used')
  #parser.add_argument('--watchdog-directory', dest='watchdog_directory', action='store', default='.',type=str, help=': %(default)s , directory where the RTS2 external CCD camera writes the images')
  #
  parser.add_argument('--meteo-class', dest='meteo_class', action='store', default='meteo', help=': %(default)s, specify your meteo data collector, see meteo.py for a stub')
  # group device
  parser.add_argument('--device-class', dest='device_class', action='store', default='DeviceDss', help=': %(default)s, specify your device (RTS2,INDI), see devices.py')
  parser.add_argument('--fetch-dss-image', dest='fetch_dss_image', action='store_true', default=False, help=': %(default)s, astrometry.net or simulation mode: images fetched from DSS')
  parser.add_argument('--mode-user', dest='mode_user', action='store_true', default=False, help=': %(default)s, True: user input required False: pure astrometry.net or simulation mode: no user input, no images fetched from DSS, simulation: image download specify: --fetch-dss-image')
  # group create
  parser.add_argument('--create-nominal-altaz', dest='create_nominal_altaz', action='store_true', default=False, help=': %(default)s, True: create AltAz positions to be observed, see --nominal-positions')
  parser.add_argument('--eq-mount', dest='eq_mount', action='store_true', default=False, help=': %(default)s, True: create AltAz positions to be observed from HA/Dec grid, see --nominal-positions')

  parser.add_argument('--nominal-positions', dest='nominal_positions', action='store', default='nominal_positions.nml', help=': %(default)s, to be observed positions (AltAz coordinates)')
  parser.add_argument('--eq-excluded-ha-interval', dest='eq_excluded_ha_interval',   default=[120.,240.],type=arg_floats,help=': %(default)s [deg],  EQ mount excluded HA interval (avoid observations below NCP), format "p1 p2"')
  parser.add_argument('--eq-minimum-altitude', dest='eq_minimum_altitude',   default=30.,type=float,help=': %(default)s [deg],  EQ mount minimum altitude')

  parser.add_argument('--azimuth-interval', dest='azimuth_interval',   default=[0.,360.],type=arg_floats,help=': %(default)s [deg], AltAz mount allowed azimuth interval, format "p1 p2"')
  parser.add_argument('--altitude-interval',   dest='altitude_interval',   default=[30.,80.],type=arg_floats,help=': %(default)s [deg], AltAz mount, allowed altitude, format "p1 p2"')
  parser.add_argument('--lon-step', dest='lon_step', action='store', default=20, type=int,help=': %(default)s [deg], Az points: step is used as range(LL,UL,step), LL,UL defined by --azimuth-interval')
  parser.add_argument('--lat-step', dest='lat_step', action='store', default=10, type=int,help=': %(default)s [deg], Alt points: step is used as: range(LL,UL,step), LL,UL defined by --altitude-interval ')
  # 
  parser.add_argument('--use-bright-stars', dest='use_bright_stars', action='store_true', default=False, help=': %(default)s, True: use selected stars fron Yale bright Star catalog as target, see u_select.py')
  parser.add_argument('--observable-catalog', dest='observable_catalog', action='store', default='observable.cat', help=': %(default)s, file with on site observable objects, see u_select.py')  
  parser.add_argument('--max-separation', dest='max_separation', action='store', default=5.1,type=float, help=': %(default)s [deg], maximum separation (nominal, catalog) position')
  parser.add_argument('--sun-separation', dest='sun_separation', action='store', default=30.,type=float, help=': %(default)s [deg], angular separation between sun and observed object')
  #
  parser.add_argument('--ccd-size', dest='ccd_size', default=[862.,655.], type=arg_floats, help=': %(default)s, ccd pixel size x,y[px], format "p1 p2"')
  parser.add_argument('--pixel-scale', dest='pixel_scale', action='store', default=1.7,type=arg_float, help=': %(default)s [arcsec/pixel], pixel scale of the CCD camera')
  parser.add_argument('--exposure-start', dest='exposure_start',   default=5.,type=float,help=': %(default)s [sec], start exposure ')
  parser.add_argument('--do-quick-analysis', dest='do_quick_analysis', action='store_true', default=False, help=': %(default)s, True: check exposure time and change it see --exposure-interval, --background-max, --peak-max')
  parser.add_argument('--exposure-interval', dest='exposure_interval',   default=[1.,30.],type=float,help=': %(default)s [sec], exposure lower and upper limits, format "p1 p2"')
  parser.add_argument('--ratio-interval', dest='ratio_interval',   default=[.7,1.4],type=float,help=': %(default)s [], acceptable ratio maximum object brightnes/value of --peak-max, format "p1 p2"')
  parser.add_argument('--objects-min', dest='objects_min', default=10.,type=float,help=': %(default)s minimum number of objects per image')
  parser.add_argument('--background-max', dest='background_max', default=20000.,type=float,help=': %(default)s within 16 bits, background maximum')
  parser.add_argument('--peak-max', dest='peak_max', default=45000.,type=float,help=': %(default)s within 16 bits, peak maximum as found by SExtractor')

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
    soh.setLevel(args.level)
    logger.addHandler(soh)
    
  if not os.path.exists(args.base_path):
    os.makedirs(args.base_path)
        

  # ToDo
  if args.delete:
    args.animate=True
    
  # ToDo not yet ready for prime time
  acq_queue=None
  #if args.mode_watchdog:
  #  acq_queue=queue.Queue(maxsize=10)
  #  image_event_handler = ImageEventHandler(lg=logger,acq_queue=acq_queue)
  #  image_observer = Observer()
  #  image_observer.schedule(image_event_handler,args.watchdog_directory,recursive=True)
  #  image_observer.start()
    
  # ToDo not yet ready for prime time
  acq_e_h=None
  #if args.delete:
  #  wm=pyinotify.WatchManager()
  #  wm.add_watch(args.base_path,pyinotify.ALL_EVENTS, rec=True)
  #  acq_e_h=EventHandler(lg=logger,fn=args.acquired_positions)
  #  nt=pyinotify.ThreadedNotifier(wm,acq_e_h)
  #  nt.start()
    
  # ToD revisit dynamical loding
  # now load meteo class ...
  mt = importlib.import_module('meteo.'+args.meteo_class)
  meteo=mt.Meteo(lg=logger)

  quick_analysis=None
  if args.do_quick_analysis:
    quick_analysis=QuickAnalysis(lg=logger,ds9_display=args.ds9_display,background_max=args.background_max,peak_max=args.peak_max,objects_min=args.objects_min,exposure_interval=args.exposure_interval,ratio_interval=args.ratio_interval)
    
  px_scale=args.pixel_scale/3600./180.*np.pi # arcsec, radian
  obs=EarthLocation(lon=float(args.obs_lng)*u.degree, lat=float(args.obs_lat)*u.degree, height=float(args.obs_height)*u.m)
  # ... and device, ToDo revisit
  mod =importlib. __import__('u_point.devices', fromlist=[args.device_class])
  Device = getattr(mod, args.device_class)
  device= Device(lg=logger,obs=obs,px_scale=px_scale,ccd_size=args.ccd_size,base_path=args.base_path,fetch_dss_image=args.fetch_dss_image,quick_analysis=quick_analysis)
  if not device.check_presence():
    logger.error('no device present: {}, exiting'.format(args.device_class))
    sys.exit(1)
  
  sun_separation=Angle(args.sun_separation, u.degree)
  ac= Acquisition(
    lg=logger,
    obs=obs,
    acquired_positions=args.acquired_positions,
    break_after=args.break_after,
    acq_queue=acq_queue,
    mode_watchdog=args.mode_watchdog,
    meteo=meteo,
    base_path=args.base_path,
    device=device,
    use_bright_stars=args.use_bright_stars,
    acq_e_h=acq_e_h,
    sun_separation=sun_separation,
    exposure_start=args.exposure_start
  )
  if args.create_nominal_altaz:
    ac.store_nominal_altaz(lon_step=args.lon_step,lat_step=args.lat_step,azimuth_interval=args.azimuth_interval,altitude_interval=args.altitude_interval,eq_mount=args.eq_mount,eq_excluded_ha_interval=args.eq_excluded_ha_interval,eq_minimum_altitude=args.eq_minimum_altitude,fn=args.nominal_positions)
    if not args.plot:
      sys.exit(0)
      
  if args.plot and not args.animate:
    ac.plot(title='to be observed nominal positions',ptfn=args.nominal_positions,lon_step=args.lon_step,ds9_display=args.ds9_display,animate=False,delete=False) # no images yet
    sys.exit(0)

  if args.plot:      
    title='progress: acquired positions'
    if args.ds9_display:
      title += ':\n click on blue dots to watch image (DS9)'
    if args.delete:
      title += '\n then press <Delete> to remove from the list of acquired positions'
      
    ac.plot(title=title,ptfn=args.nominal_positions,lon_step=args.lon_step,ds9_display=args.ds9_display,animate=args.animate,delete=args.delete)
    sys.exit(0)
      

  max_separation=args.max_separation/180.*np.pi
  altitude_interval=list()
  for v in args.altitude_interval:
    altitude_interval.append(v/180.*np.pi)

  # candidate objects, predefined with u_select.py
  if args.use_bright_stars:
    ac.fetch_observable_catalog(fn=args.observable_catalog)

  ac.fetch_nominal_altaz(fn=args.nominal_positions,sys_exit=True)

  ac.fetch_positions(sys_exit=False,analyzed=False)
  # drop already observed positions
  ac.drop_nominal_altaz()
  # acquire unobserved positions
  ac.acquire(
    altitude_interval=altitude_interval,
    max_separation=max_separation,
  )
  logger.info('DONE: {}'.format(sys.argv[0].replace('.py','')))
  sys.exit(0)

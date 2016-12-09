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
from astropy.coordinates import SkyCoord,EarthLocation
from astropy.coordinates import AltAz

from notify import ImageEventHandler 
from notify import EventHandler 
from callback import AnnoteFinder
from callback import  AnnotatedPlot
from script import Script

class Acquisition(Script):
  def __init__(
      self,
      lg=None,
      break_after=None,
      base_path=None,
      obs=None,
      acquired_positions=None,
      acq_e_h=None,
      mode_user=None,
      acq_queue=None,
      mode_watchdog=None,
      meteo=None,
      device=None,
      use_bright_stars=None,
  ):

    Script.__init__(self,lg=lg,break_after=break_after,base_path=base_path,obs=obs,acquired_positions=acquired_positions,acq_e_h=acq_e_h)
    self.mode_user=mode_user
    self.acq_queue=acq_queue
    self.mode_watchdog=mode_watchdog
    self.meteo=meteo
    self.device=device
    self.use_bright_stars=use_bright_stars
    #
    self.dt_utc = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date')
    
  def now_observable(self,cat_ic=None, altitude_interval=None):
    cat_aa=cat_ic.transform_to(AltAz(location=self.obs, pressure=0.)) # no refraction here, UTC is in cat_ic
    #self.lg.debug('now_observable: altitude: {0:.2f},{1},{2}'.format(cat_aa.alt.degree, altitude_interval[0]*180./np.pi,altitude_interval[1]*180./np.pi))
    if altitude_interval[0]<cat_aa.alt.radian<altitude_interval[1]:
      return cat_aa
    else:
      #self.lg.debug('now_observable: out of altitude range {0:.2f},{1},{2}'.format(cat_aa.alt.degree, altitude_interval[0]*180./np.pi,altitude_interval[1]*180./np.pi))
      return None
  
  
  def create_socket(self, port=9999):
    # acquire runs as a subprocess of rts2-script-exec and has no
    # stdin available from controlling TTY.
    sckt=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # use telnet 127.0.0.1 9999 to connect
    for p in range(port,port-10,-1):
      try:
        sckt.bind(('0.0.0.0',p)) # () tupple!
        self.lg.info('create_socket port to connect for user input: {0}, use telnet 127.0.0.1 {0}'.format(p))
        break
      except Exception as e :
        self.lg.debug('create_socket: can not bind socket {}'.format(e))
        continue
      
    sckt.listen(1)
    (user_input, address) = sckt.accept()
    return user_input

  def wait_for_user(self,user_input=None, last_exposure=None,):
    menu=bytearray('mount synchronized\n',encoding='UTF-8')
    user_input.sendall(menu)
    menu=bytearray('current exposure: {0:.1f}\n'.format(last_exposure),encoding='UTF-8')
    user_input.sendall(menu)
    menu=bytearray('r [exp]: redo current position, [exposure [sec]]\n',encoding='UTF-8')
    user_input.sendall(menu)
    menu=bytearray('[exp]:  <RETURN> current exposure, [exposure [sec]],\n',encoding='UTF-8')
    user_input.sendall(menu)
    menu=bytearray('q: quit\n\n',encoding='UTF-8')
    user_input.sendall(menu)
    cmd=None
    while True:
      menu=bytearray('your choice: ',encoding='UTF-8')
      user_input.sendall(menu)
      uir=user_input.recv(512)
      user_input.sendall(uir)
      ui=uir.decode('utf-8').replace('\r\n','')
      self.lg.debug('user input:>>{}<<, len: {}'.format(ui,len(ui)))

      if 'q' in ui or 'Q' in ui: 
        user_input.close()
        self.lg.info('wait_for_user: quit, exiting')
        sys.exit(1)

      if len(ui)==0: #<ENTER> 
        exp=last_exposure
        cmd='e'
        break
      
      if len(ui)==1 and 'r' in ui: # 'r' alone
        exp=last_exposure
        cmd='r'
        break
      
      try:
        cmd,exps=ui.split()
        exp=float(exps)
        break
      except Exception as e:
        self.lg.debug('exception: {} '.format(e))

      try:
        cmd='e'
        exp=float(ui)
        break
      except:
        # ignore bad input
        menu=bytearray('no acceptable input: >>{}<<,try again\n'.format(ui),encoding='UTF-8')
        user_input.sendall(menu)
        continue

    return cmd,exp

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
      
  def acquire(self,altitude_interval=None,max_separation=None):
    if self.mode_user:
      user_input=self.create_socket()
    #
    self.device.ccd_init()  
    # self.nml contains only positions which need to be observed
    last_exposure=exp=.1
    not_first=False
    for nml in self.nml:
      # astropy N=0,E=90
      now=Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date_hms')
      nml_aa=SkyCoord(az=nml.nml_aa.az.radian,alt=nml.nml_aa.alt.radian,unit=(u.radian,u.radian),frame='altaz',location=self.obs,obstime=now)
      # only for book keeping
      mnl_ic=self.to_ic(aa=nml_aa)
      cat_no=None
      # only catalog coordinates are needed here
      if self.use_bright_stars:
        cat_no,cat_ic=self.find_near_neighbor(mnl_ic=mnl_ic,altitude_interval=altitude_interval,max_separation=max_separation)
        if cat_ic is None:
          continue # no suitable object found, try next
      else:
        cat_ic=mnl_ic # observe at the current nominal position
  
      if self.mode_user:
        while True:
          if not_first:
                self.expose(nml_id=nml.nml_id,cat_no=cat_no,cat_ic=cat_ic,exp=exp)
          else:
            not_first=True
              
          self.lg.info('acquire: mount synchronized, waiting for user input')
          cmd,exp=self.wait_for_user(user_input=user_input,last_exposure=last_exposure)
          last_exposure=exp
          self.lg.debug('acquire: user input received: {}, {}'.format(cmd,exp))
          if 'r' not in cmd:
            break
          else:
            self.lg.debug('acquire: redoing same position')
   
      else:
        self.expose(nml_id=nml.nml_id,cat_no=cat_no,cat_ic=cat_ic,exp=exp)

  def re_plot(self,i=0,ptfn=None,az_step=None,animate=None):
    self.fetch_positions(sys_exit=False,analyzed=False)
    self.fetch_nominal_altaz(fn=ptfn,sys_exit=False)
    self.drop_nominal_altaz()
    
    mnt_aa_rdb_az = [x.mnt_aa_rdb.az.degree for x in self.sky_acq if x.mnt_aa_rdb is not None]
    mnt_aa_rdb_alt= [x.mnt_aa_rdb.alt.degree for x in self.sky_acq if x.mnt_aa_rdb is not None]
    nml_aa_az = [x.nml_aa.az.degree for x in self.nml ]
    nml_aa_alt= [x.nml_aa.alt.degree for x in self.nml ]
    
    self.ax.clear()
    self.ax.set_title(self.title, fontsize=10)
    self.ax.set_xlim([-az_step,360.+az_step]) # ToDo use args.az_step for that
    self.ax.scatter(nml_aa_az, nml_aa_alt,color='red')

    if len(mnt_aa_rdb_az) > 0:
      self.ax.scatter(mnt_aa_rdb_az[-1], mnt_aa_rdb_alt[-1],color='magenta',s=240.)
      self.ax.scatter(mnt_aa_rdb_az, mnt_aa_rdb_alt,color='blue')
    
    if animate:
      mnt_ic=self.device.fetch_mount_position()
      if mnt_ic is None:
        #self.lg.debug('re_plot: mount position is None')
        # shorten it
        now=str(Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date'))[:-7]
        self.ax.set_xlabel('azimuth [deg] (N=0,E=90), at: {0} [UTC]'.format(now))
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

  def plot(self,title=None,ptfn=None,az_step=None,ds9_display=None,animate=None,delete=None):
    import matplotlib
    import matplotlib.animation as animation
    # this varies from distro to distro:
    matplotlib.rcParams["backend"] = "TkAgg"
    import matplotlib.pyplot as plt
    plt.ioff()
    fig = plt.figure(figsize=(8,6))
    self.ax = fig.add_subplot(111)
    self.title=title # ToDo could be passed via animation.F...

    if animate:
      ani = animation.FuncAnimation(fig,self.re_plot,fargs=(ptfn,az_step,animate),interval=5000)
      #while True:
      #  self.fetch_positions(sys_exit=False,analyzed=False)
      #  if self.sky_acq is not None and len(self.sky_acq)>0:
      #    break
      #  self.lg.debug('plot: waiting for data')
      #  time.sleep(1)
        
    aps=self.re_plot(ptfn=ptfn,az_step=az_step,animate=animate)

    self.af = AnnoteFinder(ax=self.ax,aps=aps,xtol=5.,ytol=5.,ds9_display=ds9_display,lg=self.lg,annotate_fn=True,analyzed=False,delete_one=self.delete_one_position)
    ##self.af =  SimpleAnnoteFinder(acq_nml_az,acq_nml_alt, annotes, ax=self.ax,xtol=5., ytol=5., ds9_display=False,lg=self.lg)
    fig.canvas.mpl_connect('button_press_event',self.af.mouse_event)
    if delete:
      fig.canvas.mpl_connect('key_press_event',self.af.keyboard_event)
      
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
  #
  parser.add_argument('--mode-watchdog', dest='mode_watchdog', action='store_true', default=False, help=': %(default)s, True: an RTS2 external CCD camera must be used')
  parser.add_argument('--watchdog-directory', dest='watchdog_directory', action='store', default='.',type=str, help=': %(default)s , directory where the RTS2 external CCD camera writes the images')
  #
  parser.add_argument('--meteo-class', dest='meteo_class', action='store', default='meteo', help=': %(default)s, specify your meteo data collector, see meteo.py for a stub')
  # group device
  parser.add_argument('--device-class', dest='device_class', action='store', default='DeviceDss', help=': %(default)s, specify your devices (mount,ccd), see devices.py')
  parser.add_argument('--fetch-dss-image', dest='fetch_dss_image', action='store_true', default=False, help=': %(default)s, astrometry.net or simulation mode: images fetched from DSS')
  parser.add_argument('--mode-user', dest='mode_user', action='store_true', default=False, help=': %(default)s, True: user input required False: pure astrometry.net or simulation mode: no user input, no images fetched from DSS, simulation: image download specify: --fetch-dss-image')
  # group create
  parser.add_argument('--create-nominal', dest='create_nominal', action='store_true', default=False, help=': %(default)s, True: create positions to be observed, see --nominal-positions')
  parser.add_argument('--nominal-positions', dest='nominal_positions', action='store', default='nominal_positions.nml', help=': %(default)s, to be observed positions (AltAz coordinates)')
  parser.add_argument('--azimuth-interval',   dest='azimuth_interval',   default=[0.,360.],type=arg_floats,help=': %(default)s [deg],  allowed azimuth, format "p1 p2"')
  # see Ronald C. Stone, Publications of the Astronomical Society of the Pacific 108: 1051-1058, 1996 November
  parser.add_argument('--altitude-interval',   dest='altitude_interval',   default=[30.,80.],type=arg_floats,help=': %(default)s [deg],  allowed altitude, format "p1 p2"')
  parser.add_argument('--az-step', dest='az_step', action='store', default=20, type=int,help=': %(default)s [deg], Az points: step is used as range(LL,UL,step), LL,UL defined by --azimuth-interval')
  parser.add_argument('--alt-step', dest='alt_step', action='store', default=10, type=int,help=': %(default)s [deg], Alt points: step is used as: range(LL,UL,step), LL,UL defined by --altitude-interval ')
  # 
  parser.add_argument('--use-bright-stars', dest='use_bright_stars', action='store_true', default=False, help=': %(default)s, True: use selected stars fron Yale bright Star catalog as target, see u_select.py')
  parser.add_argument('--observable-catalog', dest='observable_catalog', action='store', default='observable.cat', help=': %(default)s, file with on site observable objects, see u_select.py')  
  parser.add_argument('--max-separation', dest='max_separation', action='store', default=5.1,type=float, help=': %(default)s [deg], maximum separation (nominal, catalog) position')
  #
  parser.add_argument('--ccd-size', dest='ccd_size', default=[862.,655.], type=arg_floats, help=': %(default)s, ccd pixel size x,y[px], format "p1 p2"')
  parser.add_argument('--pixel-scale', dest='pixel_scale', action='store', default=1.7,type=arg_float, help=': %(default)s [arcsec/pixel], pixel scale of the CCD camera')

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

  acq_queue=None
  if args.mode_watchdog:
    acq_queue=queue.Queue(maxsize=10)
    image_event_handler = ImageEventHandler(lg=logger,acq_queue=acq_queue)
    image_observer = Observer()
    image_observer.schedule(image_event_handler,args.watchdog_directory,recursive=True)
    image_observer.start()
    
  acq_e_h=None
  if args.delete:
    wm=pyinotify.WatchManager()
    wm.add_watch(args.base_path,pyinotify.ALL_EVENTS, rec=True)
    acq_e_h=EventHandler(lg=logger,fn=args.acquired_positions)
    nt=pyinotify.ThreadedNotifier(wm,acq_e_h)
    nt.start()
    
  # ToD revisit dynamical loding
  # now load meteo class ...
  mt = importlib.import_module(args.meteo_class)
  meteo=mt.Meteo(lg=logger)
  # ... and device, ToDo revisit
  mod =importlib. __import__('devices', fromlist=[args.device_class])
  Device = getattr(mod, args.device_class)

  px_scale=args.pixel_scale/3600./180.*np.pi # arcsec, radian

  obs=EarthLocation(lon=float(args.obs_lng)*u.degree, lat=float(args.obs_lat)*u.degree, height=float(args.obs_height)*u.m)
  device= Device(lg=logger,obs=obs,px_scale=px_scale,ccd_size=args.ccd_size,base_path=args.base_path,fetch_dss_image=args.fetch_dss_image)
  ac= Acquisition(
    lg=logger,
    obs=obs,
    acquired_positions=args.acquired_positions,
    break_after=args.break_after,
    mode_user=args.mode_user,
    acq_queue=acq_queue,
    mode_watchdog=args.mode_watchdog,
    meteo=meteo,
    base_path=args.base_path,
    device=device,
    use_bright_stars=args.use_bright_stars,
    acq_e_h=acq_e_h,
  )
  
  if not device.check_presence():
    logger.error('no device present: {}, exiting'.format(args.device_class))
    sys.exit(1)

  max_separation=args.max_separation/180.*np.pi
  altitude_interval=list()
  for v in args.altitude_interval:
    altitude_interval.append(v/180.*np.pi)

  if not os.path.exists(args.base_path):
    os.makedirs(args.base_path)
        
  if args.create_nominal:
    ac.store_nominal_altaz(az_step=args.az_step,alt_step=args.alt_step,azimuth_interval=args.azimuth_interval,altitude_interval=args.altitude_interval,fn=args.nominal_positions)
    if args.plot:
      ac.plot(title='to be observed nominal positions',ptfn=args.nominal_positions,az_step=args.az_step,ds9_display=args.ds9_display,animate=False,delete=False) # no images yet
    sys.exit(0)


  if args.plot:
    title='progress: acquired positions'
    if args.ds9_display:
      title += ':\n click on blue dots to watch image (DS9)'
    if args.delete:
      title += '\n then press <Delete> to remove from the list of acquired positions'
      
    ac.plot(title=title,ptfn=args.nominal_positions,az_step=args.az_step,ds9_display=args.ds9_display,animate=args.animate,delete=args.delete)
    sys.exit(0)
   
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

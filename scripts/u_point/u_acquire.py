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
import requests
import pandas as pd

from collections import OrderedDict
from datetime import datetime
from astropy import units as u
from astropy.time import Time,TimeDelta
from astropy.coordinates import SkyCoord,EarthLocation
from astropy.coordinates import AltAz,CIRS,ITRS,ICRS
from astropy.coordinates import Longitude,Latitude,Angle
import astropy.coordinates as coord

from watchdog.observers import Observer
from watchdog.events import LoggingEventHandler
from watchdog.events import FileSystemEventHandler
import queue
# python 3 version
import scriptcomm_3
from position import CatPosition,NmlPosition,AcqPosition,cl_nms_acq,cl_nms_anl
from meteo import Meteo

dss_base_url='http://archive.eso.org/dss/dss/'

class Acquisition(scriptcomm_3.Rts2Comm):
  def __init__(
      self,
      dbg=None,
      lg=None,
      obs_lng=None,
      obs_lat=None,
      obs_height=None,
      acquired_positions=None,
      break_after=None,
      do_not_use_rts2=None,
      px_scale=None,
      ccd_size=None,
      mode_continues=None,
      acq_queue=None,
      mode_watchdog=None,
      fetch_dss_image=None,
      meteo=None,
      base_path=None,
  ):
    
    scriptcomm_3.Rts2Comm.__init__(self)

    self.dbg=dbg
    self.lg=lg
    self.obs_lng=obs_lng
    self.obs_lat=obs_lat
    self.obs_height=obs_height
    self.acquired_positions=acquired_positions
    self.break_after=break_after
    self.do_not_use_rts2=do_not_use_rts2
    self.px_scale=px_scale
    self.ccd_size=ccd_size
    self.mode_continues=mode_continues
    self.acq_queue=acq_queue
    self.mode_watchdog=mode_watchdog
    self.fetch_dss_image=fetch_dss_image
    self.acquired_positions=acquired_positions
    self.meteo=meteo
    self.base_path=args.base_path

    #
    self.obs=EarthLocation(lon=float(obs_lng)*u.degree, lat=float(obs_lat)*u.degree, height=float(obs_height)*u.m)
    self.dt_utc = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date')
    self.cat=list()
    self.nml=list()
    self.acq=list()
    
  def now_observable(self,cat_eq=None, altitude_interval=None):
    cat_aa=cat_eq.transform_to(AltAz(location=self.obs, pressure=0.)) # no refraction here, UTC is in cat_eq
    #self.lg.debug('now_observable: altitude: {0:.2f},{1},{2}'.format(cat_aa.alt.degree, altitude_interval[0]*180./np.pi,altitude_interval[1]*180./np.pi))
    if altitude_interval[0]<cat_aa.alt.radian<altitude_interval[1]:
      return cat_aa
    else:
      #self.lg.debug('now_observable: out of altitude range {0:.2f},{1},{2}'.format(cat_aa.alt.degree, altitude_interval[0]*180./np.pi,altitude_interval[1]*180./np.pi))
      return None
  
  def to_altaz(self,cat_eq=None):
    return cat_eq.transform_to(AltAz(location=self.obs, pressure=0.)) # no refraction here, UTC is in cat_eq

  def to_eq(self,aa=None):
    return aa.transform_to(ICRS()) 

  def fetch_pandas(self, ptfn=None,columns=None,sys_exit=True):
    pd_cat=None
    if not os.path.isfile(ptfn):
      self.lg.debug('fetch_pandas:{} does not exist'.format(ptfn))
      if sys_exit:
        sys.exit(1)
      return None
    
    pd_cat = pd.read_csv(ptfn, sep=',',header=None)
    try:
      pd_cat = pd.read_csv(ptfn, sep=',',header=None)
    except ValueError as e:
      self.lg.debug('fetch_pandas: {},{}'.format(ptfn, e))
      return None
    except OSError as e:
      self.lg.debug('fetch_pandas: {},{}'.format(ptfn, e))
      return None
    except Exception as e:
      #self.lg.debug('fetch_pandas: {},{}, exiting'.format(ptfn, e))
      sys.exit(1)
    pd_cat.columns = columns
    return pd_cat
  
  def fetch_observable_catalog(self, ptfn=None):
    pd_cat = self.fetch_pandas(ptfn=ptfn,columns=['cat_no','ra','dec','mag_v',],sys_exit=True)
    if pd_cat is None:
      return
    for i,rw in pd_cat.iterrows():
      if i > self.break_after:
        break
      
      cat_eq=SkyCoord(ra=rw['ra'],dec=rw['dec'], unit=(u.radian,u.radian), frame='icrs',obstime=self.dt_utc,location=self.obs)
      self.cat.append(CatPosition(cat_no=rw['cat_no'],cat_eq=cat_eq,mag_v=rw['mag_v'] ))

  def store_nominal_altaz(self,az_step=None,alt_step=None,azimuth_interval=None,altitude_interval=None,ptfn=None):
    # ToDo from pathlib import Path, fp=Path(ptfb),if fp.is_file())
    # format az_nml,alt_nml
    if self.base_path in ptfn:
      fn=ptfn
    else:
      fn=os.path.join(self.base_path,ptfn)

    if os.path.isfile(fn):
      a=input('overwriting existing file: {} [N/y]'.format(ptfn))
      if a not in 'y':
        self.lg.info('exiting')
        sys.exit(0)

    with  open(fn, 'w') as wfl:
      # ToDo input as int?
      az_rng=range(int(azimuth_interval[0]),int(azimuth_interval[1]),az_step) # No, exclusive + az_step
      
      alt_rng_up=range(int(altitude_interval[0]),int(altitude_interval[1]+alt_step),alt_step)
      alt_rng_down=range(int(altitude_interval[1]),int(altitude_interval[0])-alt_step,-alt_step)
      lir=len(alt_rng_up)
      up=True
      for i,az in enumerate(az_rng):
        # Epson MX-80
        if up:
          up=False
          rng=alt_rng_up
        else:
          up=True
          rng=alt_rng_down
          
        for j,alt in enumerate(rng):
          nml_id=i*lir+j
          azr=az/180.*np.pi
          altr=alt/180.*np.pi
          #          | is the id
          wfl.write('{},{},{}\n'.format(nml_id,azr,altr))
          aa_nml=SkyCoord(az=azr,alt=altr,unit=(u.radian,u.radian),frame='altaz',location=self.obs)
          self.nml.append(NmlPosition(nml_id=nml_id,aa_nml=aa_nml))

  def fetch_nominal_altaz(self,ptfn=None):
    if self.base_path in ptfn:
      fn=ptfn
    else:
      fn=os.path.join(self.base_path,ptfn)

    pd_cat = self.fetch_pandas(ptfn=fn,columns=['nml_id','az','alt'],sys_exit=True)
    if pd_cat is None:
      return

    for i,rw in pd_cat.iterrows():
      aa_nml=SkyCoord(az=rw['az'],alt=rw['alt'],unit=(u.radian,u.radian),frame='altaz',location=self.obs)
      self.nml.append(NmlPosition(nml_id=rw['nml_id'],aa_nml=aa_nml))

  def fetch_acquired_positions(self):
    if self.base_path in self.acquired_positions:
      ptfn=self.acquired_positions
    else:
      fn=os.path.join(self.base_path,self.acquired_positions)    

    pd_cat = self.fetch_pandas(ptfn=fn,columns=cl_nms_acq,sys_exit=False)
    if pd_cat is None:
      return
    for i,rw in pd_cat.iterrows():
      # ToDo why not out_subfmt='fits'
      dt_begin=Time(rw['dt_begin'],format='iso', scale='utc',location=self.obs,out_subfmt='date_hms')
      dt_end=Time(rw['dt_end'],format='iso', scale='utc',location=self.obs,out_subfmt='date_hms')
      dt_end_query=Time(rw['dt_end_query'],format='iso', scale='utc',location=self.obs,out_subfmt='date_hms')

      # ToDo set best time point
      aa_nml=SkyCoord(az=rw['aa_nml_az'],alt=rw['aa_nml_alt'],unit=(u.radian,u.radian),frame='altaz',obstime=dt_end,location=self.obs)
      acq_eq=SkyCoord(ra=rw['eq_ra'],dec=rw['eq_dec'], unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
      acq_eq_woffs=SkyCoord(ra=rw['eq_woffs_ra'],dec=rw['eq_woffs_dec'], unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
      acq_eq_mnt=SkyCoord(ra=rw['eq_mnt_ra'],dec=rw['eq_mnt_dec'], unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
      acq_aa_mnt=SkyCoord(az=rw['aa_mnt_az'],alt=rw['aa_mnt_alt'],unit=(u.radian,u.radian),frame='altaz',obstime=dt_end,location=self.obs)
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
      )
      self.acq.append(acq)
      
  def store_acquired_position(self,acq=None):
    if self.base_path in self.acquired_positions:
      fn=self.acquired_positions
    else:
      fn=os.path.join(self.base_path,self.acquired_positions)
    # append, one by one
    with  open(fn, 'a') as wfl:
      wfl.write('{0}\n'.format(acq))

  def drop_nominal_altaz(self):
    obs=[int(x.nml_id[0])  for x in self.acq]
    observed=sorted(set(obs),reverse=True)
    for i in observed:
      del self.nml[i]
      self.lg.debug('drop_nominal_altaz: deleted: {}'.format(i))
      
  def check_rts2_devices(self):
    cont=True
    try:
      mnt_nm = self.getDeviceByType(scriptcomm_3.DEVICE_TELESCOPE)
      self.lg.debug('check_rts2_devices: mount device: {}'.format(mnt_nm))
    except:
      self.lg.error('check_rts2_devices: could not find telescope')
      cont= False
    try:
      ccd_nm = self.getDeviceByType(scriptcomm_3.DEVICE_CCD)
      self.lg.debug('check_rts2_devices: CCD device: {}'.format(ccd_nm))
    except:
      self.lg.error('check_rts2_devices: could not find CCD')
      cont= False
    if not cont:
      self.lg.error('check_rts2_devices: exiting')
      sys.exit(1)
    return mnt_nm,ccd_nm

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

  def expose(self,nml_id=None,cat_no=None,cat_eq=None,mnt_nm=None,ccd_nm=None,exp=None):
    self.lg.debug('expose size: {},{},{}'.format(self.ccd_size[0],self.ccd_size[1],self.px_scale))
    if not self.do_not_use_rts2:
      self.setValue('ORI','{0} {1}'.format(cat_eq.cat_eq.ra.degree,cat_eq.cat_eq.dec.degree),mnt_nm)
      # ToDo check if Morvian can be read out in parallel (RTS2, VM Windows)
      # ToDo if not use maa-2015-10-18.py
      # ToDo CHECK if mount is THERE
      ra_oris,dec_oris=self.getValue('ORI',mnt_nm).split()
      self.lg.debug('ORI: {0:.3f},{1:.3f}'.format(float(ra_oris),float(dec_oris)))
      
    dt_begin = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')
    if self.fetch_dss_image:
      px_scale_arcmin=self.px_scale *180. * 60. /np.pi # to arcmin
      width= '{}'.format(float(self.ccd_size[0]) * px_scale_arcmin) # DSS [arcmin]
      height= '{}'.format(float(self.ccd_size[1]) * px_scale_arcmin) # DSS [arcmin]
      args=OrderedDict()
      # ToDo use cat_eq, attention format
      args['ra']='{0:.6f}'.format(cat_eq.cat_eq.ra.degree)
      args['dec']='{0:.6f}'.format(cat_eq.cat_eq.dec.degree)
      args['x']='{0:.2f}'.format(float(width))
      args['y']='{0:.2f}'.format(float(height))
      # compressed fits files SExtractor does not understand
      #args['mime-type']='image/x-gfits'
      args['mime-type']='image/x-fits'
      dfn='dss_{}_{}.fits'.format(args['ra'],args['dec']).replace('-','m').replace('.','_').replace('_fits','.fits') #ToDo lazy
      dss_fn=os.path.join(self.base_path,dfn)

      r=requests.get(dss_base_url, params=args, stream=True)
      self.lg.debug('expose: DSS: {}'.format(r.url))
      while True:
        if r.status_code == 200:
          with open(dss_fn, 'wb') as f:
            #for data in tqdm(r.iter_content()):
            f.write(r.content)
            for block in r.iter_content(1024):
              f.write(block)
          break
        else:
          self.lg.warn('expose: could not retrieve DSS: {}, continuing to retrieve'.format(r.url))

      exp=.1 # synchronization mount/CCD
    else:
      last_exposure=exp
    image_fn=None
    if not self.do_not_use_rts2:
      # RTS2 does synchronization mount/CCD
      self.lg.debug('expose: time {}'.format(exp))
      self.setValue('exposure',exp)
      image_fn = self.exposure()
      self.lg.debug('expose: image from RTS2: {}'.format(image_fn))
      
    if self.fetch_dss_image:
      try:
        os.unlink(image_fn) # the noisy CCD image
      except:
        pass # do not care
      image_fn=dss_fn
      self.lg.debug('expose: image from DSS: {}'.format(image_fn))
    elif self.mode_watchdog:
      self.lg.info('expose: waiting for image_fn from acq_queue')
      image_fn=self.acq_queue.get(acq)
      self.lg.info('expose: image_fn from queue acq_queue: {}'.format(image_fn))
    # ToDo may be best time point: dt_end - exposure/2.
    dt_end = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')

    if image_fn is None:
      self.lg.error('expose: no image source specified.')
      self.lg.info('expose: use --fetch_dss_image for simulation, --mode-watchdog for an external source, do not specify: --do-not-use-rts2')
      self.lg.error('expose: exiting.')
      sys.exit(1)
    
    # ToDo block should go to caller
    # fetch mount position etc
    if not self.do_not_use_rts2:
      JD=float(self.getValue('JD',mnt_nm))
      ras,decs=self.getValue('ORI',mnt_nm).split()
      now = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date')
      acq_eq=SkyCoord(ra=float(ras),dec=float(decs), unit=(u.degree,u.degree), frame='icrs',obstime=now,location=self.obs)

      ras,decs=self.getValue('WOFFS',mnt_nm).split()
      acq_eq_woffs=SkyCoord(ra=float(ras),dec=float(decs), unit=(u.degree,u.degree), frame='icrs',obstime=now,location=self.obs)

      ras,decs=self.getValue('TEL',mnt_nm).split()
      acq_eq_mnt=SkyCoord(ra=float(ras),dec=float(decs), unit=(u.degree,u.degree), frame='icrs',obstime=now,location=self.obs)

      alts,azs=self.getValue('TEL_',mnt_nm).split()
      acq_aa_mnt=SkyCoord(az=float(azs),alt=float(alts),unit=(u.degree,u.degree),frame='altaz',location=self.obs,obstime=now)

      # ToDo,obswl=0.5*u.micron, pressure=ln_pressure_qfe*u.hPa,temperature=ln_temperature*u.deg_C,relative_humidity=ln_humidity)
      # ToDo wrong:
      aa_nml=SkyCoord(az=float(azs),alt=float(alts),unit=(u.degree,u.degree),frame='altaz',location=self.obs,obstime=now)
      dt_end_query = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')
    else:
      JD=-1.      
      acq_eq=SkyCoord(ra=cat_eq.cat_eq.ra.radian,dec=cat_eq.cat_eq.dec.radian, unit=(u.radian,u.radian), frame='icrs',obstime=dt_begin,location=self.obs)
      acq_eq_woffs=SkyCoord(ra=0.,dec=0., unit=(u.radian,u.radian), frame='icrs',obstime=dt_begin,location=self.obs)
      acq_eq_mnt=SkyCoord(ra=0.,dec=0., unit=(u.radian,u.radian), frame='icrs',obstime=dt_begin,location=self.obs)
      # not yet used
      acq_aa_mnt=self.to_altaz(cat_eq=cat_eq.cat_eq)
      # ToDo wrong:
      aa_nml=self.to_altaz(cat_eq=cat_eq.cat_eq)
      dt_end_query = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')
    
    pressure=temperature=humidity=None
    if self.meteo is not None:
      pressure,temperature,humidity=meteo.retrieve()

    if self.base_path in image_fn:
      fn=image_fn
    else:
      fn=os.path.join(self.base_path,image_fn)

    acq=AcqPosition(
      nml_id=nml_id,
      cat_no=cat_eq.cat_no,
      aa_nml=aa_nml,
      eq=acq_eq,
      dt_begin=dt_begin,
      dt_end=dt_end,
      dt_end_query=dt_end_query,
      JD=JD,
      eq_woffs=acq_eq_woffs,
      eq_mnt=acq_eq_mnt,
      aa_mnt=acq_aa_mnt,
      image_fn=fn,
      exp=exp,
      pressure=pressure,
      temperature=temperature,
      humidity=humidity,
    )
    
    self.store_acquired_position(acq=acq)

  def find_near_neighbor(self,eq_nml=None,altitude_interval=None,max_separation=None):
    il=iu=None
    max_separation2= max_separation * max_separation # lazy
    for i,o in enumerate(self.cat): # catalog is RA sorted
      if il is None and o.cat_eq.ra.radian > eq_nml.ra.radian - max_separation:
        il=i
      if iu is None and o.cat_eq.ra.radian > eq_nml.ra.radian + max_separation:
        iu=i
        break
    else:
      iu=-1

    dist=list()
    for o in self.cat[il:iu]:
      dra2=pow(o.cat_eq.ra.radian-eq_nml.ra.radian,2)
      ddec2=pow(o.cat_eq.dec.radian-eq_nml.dec.radian,2)

      cat_aa=self.now_observable(cat_eq=o.cat_eq,altitude_interval=altitude_interval)
      if cat_aa is None:
        val= 2.* np.pi
      else:
        val=dra2 + ddec2
      dist.append(val)
 
    dist_min=min(dist)
    if dist_min> max_separation2:
      self.lg.warn('find_near_neighbor: NO suitable object found, {0:.3f} deg, maximal distance: {1:.3f} deg'.format(np.sqrt(dist_min)*180./np.pi,max_separation*180./np.pi))
      return None
    
    i_min=dist.index(dist_min)
    return self.cat[il+i_min]
      
  def acquire(self,mnt_nm=None,ccd_nm=None,altitude_interval=None,max_separation=None):
    if not self.mode_continues:
      user_input=self.create_socket()
    if not self.do_not_use_rts2:
      # full area
      self.setValue('WINDOW','%d %d %d %d' % (-1, -1, -1, -1))
      x_0,y_0,widths,heights=self.getValue('WINDOW',ccd_nm).split()
      self.lg.debug('acquire: CCD size: {},{}'.format(widths,heights))
      # make sure we are taking light images..
      self.setValue('SHUTTER','LIGHT')
      
    # self.nml contains only positions which need to be observed
    last_exposure=exp=.1
    not_first=False
    for nml in self.nml:
      aa_nml=nml.aa_nml
      # ToDo reconsider
      now=Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')
      aa=SkyCoord(az=aa_nml.az.radian,alt=aa_nml.alt.radian,unit=(u.radian,u.radian),frame='altaz',location=self.obs,obstime=now)
      eq_nml=self.to_eq(aa=aa)
      cat_eq=self.find_near_neighbor(eq_nml=eq_nml,altitude_interval=altitude_interval,max_separation=max_separation)
      if cat_eq: # else no suitable object found, try next
        if not self.mode_continues:
            while True:
              if not_first:
                self.expose(nml_id=nml.nml_id,cat_eq=cat_eq,mnt_nm=mnt_nm,ccd_nm=ccd_nm,exp=exp)
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
          self.expose(nml_id=nml.nml_id,cat_eq=cat_eq,mnt_nm=mnt_nm,ccd_nm=ccd_nm,exp=exp)

  def plot(self,title=None,projection='polar'): #AltAz 
    acq_az = coord.Angle([x.aa_mnt.az.radian for x in self.acq if x.aa_mnt is not None], unit=u.radian)
    acq_alt = coord.Angle([x.aa_mnt.alt.radian for x in self.acq if x.aa_mnt is not None],unit=u.radian)
    # ToDo rts2,astropy altaz
    off=np.pi
    if self.do_not_use_rts2:
      off=0.
    acq_nml_az = coord.Angle([x.aa_nml.az.radian-off for x in self.nml], unit=u.radian)
    #acq_nml_az = acq_nml_az.wrap_at(180*u.degree)
    acq_nml_alt = coord.Angle([x.aa_nml.alt.radian for x in self.nml],unit=u.radian)

    import matplotlib
    # this varies from distro to distro:
    matplotlib.rcParams["backend"] = "TkAgg"
    import matplotlib.pyplot as plt
    plt.ioff()
    fig = plt.figure(figsize=(8,6))

    ax = fig.add_subplot(111, projection=projection)
    ax.set_title(title)
    ax.scatter(acq_nml_az, acq_nml_alt,color='red')
    ax.scatter(acq_az, acq_alt,color='blue')
    ax.set_rmax(1.6)

    ax.grid(True)
    plt.show()

# Ja, ja,..
class MyHandler(FileSystemEventHandler):
    def __init__(self,lg=None,acq_queue=None):
        self.lg=lg
        self.acq_queue=acq_queue
    def on_modified(self, event):
      if 'fit' in event.src_path.lower():
        self.lg.info('RTS2 external CCD image: {}'.format(event.src_path))
        acq=acq_queue.put(event.src_path)
  
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
  parser.add_argument('--brightness-interval', dest='brightness_interval', default=[0.,7.0], type=arg_floats, help=': %(default)s, visual star brightness [mag], format "p1 p2"')
  parser.add_argument('--azimuth-interval',   dest='azimuth_interval',   default=[0.,360.],type=arg_floats,help=': %(default)s [deg],  allowed azimuth, format "p1 p2"')
  parser.add_argument('--altitude-interval',   dest='altitude_interval',   default=[10.,80.],type=arg_floats,help=': %(default)s [deg],  allowed altitude, format "p1 p2"')
  parser.add_argument('--observable-catalog', dest='observable_catalog', action='store', default='observable.cat', help=': %(default)s, file with on site observable objects, see u_select.py')
  parser.add_argument('--nominal-positions', dest='nominal_positions', action='store', default='nominal_positions.nml', help=': %(default)s, to be observed positions (AltAz coordinates)')
  parser.add_argument('--acquired-positions', dest='acquired_positions', action='store', default='acquired_positions.acq', help=': %(default)s, already observed positions')
  parser.add_argument('--create-nominal', dest='create_nominal', action='store_true', default=False, help=': %(default)s, True: create positions to be observed, see --nominal-positions')
  parser.add_argument('--az-step', dest='az_step', action='store', default=20, type=int,help=': %(default)s [deg], Az points: step is used as range(LL,UL,step), LL,UL defined by --azimuth-interval')
  parser.add_argument('--alt-step', dest='alt_step', action='store', default=10, type=int,help=': %(default)s [deg], Alt points: step is used as: range(LL,UL,step), LL,UL defined by --altitude-interval ')
  parser.add_argument('--pixel-scale', dest='pixel_scale', action='store', default=1.7/60.,type=arg_float, help=': %(default)s [arcmin/pixel], arcmin/pixel of the CCD camera')
  parser.add_argument('--mode-continues', dest='mode_continues', action='store_true', default=False, help=': %(default)s, True: simulation mode: no user input, no images fetched from DSS, image download specify: --fetch-dss-image')
  parser.add_argument('--fetch-dss-image', dest='fetch_dss_image', action='store_true', default=False, help=': %(default)s, simulation mode: images fetched from DSS')
  parser.add_argument('--max-separation', dest='max_separation', action='store', default=5.1,type=float, help=': %(default)s [deg], maximum separation (nominal, catalog) position')
  parser.add_argument('--mode-watchdog', dest='mode_watchdog', action='store_true', default=False, help=': %(default)s, True: an RTS2 external CCD camera must be used')
  parser.add_argument('--watchdog-directory', dest='watchdog_directory', action='store', default='.',type=str, help=': %(default)s , directory where the RTS2 external CCD camera writes the images')
  parser.add_argument('--do-not-use-rts2', dest='do_not_use_rts2', action='store_true', default=False, help=': %(default)s, True: only useful in simulation mode')
  parser.add_argument('--ccd-size', dest='ccd_size', default=[862.,655.], type=arg_floats, help=': %(default)s, ccd pixel size x,y[px], format "p1 p2"')
  parser.add_argument('--base-path', dest='base_path', action='store', default='./u_point_data/',type=str, help=': %(default)s , directory where images are stored')
  parser.add_argument('--with-meteo', dest='with_meteo', action='store_true', default=False, help=': %(default)s, True: retrieve meteo data through meteo.py')

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

  acq_queue=None
  if args.mode_watchdog:
    acq_queue=queue.Queue(maxsize=10)
    event_handler = MyHandler(lg=logger,acq_queue=acq_queue)
    observer = Observer()
    observer.schedule(event_handler, args.watchdog_directory, recursive=True)
    observer.start()
  meteo=None
  if args.with_meteo:
    meteo=Meteo(dbg=args.debug,lg=logger)
  
  px_scale=args.pixel_scale/60./180.*np.pi # arcmin
  ac= Acquisition(
    dbg=args.debug,
    lg=logger,
    obs_lng=args.obs_lng,
    obs_lat=args.obs_lat,
    obs_height=args.obs_height,
    acquired_positions=args.acquired_positions,
    break_after=args.break_after,
    do_not_use_rts2=args.do_not_use_rts2,
    px_scale=px_scale,
    ccd_size=args.ccd_size,
    mode_continues=args.mode_continues,
    acq_queue=acq_queue,
    mode_watchdog=args.mode_watchdog,
    fetch_dss_image=args.fetch_dss_image,
    meteo=meteo,
    base_path=args.base_path,
  )
  
  max_separation=args.max_separation/180.*np.pi
  altitude_interval=list()
  for v in args.altitude_interval:
    altitude_interval.append(v/180.*np.pi)

  if not os.path.exists(args.base_path):
    os.makedirs(args.base_path)
        
  if args.create_nominal:
    ac.store_nominal_altaz(az_step=args.az_step,alt_step=args.alt_step,azimuth_interval=args.azimuth_interval,altitude_interval=args.altitude_interval,ptfn=args.nominal_positions)
    if args.plot:
      ac.plot(title='to be observed nominal positions')
    sys.exit(1)

  ac.fetch_nominal_altaz(ptfn=args.nominal_positions)
  ac.fetch_acquired_positions()
  # drop already observed positions
  ac.drop_nominal_altaz()

  if args.plot:
    ac.plot(title='AltAz progress report acquired (blue), to be observed nominal positions (red)')
    sys.exit(1)
  
  mnt_nm=ccd_nm=None
  if not args.do_not_use_rts2:
    mnt_nm,ccd_nm=ac.check_rts2_devices()
    
  # candidate objects, predefined with u_select.py
  ac.fetch_observable_catalog(ptfn=args.observable_catalog)
  # acquire unobserved positions
  ac.acquire(
    mnt_nm=mnt_nm,
    ccd_nm=ccd_nm,
    altitude_interval=altitude_interval,
    max_separation=max_separation,
  )
  logger.info('DONE: {}'.format(sys.argv[0].replace('.py','')))

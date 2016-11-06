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

dss_base_url='http://archive.eso.org/dss/dss/'

class CatPosition(object):
  def __init__(self, cat_no=None,cat_eq=None,mag_v=None):
    self.cat_no=cat_no
    self.cat_eq=cat_eq 
    self.mag_v=mag_v
    
# ToDo may be only a helper
class NmlPosition(object):
  def __init__(self, nml_id=None,aa_nml=None,count=1):
    self.nml_id=nml_id
    self.aa_nml=aa_nml # nominal position (grid created with store_nominal_altaz())
    self.count=count
  # http://stackoverflow.com/questions/390250/elegant-ways-to-support-equivalence-equality-in-python-classes
  def __eq__(self, other):
    """Override the default Equals behavior"""
    if isinstance(other, self.__class__):
      return self.__dict__ == other.__dict__

    return NotImplemented
  
  def __ne__(self, other):
    """Define a non-equality test"""
    if isinstance(other, self.__class__):
      return not self.__eq__(other)
    return NotImplemented

  def __hash__(self):
    """Override the default hash behavior (that returns the id or the object)"""
    return hash(tuple(sorted(self.__dict__.items())))
  
class AcqPosition(object):
  def __init__(self,
               nml_id=None,
               cat_no=None,
               aa_nml=None,
               eq=None,
               dt_begin=None,
               dt_end=None,
               dt_end_query=None,
               JD=None,
               eq_woffs=None,
               eq_mnt=None,
               aa_mnt=None,
               image_fn=None,
               exp=None,
  ):
    self.nml_id=nml_id,
    self.cat_no=cat_no,
    self.aa_nml=aa_nml # nominal position (grid created with store_nominal_altaz())
    self.eq=eq # ORI (rts2-mon) acquired star positions
    self.dt_begin=dt_begin 
    self.dt_end=dt_end
    self.dt_end_query=dt_end_query
    self.JD=JD
    self.eq_woffs=eq_woffs # OFFS, offsets set manually
    self.eq_mnt=eq_mnt # TEL, read back from encodes
    self.aa_mnt=aa_mnt # TEL_ altaz 
    self.image_fn=image_fn
    self.exp=exp
    
class Acquisition(scriptcomm_3.Rts2Comm):
  def __init__(self, dbg=None,lg=None, obs_lng=None, obs_lat=None, obs_height=None,acqired_positions=None,break_after=None):
    scriptcomm_3.Rts2Comm.__init__(self)
    
    self.dbg=dbg
    self.lg=lg
    self.break_after=break_after
    #
    self.acqired_positions=acqired_positions
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

  def fetch_observable_catalog(self, ptfn=None):
    lns=list()
    with  open(ptfn, 'r') as lfl:
      lns=lfl.readlines()

    for i,ln in enumerate(lns):
      if i > self.break_after:
        break
      cat_nos,ras,decs,mag_vs=ln.split(',')
      try:
        cat_no=int(cat_nos)
        ra=float(ras)
        dec=float(decs)
        mag_v=float(mag_vs)
      except ValueError:
        self.lg.warn('fetch_observable_catalog: value error on line: {}, {}, {}'.format(i,ln[:-1],ptfn))
        continue
      except Exception as e:
        self.lg.error('fetch_observable_catalog: error on line: {}, {},{}'.format(i,e,ptfn))
        sys.exit(1)
      #not yet if brightness[0]<mag_v<brighness[1]:
      cat_eq=SkyCoord(ra=ra,dec=dec, unit=(u.radian,u.radian), frame='icrs',obstime=self.dt_utc,location=self.obs)
      self.cat.append(CatPosition(cat_no=cat_no,cat_eq=cat_eq,mag_v=mag_v))
      

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
      outer_rng=range(int(azimuth_interval[0]),int(azimuth_interval[1]),args.step)
      inner_rng=range(int(altitude_interval[0]),int(altitude_interval[1]),step)
      lir=len(inner_rng)
      for i,az in enumerate(outer_rng):
        for j,alt in enumerate(inner_rng):
          nml_id=i*lir+j
          azr=az/180.*np.pi
          altr=alt/180.*np.pi
          #          | is the id
          wfl.write('{},{},{}\n'.format(nml_id,azr,altr))
          aa_nml=SkyCoord(az=azr,alt=altr,unit=(u.radian,u.radian),frame='altaz',location=self.obs)
          self.nml.append(NmlPosition(nml_id=nml_id,aa_nml=aa_nml))

  def fetch_nominal_altaz(self,ptfn=None):
    # format: az_nml,alt_nml
    lns=list()
    try:
      with  open(ptfn, 'r') as rfl:
        lns=rfl.readlines()
    except:
      self.lg.error('fetch_nominal_altaz: no nominal altaz file found: {}, exiting'.format(ptfn))
      sys.exit(1)
    for i,ln in enumerate(lns):
      try:
        nml_ids,azs,alts,=ln.split(',')
        nml_id=int(nml_ids) # id
        az=float(azs)
        alt=float(alts)
      except ValueError:
        self.lg.warn('fetch_nominal_altaz: value error on line: {}, {}, {}'.format(i,ln[:-1],ptfn))
        continue
      except Exception as e:
        self.lg.error('fetch_nominal_altaz: error on line: {}, {},{}'.format(i,e,ptfn))
        sys.exit(1)
      # no Time() here
      aa_nml=SkyCoord(az=az,alt=alt,unit=(u.radian,u.radian),frame='altaz',location=self.obs)
      self.nml.append(NmlPosition(nml_id=nml_id,aa_nml=aa_nml))

  def fetch_acquired_positions(self):
    lns=list()
    try:
      with  open(self.acqired_positions, 'r') as rfl:
        lns=rfl.readlines()
    except Exception as e:
      self.lg.debug('fetch_acquired_positions: {},{}'.format(self.acqired_positions, e))

    for i,ln in enumerate(lns):
      try:
        (nml_id,#0
         cat_no,#1
         aa_nml_az_radian,#2
         aa_nml_alt_radian,#3
         eq_ra_radian,#4
         eq_dec_radian,#5
         dt_begins,#6
         dt_ends,#7
         dt_end_querys,#8
         JDs,#9
         eq_woffs_ra_radian,#10
         eq_woffs_dec_radian,#11
         eq_mnt_ra_radian,#12
         eq_mnt_dec_radian,#13
         aa_mnt_az_radian,#14
         aa_mnt_alt_radian,#15
         image_fn,#16
         exps)=ln.split(',')

      except ValueError:
        self.lg.warn('fetch_acquired_positions: value error on line: {},{}'.format(i,ln[:-1]))
        continue
      except Exception as e:
        self.lg.error('fetch_acquired_positions: error on line: {},{}'.format(i,e))
        sys.exit(1)

      dt_begin = Time(dt_begins,format='iso', scale='utc',location=self.obs,out_subfmt='fits')
      dt_end = Time(dt_ends,format='iso', scale='utc',location=self.obs,out_subfmt='fits')
      dt_end_query = Time(dt_end_querys,format='iso', scale='utc',location=self.obs,out_subfmt='fits')

      # ToDo set best time point
      aa_nml=SkyCoord(az=float(aa_nml_az_radian),alt=float(aa_nml_alt_radian),unit=(u.radian,u.radian),frame='altaz',location=self.obs,obstime=dt_end)
      acq_eq=SkyCoord(ra=float(eq_ra_radian),dec=float(eq_dec_radian), unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
      acq_eq_woffs=SkyCoord(ra=float(eq_woffs_ra_radian),dec=float(eq_woffs_dec_radian), unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
      acq_eq_mnt=SkyCoord(ra=float(eq_mnt_ra_radian),dec=float(eq_mnt_dec_radian), unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
      acq_aa_mnt=SkyCoord(az=float(aa_mnt_az_radian),alt=float(aa_mnt_alt_radian),unit=(u.radian,u.radian),frame='altaz',location=self.obs,obstime=dt_end)

      self.lg.debug('{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},{13},{14},{15},{16},{17}\n'.format(
        nml_id,#0
        cat_no,#1
        aa_nml_az_radian,#2
        aa_nml_alt_radian,#3
        eq_ra_radian,#4
        eq_dec_radian,#5
        dt_begins,#6
        dt_ends,#7
        dt_end_querys,#8
        JDs,#9
        eq_woffs_ra_radian,#10
        eq_woffs_dec_radian,#11
        eq_mnt_ra_radian,#12
        eq_mnt_dec_radian,#13
        aa_mnt_az_radian,#14
        aa_mnt_alt_radian,#15
        image_fn,#16
        exps,#17
      ))
      acq=AcqPosition(
        nml_id=nml_id,
        cat_no=cat_no,
        aa_nml=aa_nml,
        eq=acq_eq,
        dt_begin=dt_begin,
        dt_end=dt_end,
        dt_end_query=dt_end_query,
        JD=float(JDs),
        eq_woffs=acq_eq_woffs,
        eq_mnt=acq_eq_mnt,
        aa_mnt=acq_aa_mnt,
        image_fn=image_fn,
        exp=float(exps),
      )

      self.acq.append(acq)
      
  def store_acquired_position(self,acq=None):
    # append, one by one
    with  open(self.acqired_positions, 'a') as wfl:
      wfl.write('{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},{13},{14},{15},{16},{17}\n'.format(
        # TODO what is that: tupple it is an int!
        acq.nml_id[0],#0
        acq.cat_no[0],#1
        acq.aa_nml.az.radian,#2
        acq.aa_nml.alt.radian,#3
        acq.eq.ra.radian,#4
        acq.eq.dec.radian,#5
        acq.dt_begin,#6
        acq.dt_end,#7
        acq.dt_end_query,#8
        acq.JD,#9
        acq.eq_woffs.ra.radian,#10
        acq.eq_woffs.dec.radian,#11
        acq.eq_mnt.ra.radian,#12
        acq.eq_mnt.dec.radian,#13
        acq.aa_mnt.az.radian,#14
        acq.aa_mnt.alt.radian,#15
        acq.image_fn,#16
        acq.exp,#17
      ))

  def drop_nominal_altaz(self):
    # comparison nml_id self.nml, self.acq
    # compare and drop 
    # ToDo tupple !!
    obs=[int(x.nml_id[0])  for x in self.acq]
    observed=sorted(set(obs),reverse=True)
    for i in observed:
      del self.nml[i]
      self.lg.debug('drop_nominal_altaz: deleted: {}'.format(i))
      
  def check_rts2_devices(self):
    cont=True
    try:
      mnt_nm = self.getDeviceByType(scriptcomm_3.DEVICE_TELESCOPE)
      self.lg.debug('mount device: {}'.format(mnt_nm))
    except:
      self.lg.error('check_rts2_devices: could not find telescope')
      cont= False
    try:
      ccd_nm = self.getDeviceByType(scriptcomm_3.DEVICE_CCD)
      self.lg.debug('CCD device: {}'.format(ccd_nm))
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
        self.lg.info('port to connect for user input: {0}, use telnet 127.0.0.1 {0}'.format(p))
        break
      except Exception as e :
        self.lg.debug(': can not bind socket {}'.format(e))
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

  def expose(self,nml_id=None,cat_no=None,cat_eq=None,mnt_nm=None,ccd_nm=None,px_scale=0.,simulate=False,exp=None,widths=None,heights=None,mode_continues=True,acq_queue=None,mode_watchdog=None,fetch_dss_image=None):

    self.lg.debug('expose size: {},{},{}'.format(widths,heights,px_scale))

    self.setValue('ORI','{0} {1}'.format(cat_eq.cat_eq.ra.degree,cat_eq.cat_eq.dec.degree),mnt_nm)
    dt_begin = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')
    # ToDo put that in a thread
    # ToDo check if Morvian can be read out in parallel (RTS2, VM Windows)
    # if not use maa-2015-10-18.py
    if simulate:
      # ToDo CHECK if mount is THERE
      ra_oris,dec_oris=self.getValue('ORI',mnt_nm).split()
      self.lg.debug('ORI: {0:.3f},{1:.3f}'.format(float(ra_oris),float(dec_oris)))
      width_s= '{}'.format(float(widths) * px_scale/60.) # DSS [arcmin]
      height_s= '{}'.format(float(heights) * px_scale/60.) # DSS [arcmin]
      args=OrderedDict()
      args['ra']='{0:.6f}'.format(float(ra_oris))# DSS hh mm ss
      args['dec']='{0:.6f}'.format(float(dec_oris))# DSS ±dd mm ss
      args['x']='{0:.2f}'.format(float(width_s))# DSS ±dd mm ss
      args['y']='{0:.2f}'.format(float(height_s))
      args[ 'mime-type']='image/x-gfits'
      dss_fn='dss_{}_{}.fits'.format(args['ra'],args['dec']).replace('-','m')
      

      r=requests.get(dss_base_url, params=args, stream=True)
      self.lg.debug('expose: DSS: {}'.format(r.url))
      if fetch_dss_image:
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
    
    # RTS2 does synchronization mount/CCD
    self.lg.debug('expose: time {}'.format(exp))
    self.setValue('exposure',exp)
    image_fn = self.exposure()
    if simulate:
      try:
        os.unlink(image_fn) # the noisy CCD image
      except:
        pass # do not care
      image_fn=dss_fn
    elif mode_watchdog:
      self.lg.info('expose: waiting for image_fn from acq_queue')
      image_fn=acq_queue.get(acq)
      self.lg.info('expose: image_fn from queue acq_queue: {}'.format(image_fn))
    else:
      self.lg.debug('RTS2 expose image: {}'.format(image_fn))

    # ToDo block should go to caller
    dt_end = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')
    # fetch mount position etc
    JDs=self.getValue('JD',mnt_nm)
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
    dt_end_query = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')

    aa_nml=SkyCoord(az=float(azs),alt=float(alts),unit=(u.degree,u.degree),frame='altaz',location=self.obs,obstime=now)

    acq=AcqPosition(
      nml_id=nml_id,
      cat_no=cat_eq.cat_no,
      aa_nml=aa_nml,
      eq=acq_eq,
      dt_begin=dt_begin,
      dt_end=dt_end,
      dt_end_query=dt_end_query,
      JD=float(JDs),
      eq_woffs=acq_eq_woffs,
      eq_mnt=acq_eq_mnt,
      aa_mnt=acq_aa_mnt,
      image_fn=image_fn,
      exp=exp,)

    self.store_acquired_position(acq=acq)


  def find_near_neighbor(self,eq_nml=None,altitude_interval=None,max_separation=None):
    il=iu=None
    max_separation2= max_separation * max_separation
    for i,o in enumerate(self.cat): # is RA sorted
      if il is None and o.cat_eq.ra.radian > eq_nml.ra.radian - max_separation:
        il=i
      if iu is None and o.cat_eq.ra.radian > eq_nml.ra.radian + max_separation:
        iu=i
        break
    else:
      # self.lg.debug('find_near_neighbor: il: {},iu: {} upper limit not found'.format(il,iu))
      iu=-1

    dist=list()
    for o in self.cat[il:iu]:
      dra2=pow(o.cat_eq.ra.radian-eq_nml.ra.radian,2)
      ddec2=pow(o.cat_eq.dec.radian-eq_nml.dec.radian,2)

      cat_aa=self.now_observable(cat_eq=o.cat_eq,altitude_interval=altitude_interval)
      if cat_aa is None:
        #self.lg.debug('find_near_neighbor: no altitude, cat_no:         {}'.format(o.cat_no))
        val= 2.* np.pi
      else:
        val=dra2 + ddec2
      dist.append(val)
 
    dist_min=min(dist)
    if dist_min> max_separation2:
      self.lg.warn('find_near_neighbor: NO suitable object found')
      return None
    
    i_min=dist.index(dist_min)
    #self.lg.debug('find_near_neighbor: il: {0},index: {1}, min: {2:.2f}, altitude: {3:.2f}'.format(il, i_min,dist[i_min]*180./np.pi,self.to_altaz(cat_eq=self.cat[il+i_min].cat_eq).alt.degree))
    #self.lg.debug('find_near_neighbor: cat no: {0} {1}<<<<<<<<<<'.format(self.cat[il+i_min].cat_no,il+i_min))
    return self.cat[il+i_min]
      
  def acquire(self,mnt_nm=None,ccd_nm=None,px_scale=None,simulate=False,altitude_interval=None,mode_continues=False,max_separation=None,acq_queue=None,mode_watchdog=None,fetch_dss_image=None):
    if not mode_continues:
      user_input=self.create_socket()
    # full area
    self.setValue('WINDOW','%d %d %d %d' % (-1, -1, -1, -1))
    x_0,y_0,widths,heights=self.getValue('WINDOW',ccd_nm).split()
    self.lg.debug('size: {},{}'.format(widths,heights))
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
      if cat_eq:
        if not mode_continues:
            while True:
              if not_first:
                self.expose(nml_id=nml.nml_id,cat_eq=cat_eq,mnt_nm=mnt_nm,ccd_nm=ccd_nm,px_scale=px_scale,simulate=simulate,exp=exp,widths=widths,heights=heights,mode_continues=mode_continues,acq_queue=acq_queue,mode_watchdog=mode_watchdog,fetch_dss_image=fetch_dss_image)
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
          self.expose(nml_id=nml.nml_id,cat_eq=cat_eq,mnt_nm=mnt_nm,ccd_nm=ccd_nm,px_scale=px_scale,simulate=simulate,exp=exp,widths=widths,heights=heights,mode_continues=mode_continues,acq_queue=acq_queue,mode_watchdog=mode_watchdog,fetch_dss_image=fetch_dss_image)

  def plot(self,title=None,projection='polar'): #AltAz 
    acq_az = coord.Angle([x.aa_mnt.az.radian for x in self.acq if x.aa_mnt is not None], unit=u.radian)
    #acq_az = acq_az.wrap_at(180*u.degree)
    acq_alt = coord.Angle([x.aa_mnt.alt.radian for x in self.acq if x.aa_mnt is not None],unit=u.radian)
    # ToDo rts2,astropy altaz 
    acq_nml_az = coord.Angle([x.aa_nml.az.radian-np.pi for x in self.nml], unit=u.radian)
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
  parser.add_argument('--altitude-interval',   dest='altitude_interval',   default=[10.,80.],type=arg_floats,help=': %(default)s,  allowed altitude [deg], format "p1 p2"')
  parser.add_argument('--azimuth-interval',   dest='azimuth_interval',   default=[0.,360.],type=arg_floats,help=': %(default)s,  allowed azimuth [deg], format "p1 p2"')
  parser.add_argument('--observable-catalog', dest='observable_catalog', action='store', default='observable.cat', help=': %(default)s, retrieve the observable objects')
  parser.add_argument('--nominal-positions', dest='nominal_positions', action='store', default='nominal_positions.cat', help=': %(default)s, to be observed positions (AltAz coordinates)')
  parser.add_argument('--acqired-positions', dest='acqired_positions', action='store', default='acqired_positions.cat', help=': %(default)s, already observed positions')
  parser.add_argument('--create-nominal', dest='create_nominal', action='store_true', default=False, help=': %(default)s, create positions to be observed, see --nominal-positions')
  parser.add_argument('--step', dest='step', action='store', default=10, type=int,help=': %(default)s, AltAz points: step is used as: range(0,360,step), range(0,90,step) [deg]')
  parser.add_argument('--now-observable', dest='now_observable', action='store_true', default=False, help=': %(default)s, created positions are now above horizon')
  parser.add_argument('--simulate-image', dest='simulate_image', action='store_true', default=False, help=': %(default)s, fetch image from DSS, perform all operations on RTS2')
  parser.add_argument('--pixel-scale', dest='pixel_scale', action='store', default=4.,type=arg_float, help=': %(default)s [arcmin/pixel], arcmin/pixel of the CCD camera')
  parser.add_argument('--mode-continues', dest='mode_continues', action='store_true', default=False, help=': %(default)s, debug mode: no user input, no images fetched from DSS')
  parser.add_argument('--fetch-dss-image', dest='fetch_dss_image', action='store_true', default=False, help=': %(default)s, debug mode: images fetched from DSS')
  parser.add_argument('--max-separation', dest='max_separation', action='store', default=5.1,type=float, help=': %(default)s [deg], maximum separation nominal, catalog position')
  parser.add_argument('--mode-watchdog', dest='mode_watchdog', action='store_true', default=False, help=': %(default)s, set it, if an RTS2 external CCD camera must be used')
  parser.add_argument('--watchdog-directory', dest='watchdog_directory', action='store', default='.',type=str, help=': %(default)s , directory where the RTS2 external CCD camer writes the images')


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

    
  ac= Acquisition(dbg=args.debug,lg=logger,obs_lng=args.obs_lng,obs_lat=args.obs_lat,obs_height=args.obs_height,acqired_positions=args.acqired_positions,break_after=args.break_after)
  max_separation=args.max_separation/180.*np.pi
  altitude_interval=list()
  for v in args.altitude_interval:
    altitude_interval.append(v/180.*np.pi)

  if args.create_nominal:
    ac.store_nominal_altaz(step=args.step,azimuth_interval=args.azimuth_interval,altitude_interval=args.altitude_interval,ptfn=args.nominal_positions)
    if args.plot:
      ac.plot(title='to be observed nominal positions')
    sys.exit(1)

  ac.fetch_nominal_altaz(ptfn=args.nominal_positions)
  ac.fetch_acquired_positions()
  # drop already observed positions
  ac.drop_nominal_altaz()

  if args.plot:
    ac.plot(title='progress report, dropped observed nominal positions')
  else:
    mnt_nm,ccd_nm=ac.check_rts2_devices()
    # candidate objects, predefined with u_select.py
    ac.fetch_observable_catalog(ptfn=args.observable_catalog)
    # acquire unobserved positions
    ac.acquire(
      mnt_nm=mnt_nm,
      ccd_nm=ccd_nm,
      px_scale=args.pixel_scale,
      simulate=args.simulate_image,
      altitude_interval=altitude_interval,
      mode_continues=args.mode_continues,
      max_separation=max_separation,
      acq_queue=acq_queue,
      mode_watchdog=args.mode_watchdog,
      fetch_dss_image=args.fetch_dss_image,
    )

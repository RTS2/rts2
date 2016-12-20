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

Connect to various devices, RTS2 or DSS


'''

__author__ = 'wildi.markus@bluewin.ch'

import os
from collections import OrderedDict

import requests
import numpy as np
from astropy import units as u
from astropy.coordinates import SkyCoord
# ToDo unify AltAz altaz
from astropy.coordinates import AltAz
from datetime import datetime
from astropy.time import Time

# python 3 version
import u_point.scriptcomm_3 as scriptcomm_3
from u_point.structures import SkyPosition

class DeviceDss(object):
  def __init__(
    self,
    lg=None,
    obs=None,
    px_scale=None,
    ccd_size=None,
    base_path=None,
    fetch_dss_image=None,
    quick_analysis=None,
  ):
    self.lg=lg
    self.dss_base_url='http://archive.eso.org/dss/dss/'
    self.obs=obs
    self.px_scale_arcmin=px_scale *180. * 60. /np.pi # to arcmin
    self.ccd_size=ccd_size
    self.base_path=base_path
    self.fetch_dss_image=fetch_dss_image
    self.mount_type_eq=True
    self.quick_analysis=quick_analysis
    self.cat_ic=None
    self.dss_image_ptfn=None
    self.dss_image_fn=None
    self.dt_begin=None
    self.dt_end=None
    self.nml_id=None
    self.cat_no=None

    
  def check_presence(self,**kwargs):
    return True

  def fetch_mount_position(self):
    if self.cat_ic is None:
      return None
    return self.cat_ic
                  
  def ccd_init(self):
    pass

  def __expose(self,exp=None):
    width= '{}'.format(float(self.ccd_size[0]) * self.px_scale_arcmin) # DSS [arcmin]
    height= '{}'.format(float(self.ccd_size[1]) * self.px_scale_arcmin) # DSS [arcmin]
    args=OrderedDict()
    # 
    args['ra']='{0:.6f}'.format(self.cat_ic.ra.degree)
    args['dec']='{0:.6f}'.format(self.cat_ic.dec.degree)
    args['x']='{0:.2f}'.format(float(width))
    args['y']='{0:.2f}'.format(float(height))
    # compressed fits files SExtractor does not understand
    #args['mime-type']='image/x-gfits'
    args['mime-type']='image/x-fits'
    self.dss_image_fn='dss_{}_{}.fits'.format(args['ra'],args['dec']).replace('-','m').replace('.','_').replace('_fits','.fits') #ToDo lazy
    self.dss_image_ptfn=os.path.join(self.base_path,self.dss_image_fn)
    self.dt_begin = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')
    if self.fetch_dss_image: # e.g., Done Concordia almost offline
      dbglvl=self.lg.level
      # do not want all the messages
      self.lg.setLevel('ERROR')
      r=requests.get(self.dss_base_url, params=args, stream=True)
      
      self.dt_end = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')
      #self.lg.debug('expose: DSS: {}'.format(r.url))
      while True:
        if r.status_code == 200:
          with open(self.dss_image_ptfn, 'wb') as f:
            #for data in tqdm(r.iter_content()):
            f.write(r.content)
            for block in r.iter_content(1024):
              f.write(block)
            break
        else:
          self.lg.warn('expose: could not retrieve DSS: {}, continuing to retrieve'.format(r.url))
      self.lg.level=dbglvl

      self.lg.debug('expose: DSS: saved {}'.format(r.url))
    else:
      self.dt_end = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')
      self.lg.debug('expose: not fetching from DSS and not storing: {}'.format(self.dss_image_ptfn))
        
    # for DeviceRts2 in simulation mode
    return exp
  
  def ccd_expose(self,exp=None):
    self.exp=exp
    if self.quick_analysis is not None:
      # since this is virtual device we can not change exposure, break after second trial
      i_break=0
      while True:
        self.exp=self.__expose(exp=self.exp)
        self.exp,redo=self.quick_analysis.analyze(nml_id=self.nml_id,exposure_last=self.exp,ptfn=self.dss_image_ptfn)
        if self.exp <= self.quick_analysis.exposure_interval[0]:
          self.lg.warn('ccd_expose: giving up, exposure time reached lower limit: {}'.format(self.quick_analysis.exposure_interval[0]))
          break
        if self.exp >= self.quick_analysis.exposure_interval[1]:
          self.lg.warn('ccd_expose: giving up, exposure time reached upper limit: {}'.format(self.quick_analysis.exposure_interval[1]))
          break
        if not redo:
          break
        if i_break > 2:
          break
        i_break +=1
    else:
      self.exp=self.__expose(exp=self.exp)
    # for DeviceRts2 in simulation mode
    return self.exp
  
  def store_mount_data(self,cat_ic=None,nml_id=None,cat_no=None):
    self.nml_id=nml_id
    self.cat_no=cat_no
    self.cat_ic=cat_ic
    self.mnt_ap_set=cat_ic
  
  def fetch_mount_data(self):
    if self.cat_ic is None:
      return None
    JD=np.nan
    # simulation
    mnt_ra_rdb=SkyCoord(ra=self.cat_ic.ra.radian,dec=self.cat_ic.dec.radian, unit=(u.radian,u.radian), frame='icrs',obstime=self.dt_begin,location=self.obs)
    mnt_aa_rdb=self.cat_ic.transform_to(AltAz(location=self.obs, pressure=0.)) # no refraction here, UTC is in cat_ic
    nml_aa=self.cat_ic.transform_to(AltAz(location=self.obs, pressure=0.)) # no refraction here, UTC is in cat_ic
    dt_end_query = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date_hms')
    sky=SkyPosition(
      nml_id=self.nml_id,
      cat_no=self.cat_no,
      nml_aa=nml_aa,
      cat_ic=self.cat_ic,
      dt_begin=self.dt_begin,
      dt_end=self.dt_end,
      dt_end_query=dt_end_query,
      JD=JD,
      mnt_ra_rdb=mnt_ra_rdb,
      mnt_aa_rdb=mnt_aa_rdb,
      image_fn=self.dss_image_fn, # only fn
      exp=self.exp,
      mount_type_eq=self.mount_type_eq,
    )
    return sky


class DeviceRts2(scriptcomm_3.Rts2Comm):
  def __init__(
      self,
      dbg=None,
      lg=None,
      obs=None,
      base_path=None,
      px_scale=None, 
      ccd_size=None, 
      fetch_dss_image=None,
      quick_analysis=None,
  ):
    scriptcomm_3.Rts2Comm.__init__(self)
    self.lg=lg
    self.obs=obs
    self.base_path=base_path
    self.px_scale=px_scale
    self.ccd_size=ccd_size
    self.fetch_dss_image=fetch_dss_image
    self.quick_analysis=quick_analysis

    self.mount_type_eq=True #, init? ToDo, ev. fetch it from RTS2
    self.mnt_nm=None
    self.ccd_nm=None
    self.dt_begin=None
    self.cat_ic=None
    self.mnt_ap_set=None
    self.exp=None
    self.dt_begin=None
    self.dt_end=None
    self.nml_id=None
    self.cat_no=None
    self.image_fn=None
    # a bit a murcks
    # RTS2 dummy driver has only noisy pictures
    self.d_dss=None
    if fetch_dss_image:
        self.d_dss=DeviceDss(lg=self.lg,obs=self.obs,px_scale=self.px_scale,ccd_size=self.ccd_size,base_path=self.base_path,fetch_dss_image=self.fetch_dss_image)

    self.exposure_attempts=0
    self.modulo=9
    
  def check_presence(self):
    cont=True
    try:
      self.mnt_nm = self.getDeviceByType(scriptcomm_3.DEVICE_TELESCOPE)
      self.lg.debug('check_presence: mount device: {}'.format(self.mnt_nm))
    except Exception as e:
      self.lg.error('check_presence: could not find telescope')
      self.lg.error('check_presence: exception: {}'.format(e))
      cont= False
    try:
      self.ccd_nm = self.getDeviceByType(scriptcomm_3.DEVICE_CCD)
      self.lg.debug('check_presence: CCD device: {}'.format(self.ccd_nm))
    except Exception as e:
      self.lg.error('check_presence: could not find CCD')
      self.lg.error('check_presence: exception: {}'.format(e))
      cont= False
    
    return cont
  
  def fetch_mount_position(self):
    #  self.cat_ic is None!
    now=Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date_hms')
    ras,decs=self.getValue('TEL',self.mnt_nm).split()
    self.lg.debug('fetch_mount_position: TEL ra: {0:.3f},: dec: {1:.3f}'.format(float(ras),float(decs)))
    # ToDo strictly not correct ?mnt_ci (CIRS)
    mnt_ic=SkyCoord(ra=float(ras),dec=float(decs), unit=(u.degree,u.degree), frame='icrs',obstime=now,location=self.obs)
    return mnt_ic
  
  def ccd_init(self):
    # full area
    self.setValue('WINDOW','%d %d %d %d' % (-1, -1, -1, -1))
    x_0,y_0,widths,heights=self.getValue('WINDOW',self.ccd_nm).split()
    self.lg.debug('acquire: CCD size: {},{}'.format(widths,heights))
    # make sure we are taking light images..
    self.setValue('SHUTTER','LIGHT')

  def __expose(self,exp=None):
    # ToDo check if Morvian can be read out in parallel (RTS2, VM Windows)
    # ToDo if not use maa-2015-10-18.py
    # RTS2 does synchronization mount/CCD
    self.lg.debug('expose: {0:6.3f}'.format(self.exp))
    self.setValue('exposure',self.exp)
    self.dt_begin = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date_hms')
    rts2_image_ptfn = self.exposure()
    self.dt_end = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date_hms')
    self.lg.debug('expose: image from RTS2: {}'.format(rts2_image_ptfn))
    # fetch image from DSS (simulation only)
    if self.fetch_dss_image:
      try:
          os.unlink(rts2_image_ptfn) # the noisy CCD image
          self.lg.debug('expose: unlinking image from RTS2: {}'.format(rts2_image_ptfn))
      except:
          pass # do not care

      self.d_dss.store_mount_data(cat_ic=self.cat_ic,nml_id=self.nml_id,cat_no=self.cat_no)
      self.exp=self.d_dss.ccd_expose(exp=self.exp)
      self.image_fn=self.d_dss.dss_image_fn # only fn
      self.image_ptfn=os.path.join(self.base_path,self.image_fn)
      self.lg.debug('expose: image from DSS: {}'.format(self.image_fn))
    else:
      self.image_fn=os.path.basename(rts2_image_ptfn)
      self.image_ptfn=rts2_image_ptfn
      
  def ccd_expose(self,exp=None):
    self.exp=exp
    if self.quick_analysis is not None:
      first=True
      while True:  
        self.__expose()
        self.exp,redo=self.quick_analysis.analyze(nml_id=self.nml_id,exposure_last=self.exp,ptfn=self.image_ptfn)
        # simulation mode
        #if self.fetch_dss_image:
        #  break
        if self.exp <= self.quick_analysis.exposure_interval[0]:
          self.lg.warn('ccd_expose: giving up, exposure time reached lower limit: {}'.format(self.quick_analysis.exposure_interval[0]))
          if first:
            first=False
          else:
            break
        if self.exp >= self.quick_analysis.exposure_interval[1]:
          self.lg.warn('ccd_expose: giving up, exposure time reached upper limit: {}'.format(self.quick_analysis.exposure_interval[1]))
          if first:
            first=False
          else:
            break
        if not redo:
          break
        else:
          self.lg.warn('ccd_expose: re doing nml_id: {}'.format(self.nml_id))
          
        self.exposure_attempts +=1
        if not  self.exposure_attempts % self.modulo:
          self.lg.warn('ccd_expose: giving up after: {} attempts'.format(self.modulo))
          break
    else:
      self.exp=self.__expose(exp=self.exp)

  def store_mount_data(self,cat_ic=None,nml_id=None,cat_no=None):
    self.nml_id=nml_id
    self.cat_no=cat_no
    # ToDo transform from cat_ic to cat_ap and disable the corrections in RTS2 mount
    self.cat_ic=cat_ic
    self.mnt_ap_set=cat_ic
    
    self.setValue('ORI','{0} {1}'.format(self.cat_ic.ra.degree,self.cat_ic.dec.degree),self.mnt_nm)
    ra_oris,dec_oris=self.getValue('ORI',self.mnt_nm).split()
    self.lg.debug('ORI: {0:.3f},{1:.3f}'.format(float(ra_oris),float(dec_oris)))
    # ToD return
    
  def fetch_mount_data(self):
    if self.cat_ic is None:
      return None
    
    JD=float(self.getValue('JD',self.mnt_nm))
    now = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date')
    ras,decs=self.getValue('TEL',self.mnt_nm).split()
    # ToDo in RTS2: ORI + WOFFS + ...=TEL
    # all corrections must be turned off
    # see cat_ic above!
    mnt_ra_rdb=SkyCoord(ra=float(ras),dec=float(decs), unit=(u.degree,u.degree), frame='icrs',obstime=now,location=self.obs)

    alts,azs=self.getValue('TEL_',self.mnt_nm).split()
    # RTS2 and IAU: S=0,W=90
    # astropy: N=0,E=90               |||
    mnt_aa_rdb=SkyCoord(az=float(azs)-180.,alt=float(alts),unit=(u.degree,u.degree),frame='altaz',location=self.obs,obstime=now)

    # ToDo wrong:
    nml_aa=SkyCoord(az=float(azs),alt=float(alts),unit=(u.degree,u.degree),frame='altaz',location=self.obs,obstime=now)
    dt_end_query = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')
    sky=SkyPosition(
      nml_id=self.nml_id,
      cat_no=self.cat_no,
      nml_aa=nml_aa,
      cat_ic=self.cat_ic,
      dt_begin=self.dt_begin,
      dt_end=self.dt_end,
      dt_end_query=dt_end_query,
      JD=JD,
      mnt_ra_rdb=mnt_ra_rdb,
      mnt_aa_rdb=mnt_aa_rdb,
      image_fn=self.image_fn,
      exp=self.exp,
      mount_type_eq=self.mount_type_eq,
    )
    return sky

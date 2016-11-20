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
from astropy.coordinates import SkyCoord,EarthLocation
# ToDo unify AltAz altaz
from astropy.coordinates import AltAz
from datetime import datetime
from astropy.time import Time

# python 3 version
import scriptcomm_3
from structures import CatPosition,NmlPosition,AcqPosition,cl_nms_acq,cl_nms_anl

class DeviceDss(object):
  def __init__(
    self,
    lg=None,
    obs=None,
    px_scale=None,
    ccd_size=None,
    base_path=None,
    fetch_dss_image=None, # not used in this class
  ):
    self.lg=lg
    self.dss_base_url='http://archive.eso.org/dss/dss/'
    self.obs=obs
    self.px_scale_arcmin=px_scale *180. * 60. /np.pi # to arcmin
    self.ccd_size=ccd_size
    self.base_path=base_path
    self.cat_eq=None
    self.dss_fn=None
    self.exp=.1 
    self.dt_begin=None
    self.dt_end=None
    self.nml_id=None
    self.cat_no=None
    self.pressure=None
    self.temperature=None
    self.humidity=None
    
  def check_presence(self,**kwargs):
    return True
  
  def ccd_init(self):
    pass

  def ccd_expose(self,exp=None, pressure=None,temperature=None,humidity=None):

    self.exp=exp # not really used
    self.pressure=pressure
    self.temperature=temperature
    self.humidity=humidity
    
    width= '{}'.format(float(self.ccd_size[0]) * self.px_scale_arcmin) # DSS [arcmin]
    height= '{}'.format(float(self.ccd_size[1]) * self.px_scale_arcmin) # DSS [arcmin]
    args=OrderedDict()
    # 
    args['ra']='{0:.6f}'.format(self.cat_eq.ra.degree)
    args['dec']='{0:.6f}'.format(self.cat_eq.dec.degree)
    args['x']='{0:.2f}'.format(float(width))
    args['y']='{0:.2f}'.format(float(height))
    # compressed fits files SExtractor does not understand
    #args['mime-type']='image/x-gfits'
    args['mime-type']='image/x-fits'
    dfn='dss_{}_{}.fits'.format(args['ra'],args['dec']).replace('-','m').replace('.','_').replace('_fits','.fits') #ToDo lazy
    self.dss_fn=os.path.join(self.base_path,dfn)
    self.dt_begin = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')

    r=requests.get(self.dss_base_url, params=args, stream=True)
    self.dt_end = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')
    self.lg.debug('expose: DSS: {}'.format(r.url))
    while True:
      if r.status_code == 200:
        with open(self.dss_fn, 'wb') as f:
          #for data in tqdm(r.iter_content()):
          f.write(r.content)
          for block in r.iter_content(1024):
            f.write(block)
          break
      else:
        self.lg.warn('expose: could not retrieve DSS: {}, continuing to retrieve'.format(r.url))
    self.lg.debug('expose: DSS: saved {}'.format(r.url))
    
    return self.dss_fn,self.exp
    
  def mount_set(self,cat_eq=None,nml_id=None,cat_no=None):
    self.nml_id=nml_id
    self.cat_no=cat_no
    self.cat_eq=cat_eq
  
  def mount_retrieve_position(self):
    JD=-1.      
    acq_eq=SkyCoord(ra=self.cat_eq.ra.radian,dec=self.cat_eq.dec.radian, unit=(u.radian,u.radian), frame='icrs',obstime=self.dt_begin,location=self.obs)
    acq_eq_woffs=SkyCoord(ra=0.,dec=0., unit=(u.radian,u.radian), frame='icrs',obstime=self.dt_begin,location=self.obs)
    acq_eq_mnt=SkyCoord(ra=0.,dec=0., unit=(u.radian,u.radian), frame='icrs',obstime=self.dt_begin,location=self.obs)
    # not yet used
    acq_aa_mnt=self.cat_eq.transform_to(AltAz(location=self.obs, pressure=0.)) # no refraction here, UTC is in cat_eq
    # ToDo wrong:
    aa_nml=self.cat_eq.transform_to(AltAz(location=self.obs, pressure=0.)) # no refraction here, UTC is in cat_eq
      
    dt_end_query = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')
    acq=AcqPosition(
      nml_id=self.nml_id,
      cat_no=self.cat_no,
      aa_nml=aa_nml,
      eq=acq_eq,
      dt_begin=self.dt_begin,
      dt_end=self.dt_end,
      dt_end_query=dt_end_query,
      JD=JD,
      eq_woffs=acq_eq_woffs,
      eq_mnt=acq_eq_mnt,
      aa_mnt=acq_aa_mnt,
      image_fn=self.dss_fn,
      exp=self.exp,
      pressure=self.pressure,
      temperature=self.temperature,
      humidity=self.humidity,
    )
    return acq


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
  ):
    scriptcomm_3.Rts2Comm.__init__(self)
    self.lg=lg
    self.obs=obs
    self.base_path=base_path
    self.px_scale=px_scale
    self.ccd_size=ccd_size 

    self.mnt_nm=None
    self.ccd_nm=None
    self.dt_begin=None
    self.cat_eq=None
    self.dss_fn=None
    self.exp=None
    self.dt_begin=None
    self.dt_end=None
    self.nml_id=None
    self.cat_no=None
    self.pressure=None
    self.temperature=None
    self.humidity=None
    self.image_fn=None
    # a bit a murcks
    self.d_dss=None
    if fetch_dss_image:
        self.d_dss=DeviceDss(lg=self.lg,obs=self.obs,px_scale=self.px_scale,ccd_size=self.ccd_size,base_path=self.base_path)

    
  def check_presence(self,**kwargs):
    # Todo pythonic?    
    if kwargs is None:
      self.lg.error('expected key words: mnt_nm, ccd_nm, exiting'.format(name, value))
      sys.exit(1)
    mnt_nm=ccd_nm=None
    for name, value in kwargs.items():
      if 'mnt_nm' in name:
        mnt_nm=value
      elif 'ccd_nm' in name:
        ccd_nm=value
      else:
        self.lg.error('got unexpected key word: {0} = {1},exiting'.format(name, value))
        sys.exit(1)

    cont=True
    try:
      self.mnt_nm = self.getDeviceByType(scriptcomm_3.DEVICE_TELESCOPE)
      self.lg.debug('check_rts2_devices: mount device: {}'.format(mnt_nm))
    except:
      self.lg.error('check_rts2_devices: could not find telescope')
      cont= False
    try:
      self.ccd_nm = self.getDeviceByType(scriptcomm_3.DEVICE_CCD)
      self.lg.debug('check_rts2_devices: CCD device: {}'.format(self.ccd_nm))
    except:
      self.lg.error('check_rts2_devices: could not find CCD')
      cont= False
    
    if not cont:
      return False
    
    return True
  
  def ccd_init(self):
    # full area
    self.setValue('WINDOW','%d %d %d %d' % (-1, -1, -1, -1))
    x_0,y_0,widths,heights=self.getValue('WINDOW',self.ccd_nm).split()
    self.lg.debug('acquire: CCD size: {},{}'.format(widths,heights))
    # make sure we are taking light images..
    self.setValue('SHUTTER','LIGHT')

  def ccd_expose(self,exp=None,pressure=None,temperature=None,humidity=None):
    # ToDo check if Morvian can be read out in parallel (RTS2, VM Windows)
    # ToDo if not use maa-2015-10-18.py

    self.exp=exp
    self.pressure=pressure
    self.temperature=temperature
    self.humidity=humidity
    # RTS2 does synchronization mount/CCD
    self.lg.debug('expose: time {}'.format(self.exp))
    self.setValue('exposure',self.exp)
    self.dt_begin = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')
    self.image_fn = self.exposure()
    self.dt_end = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')
    self.lg.debug('expose: image from RTS2: {}'.format(self.image_fn))
    # fetch image from DSS
    if self.fetch_dss_image:
      try:
          os.unlink(self.image_fn) # the noisy CCD image
          self.lg.debug('expose: unlinking image from RTS2: {}'.format(self.image_fn))
      except:
          pass # do not care

      self.d_dss.mount_set(cat_eq=self.cat_eq,nml_id=self.nml_id,cat_no=self.cat_no)
      self.image_fn,self.exp=self.d_dss.ccd_expose(exp=self.exp,pressure=self.pressure,temperature=self.temperature,humidity=self.humidity)
      self.lg.debug('expose: image from DSS: {}'.format(self.image_fn))

    return self.image_fn,self.exp
  
  def mount_set(self,cat_eq=None,nml_id=None,cat_no=None):
    self.nml_id=nml_id
    self.cat_no=cat_no
    self.cat_eq=cat_eq

    self.setValue('ORI','{0} {1}'.format(self.cat_eq.ra.degree,self.cat_eq.dec.degree),self.mnt_nm)
    ra_oris,dec_oris=self.getValue('ORI',self.mnt_nm).split()
    self.lg.debug('ORI: {0:.3f},{1:.3f}'.format(float(ra_oris),float(dec_oris)))
    # ToD return
    
  def mount_retrieve_position(self):
    JD=float(self.getValue('JD',self.mnt_nm))
    ras,decs=self.getValue('ORI',self.mnt_nm).split()
    now = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='date')
    acq_eq=SkyCoord(ra=float(ras),dec=float(decs), unit=(u.degree,u.degree), frame='icrs',obstime=now,location=self.obs)
    
    ras,decs=self.getValue('WOFFS',self.mnt_nm).split()
    acq_eq_woffs=SkyCoord(ra=float(ras),dec=float(decs), unit=(u.degree,u.degree), frame='icrs',obstime=now,location=self.obs)

    ras,decs=self.getValue('TEL',self.mnt_nm).split()
    acq_eq_mnt=SkyCoord(ra=float(ras),dec=float(decs), unit=(u.degree,u.degree), frame='icrs',obstime=now,location=self.obs)

    alts,azs=self.getValue('TEL_',self.mnt_nm).split()
    acq_aa_mnt=SkyCoord(az=float(azs),alt=float(alts),unit=(u.degree,u.degree),frame='altaz',location=self.obs,obstime=now)

    # ToDo,obswl=0.5*u.micron, pressure=ln_pressure_qfe*u.hPa,temperature=ln_temperature*u.deg_C,relative_humidity=ln_humidity)
    # ToDo wrong:
    aa_nml=SkyCoord(az=float(azs),alt=float(alts),unit=(u.degree,u.degree),frame='altaz',location=self.obs,obstime=now)
    dt_end_query = Time(datetime.utcnow(), scale='utc',location=self.obs,out_subfmt='fits')
    acq=AcqPosition(
      nml_id=self.nml_id,
      cat_no=self.cat_no,
      aa_nml=aa_nml,
      eq=acq_eq,
      dt_begin=self.dt_begin,
      dt_end=self.dt_end,
      dt_end_query=dt_end_query,
      JD=JD,
      eq_woffs=acq_eq_woffs,
      eq_mnt=acq_eq_mnt,
      aa_mnt=acq_aa_mnt,
      image_fn=self.image_fn,
      exp=self.exp,
      pressure=self.pressure,
      temperature=self.temperature,
      humidity=self.humidity,
    )
    return acq

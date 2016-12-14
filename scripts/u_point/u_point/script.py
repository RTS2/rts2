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

Base class for script u_acquire.py, u_analyze.py

'''

__author__ = 'wildi.markus@bluewin.ch'

import os,time,sys
import numpy as np
from astropy import units as u
from astropy.time import Time,TimeDelta
from astropy.coordinates import SkyCoord,AltAz
import pandas as pd

from u_point.structures import NmlPosition,CatPosition,SkyPosition,cl_nms,cl_acq
import u_point.ds9region

class Script(object):
  def __init__(
      self,
      lg=None,
      break_after=None,
      base_path=None,
      obs=None,
      acquired_positions=None,
      analyzed_positions=None,
      acq_e_h=None):

    self.lg=lg
    self.break_after=break_after
    self.base_path=base_path
    self.obs=obs
    self.acquired_positions=acquired_positions
    self.analyzed_positions=analyzed_positions
    self.acq_e_h=acq_e_h

  # ToDo see callback.py
  def display_fits(self,fn=None, x=None,y=None,color=None):
    ds9=ds9region.Ds9DisplayThread(debug=True,logger=self.lg)
    # Todo: ugly
    ds9.run(fn,x=x,y=y,color=color)
      
  # ToDo goes away
  def rebase_base_path(self,ptfn=None):
    if self.base_path in ptfn:
      new_ptfn=ptfn
    else:
      fn=os.path.basename(ptfn)
      new_ptfn=os.path.join(self.base_path,fn)
      #self.lg.debug('rebase_base_path: file: {}'.format(new_ptfn))
    return new_ptfn
    
  def expand_base_path(self,fn=None):
    if self.base_path in fn:
      ptfn=fn
    else:
      ptfn=os.path.join(self.base_path,fn)
    return ptfn
  
  def to_altaz(self,ic=None):
    # http://docs.astropy.org/en/stable/api/astropy.coordinates.AltAz.html
    #  Azimuth is oriented East of North (i.e., N=0, E=90 degrees)
    # RTS2 follows IAU S=0, W=90
    return ic.transform_to(AltAz(location=self.obs, pressure=0.)) # no refraction here, UTC is in cat_ic
  # ic
  
  def to_ic(self,aa=None):
    # strictly spoken it is not ICRS if refraction was corrected in aa
    return aa.transform_to('icrs') 

  def fetch_observable_catalog(self, fn=None):
    ptfn=self.expand_base_path(fn=fn)
    self.cat_observable=list()
    df_data = self.fetch_pandas(ptfn=ptfn,columns=['cat_no','ra','dec','mag_v',],sys_exit=True,with_nml_id=False)
    if df_data is None:
      return
    for i,rw in df_data.iterrows():
      if i > self.break_after:
        break
      
      cat_ic=SkyCoord(ra=rw['ra'],dec=rw['dec'], unit=(u.radian,u.radian), frame='icrs',obstime=self.dt_utc,location=self.obs)
      self.cat_observable.append(CatPosition(cat_no=int(rw['cat_no']),cat_ic=cat_ic,mag_v=rw['mag_v'] ))

  def store_nominal_altaz(self,az_step=None,alt_step=None,azimuth_interval=None,altitude_interval=None,fn=None):
    # ToDo from pathlib import Path, fp=Path(ptfb),if fp.is_file())
    # format az_nml,alt_nml
    ptfn=self.expand_base_path(fn=fn)

    if os.path.isfile(ptfn):
      a=input('overwriting existing file: {} [N/y]'.format(ptfn))
      if a not in 'y':
        self.lg.info('exiting')
        sys.exit(0)

    # ToDo candidate for pandas
    with  open(ptfn, 'w') as wfl:
      self.nml=list()
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
          wfl.write('#{0},{1:12.6f},{2:12.6f}\n'.format(nml_id,az,alt))
          wfl.write('{},{},{}\n'.format(nml_id,azr,altr))
          nml_aa=SkyCoord(az=azr,alt=altr,unit=(u.radian,u.radian),frame='altaz',location=self.obs)
          self.nml.append(NmlPosition(nml_id=nml_id,nml_aa=nml_aa))

  def fetch_pandas(self, ptfn=None,columns=None,sys_exit=True,with_nml_id=True):
    # ToDo simplify that
    df_data=None
    if not os.path.isfile(ptfn):
      if sys_exit:
        self.lg.debug('fetch_pandas: {} does not exist, exiting'.format(ptfn))
        sys.exit(1)
      return None
    
    if os.path.getsize(ptfn)==0:
        self.lg.debug('fetch_pandas: {} file is empty, exiting'.format(ptfn))

    try:
      if with_nml_id:
        #df_data = pd.read_csv(ptfn,sep=',',names=columns,index_col=['nml_id'],header=None,engine='python')
        # above is not equivalent to (nml_id is dropped from keys)
        df_data = pd.read_csv(ptfn,sep=',',names=columns,header=None,engine='python',comment='#')
        df_data.set_index('nml_id')
        
      else:
        df_data = pd.read_csv(ptfn,sep=',',names=columns,header=None,engine='python')
    except ValueError as e:
      self.lg.debug('fetch_pandas: {}, ValueError: {}, columnns: {}'.format(ptfn,e,columns))
      return None
    except OSError as e:
      self.lg.debug('fetch_pandas: {}, OSError: {}'.format(ptfn, e))
      return None
    except Exception as e:
      self.lg.debug('fetch_pandas: {}, Exception: {}, exiting'.format(ptfn, e))
      sys.exit(1)
        
    if with_nml_id:
      return df_data.sort_index()
    else:
      return df_data
        
  def fetch_nominal_altaz(self,fn=None,sys_exit=True):
    ptfn=self.expand_base_path(fn=fn)
    self.nml=list()
    df_data = self.fetch_pandas(ptfn=ptfn,columns=['nml_id','az','alt'],sys_exit=sys_exit,with_nml_id=True)
    if df_data is None:
      return

    for i,rw in df_data.iterrows():
      nml_aa=SkyCoord(az=rw['az'],alt=rw['alt'],unit=(u.radian,u.radian),frame='altaz',location=self.obs)
      self.nml.append(NmlPosition(nml_id=df_data.index[i],nml_aa=nml_aa))

  def drop_nominal_altaz(self):
    for nml_id in [int(x.nml_id)  for x in self.sky_acq]:
      self.nml[nml_id].nml_aa=None
      #self.lg.debug('drop_nominal_altaz: deleted: {}'.format(i))

  def distinguish(self,analyzed=None):
    if analyzed:
      ptfn=self.expand_base_path(fn=self.analyzed_positions)
    else:
      ptfn=self.expand_base_path(fn=self.acquired_positions)

    return ptfn
 
  def delete_one_position(self, nml_id=None,analyzed=None):
    ptfn=self.distinguish(analyzed=analyzed)
    # ToDo something like: i=list.index()??
    self.fetch_positions(sys_exit=True,analyzed=analyzed)
    if analyzed:
      sky_a=self.sky_anl        
    else:
      sky_a=self.sky_acq
    
    for i,sk in enumerate(sky_a):
      if nml_id==sk.nml_id:
        del sky_a[i]
        break
    else:
      self.lg.info('delete_one_position:  nml_id {} not found in file: {}'.format(nml_id, ptfn))
      return
    
    self.store_positions(analyzed=analyzed)
    self.lg.info('delete_one_position deleted nml_id: {} from file: {}'.format(nml_id, ptfn))

  def store_positions(self,analyzed=None):
    ptfn=self.distinguish(analyzed=analyzed)
    if self.acq_e_h is not None:  
      while self.acq_e_h.not_writable:
        self.lg.debug('waiting for file')
        time.sleep(.1)      
    if analyzed:
      sky_a=self.sky_anl        
    else:
      sky_a=self.sky_acq
    # append, one by one
    with  open(ptfn, 'w') as wfl:
      for ps in sky_a:
        wfl.write('{0}\n'.format(ps))
    
  def append_position(self,sky=None,analyzed=None):
    ptfn=self.distinguish(analyzed=analyzed)
    # append, one by one
    with  open(ptfn, 'a') as wfl:
      wfl.write('{0}\n'.format(sky))

  def fetch_positions(self,sys_exit=None,analyzed=None):
    # ToDo!
    # dt_utc=dt_utc - TimeDelta(rw['exp']/2.,format='sec') # exp. time is small
    if analyzed:
      sky_a=self.sky_anl=list()
    else:
      sky_a=self.sky_acq=list()


    ptfn=self.distinguish(analyzed=analyzed)
      
    cols=cl_nms
    df_data = self.fetch_pandas(ptfn=ptfn,columns=cols,sys_exit=sys_exit,with_nml_id=True)
    if df_data is None:
      return
    
    for i,rw in df_data.iterrows():
      # ToDo why not out_subfmt='fits'
      dt_begin=Time(rw['dt_begin'],format='iso', scale='utc',location=self.obs,out_subfmt='date_hms')
      dt_end=Time(rw['dt_end'],format='iso', scale='utc',location=self.obs,out_subfmt='date_hms')
      dt_end_query=Time(rw['dt_end_query'],format='iso', scale='utc',location=self.obs,out_subfmt='date_hms')

      # ToDo set best time point
      nml_aa=SkyCoord(az=rw['nml_aa_az'],alt=rw['nml_aa_alt'],unit=(u.radian,u.radian),frame='altaz',location=self.obs,obstime=dt_end)
      cat_ic=SkyCoord(ra=rw['cat_ic_ra'],dec=rw['cat_ic_dec'], unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
      # replace icrs by cirs (intermediate frame, mount apparent coordinates)
      mnt_ra_rdb=SkyCoord(ra=rw['mnt_ra_rdb_ra'],dec=rw['mnt_ra_rdb_dec'], unit=(u.radian,u.radian), frame='cirs',obstime=dt_end,location=self.obs)
      # ToDo: replace above mnt_ic with mnt_ci the transform to mnt_aa_transformed
      #       and do mnt_aa_transformed - mnt_aa
      mnt_aa_rdb=SkyCoord(az=rw['mnt_aa_rdb_az'],alt=rw['mnt_aa_rdb_alt'],unit=(u.radian,u.radian),frame='altaz',location=self.obs,obstime=dt_end)
      mount_type_eq=rw['mount_type_eq']
      if 'cat_ll_ap_lon' in cols and pd.notnull(rw['cat_ll_ap_lon']) and pd.notnull(rw['cat_ll_ap_lat']):
        if mount_type_eq:
          # ToDO icrs, cirs
          cat_ll_ap=SkyCoord(ra=rw['cat_ll_ap_lon'],dec=rw['cat_ll_ap_lat'], unit=(u.radian,u.radian), frame='cirs',obstime=dt_end,location=self.obs)
        else:
          cat_ll_ap=SkyCoord(az=rw['cat_ll_ap_lon'],alt=rw['cat_ll_ap_lat'], unit=(u.radian,u.radian), frame='altaz',obstime=dt_end,location=self.obs)
      else:
        cat_ll_ap=None
        
      if 'mnt_ll_sxtr_lon' in cols and pd.notnull(rw['mnt_ll_sxtr_lon']) and pd.notnull(rw['mnt_ll_sxtr_lat']):
        if mount_type_eq:
          # ToDO icrs, cirs
          mnt_ll_sxtr=SkyCoord(ra=rw['mnt_ll_sxtr_lon'],dec=rw['mnt_ll_sxtr_lat'], unit=(u.radian,u.radian), frame='cirs',obstime=dt_end,location=self.obs)
        else:
          mnt_ll_sxtr=SkyCoord(az=rw['mnt_ll_sxtr_lon'],alt=rw['mnt_ll_sxtr_lat'], unit=(u.radian,u.radian), frame='altaz',obstime=dt_end,location=self.obs)
      else:
        mnt_ll_sxtr=None

      if 'mnt_ll_astr_lon' in cols and pd.notnull(rw['mnt_ll_astr_lon']) and pd.notnull(rw['mnt_ll_astr_lat']):
        if mount_type_eq:
          # ToDO icrs, cirs
          mnt_ll_astr=SkyCoord(ra=rw['mnt_ll_astr_lon'],dec=rw['mnt_ll_astr_lat'], unit=(u.radian,u.radian), frame='cirs',obstime=dt_end,location=self.obs)
        else:
          mnt_ll_astr=SkyCoord(az=rw['mnt_ll_astr_lon'],alt=rw['mnt_ll_astr_lat'], unit=(u.radian,u.radian), frame='altaz',obstime=dt_end,location=self.obs)
      else:
        mnt_ll_astr=None
      
      image_ptfn=self.rebase_base_path(ptfn=rw['image_fn'])
      s_sky=SkyPosition(
          nml_id=rw['nml_id'], # there might be holes
          cat_no=rw['cat_no'],
          nml_aa=nml_aa,
          cat_ic=cat_ic,
          dt_begin=dt_begin,
          dt_end=dt_end,
          dt_end_query=dt_end_query,
          JD=rw['JD'],
          mnt_ra_rdb=mnt_ra_rdb,
          mnt_aa_rdb=mnt_aa_rdb,
          image_fn=image_ptfn,
          exp=rw['exp'],
          pressure=rw['pressure'],
          temperature=rw['temperature'],
          humidity=rw['humidity'],
          mount_type_eq=mount_type_eq,
          transform_name=rw['transform_name'],
          refraction_method=rw['refraction_method'],
          cat_ll_ap=cat_ll_ap,
          mnt_ll_sxtr=mnt_ll_sxtr,
          mnt_ll_astr=mnt_ll_astr,
      )
      sky_a.append(s_sky)
  
  def fetch_mount_meteo(self,sys_exit=None,analyzed=None,with_nml_id=False):
    # --fit-sxtr !
    # test purpose only
    # ToDo!
    # dt_utc=dt_utc - TimeDelta(rw['exp']/2.,format='sec') # exp. time is small
    if analyzed:
      sky_a=self.sky_anl=list()
    else:
      sky_a=self.sky_acq=list()

    ptfn='./mount_data_meteo.txt'
      
    cols= [
      'dt_end',#7
      #  'nml_id',#0
      #  'cat_no',#1
      #  'nml_aa_az',#2
      #  'nml_aa_alt',#3
      'cat_ic_ra',#4
      'cat_ic_dec',#5
      #  'dt_begin',#6
      #  'dt_end',#7
      #  'dt_end_query',#8
      #  'JD',#9
      #  'cat_ic_woffs_ra',#10
      #  'cat_ic_woffs_dec',#11
      ##'mnt_ic_ra',#12
      ##'mnt_ic_dec',#13
      'astr_ra',#23
      'astr_dec',#24
      #  'mnt_aa_az',#14
      #  'mnt_aa_alt',#15
      #  'image_fn',#16
      'exp',#17
      'pressure',#18
      'temperature',#19
      'humidity',#20
      #  'sxtr_ra',#21
      #  'sxtr_dec',#22
    ]

    df_data = self.fetch_pandas(ptfn=ptfn,columns=cols,sys_exit=sys_exit,with_nml_id=with_nml_id)
    if df_data is None:
      return
    
    for i,rw in df_data.iterrows():
      # ToDo why not out_subfmt='fits'
      dt_end=Time(rw['dt_end'],format='iso', scale='utc',location=self.obs,out_subfmt='date_hms')
      # ToDo set best time point
      cat_ic=SkyCoord(ra=rw['cat_ic_ra'],dec=rw['cat_ic_dec'], unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
      # replace icrs by cirs (intermediate frame, mount apparent coordinates)
      mnt_ic=SkyCoord(ra=rw['mnt_ic_ra'],dec=rw['mnt_ic_dec'], unit=(u.radian,u.radian), frame='cirs',obstime=dt_end,location=self.obs)
      
      #image_ptfn=self.rebase_base_path(ptfn=rw['image_fn'])
      s_sky=SkyPosition(
          nml_id=i, # there might be holes
          cat_no=None,
          nml_aa=None,
          cat_ic=cat_ic,
          dt_begin=None,
          dt_end=dt_end,
          dt_end_query=None,
          JD=None,
          cat_ic_woffs=None,
          mnt_ic=mnt_ic,
          mnt_aa=None,
          image_fn=None,
          exp=rw['exp'],
          pressure=rw['pressure'],
          temperature=rw['temperature'],
          humidity=rw['humidity'],
          mount_type_eq=False,
          sxtr= mnt_ic,
          astr= None,
      )
      sky_a.append(s_sky)

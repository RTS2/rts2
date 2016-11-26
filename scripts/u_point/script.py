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

from structures import NmlPosition,CatPosition,AcqPosition,AnlPosition,cl_nms_acq,cl_nms_anl

class Script(object):
  def __init__(
      self,
      lg=None,
      break_after=None,
      base_path=None,
      obs=None,
      acquired_positions=None,
      analyzed_positions=None,
      acq_e_h=None,):

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
      
  def to_altaz(self,eq=None):
    # http://docs.astropy.org/en/stable/api/astropy.coordinates.AltAz.html
    #  Azimuth is oriented East of North (i.e., N=0, E=90 degrees)
    # RTS2 follows IAU S=0, W=90
    return eq.transform_to(AltAz(location=self.obs, pressure=0.)) # no refraction here, UTC is in cat_eq

  def to_eq(self,aa=None):
    return aa.transform_to('icrs') 

  def expand_base_path(self,fn=None):
    if self.base_path in fn:
      ptfn=fn
    else:
      ptfn=os.path.join(self.base_path,fn)
    return ptfn
  
  def fetch_observable_catalog(self, fn=None):
    ptfn=self.expand_base_path(fn=fn)
    self.cat=list()
    df_data = self.fetch_pandas(ptfn=ptfn,columns=['cat_no','ra','dec','mag_v',],sys_exit=True,with_nml_id=False)
    if df_data is None:
      return
    for i,rw in df_data.iterrows():
      if i > self.break_after:
        break
      
      cat_eq=SkyCoord(ra=rw['ra'],dec=rw['dec'], unit=(u.radian,u.radian), frame='icrs',obstime=self.dt_utc,location=self.obs)
      self.cat.append(CatPosition(cat_no=int(rw['cat_no']),cat_eq=cat_eq,mag_v=rw['mag_v'] ))

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
          wfl.write('{},{},{}\n'.format(nml_id,azr,altr))
          aa_nml=SkyCoord(az=azr,alt=altr,unit=(u.radian,u.radian),frame='altaz',location=self.obs)
          self.nml.append(NmlPosition(nml_id=nml_id,aa_nml=aa_nml))

  def fetch_pandas(self, ptfn=None,columns=None,sys_exit=True,with_nml_id=True):
    # ToDo simplify that
    df_data=None
    if not os.path.isfile(ptfn):
      if sys_exit:
        self.lg.debug('fetch_pandas: {} does not exist, exiting'.format(ptfn))
        sys.exit(1)
      return None
    if with_nml_id:
      try:
        df_data = pd.read_csv(ptfn,sep=',',names=columns,index_col='nml_id',header=None)
      except ValueError as e:
        self.lg.debug('fetch_pandas: {}, ValueError: {}, columnns: {}'.format(ptfn,e,columns))
        return None
      except OSError as e:
        self.lg.debug('fetch_pandas: {}, OSError: {}'.format(ptfn, e))
        return None
      except Exception as e:
        self.lg.debug('fetch_pandas: {}, Exceptio: {}, exiting'.format(ptfn, e))
        sys.exit(1)
        
      return df_data.sort_index()
    else:
      try:
        df_data = pd.read_csv(ptfn,sep=',',names=columns,header=None)
      except ValueError as e:
        self.lg.debug('fetch_pandas: {}, ValueError: {}, columnns: {}'.format(ptfn,e,columns))
        return None
      except OSError as e:
        self.lg.debug('fetch_pandas: {}, OSError: {}'.format(ptfn, e))
        return None
      except Exception as e:
        self.lg.debug('fetch_pandas: {}, Exceptio: {}, exiting'.format(ptfn, e))
        sys.exit(1)
      return df_data
        
  def fetch_nominal_altaz(self,fn=None):
    ptfn=self.expand_base_path(fn=fn)
    self.nml=list()
    df_data = self.fetch_pandas(ptfn=ptfn,columns=['nml_id','az','alt'],sys_exit=True)
    if df_data is None:
      return

    for i,rw in df_data.iterrows():
      aa_nml=SkyCoord(az=rw['az'],alt=rw['alt'],unit=(u.radian,u.radian),frame='altaz',location=self.obs)
      self.nml.append(NmlPosition(nml_id=i,aa_nml=aa_nml))

  def drop_nominal_altaz(self):
    obs=[int(x.nml_id)  for x in self.acq]
    observed=sorted(set(obs),reverse=True)
    for i in observed:
      del self.nml[i]
      #self.lg.debug('drop_nominal_altaz: deleted: {}'.format(i))
  
  def fetch_acquired_positions(self,sys_exit=None):

    if self.acq_e_h is not None:  
      while self.acq_e_h.not_writable:
        time.sleep(.1)
      
    ptfn=self.expand_base_path(fn=self.acquired_positions)
    self.acq=list()
    df_data = self.fetch_pandas(ptfn=ptfn,columns=cl_nms_acq,sys_exit=sys_exit)
    if df_data is None:
      return
    for i,rw in df_data.iterrows():
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
        nml_id=i,
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

  def delete_one_acquired_position(self, nml_id=None):
    self.fetch_acquired_positions()
    for i,acq in enumerate(self.acq): 
      if nml_id==acq.nml_id:
        del self.acq[i]
        break
    self.store_acquired_positions()
    self.lg.info('deleted item: {} from file: {}'.format(nml_id, self.acquired_positions))
  # all acqs
  def store_acquired_positions(self,acq=None):
    if self.acq_e_h is not None:  
      while self.acq_e_h.not_writable:
        time.sleep(.1)
      
    ptfn=self.expand_base_path(fn=self.acquired_positions)
    # append, one by one
    with  open(ptfn, 'w') as wfl:
      for acq in self.acq:
        wfl.write('{0}\n'.format(acq))
  # append
  def append_acquired_position(self,acq=None):
    
    ptfn=self.expand_base_path(fn=self.acquired_positions)
    # append, one by one
    with  open(ptfn, 'a') as wfl:
      wfl.write('{0}\n'.format(acq))

  def delete_one_analyzed_position(self, nml_id=None):
    self.fetch_analyzed_positions()
    for i,anl in enumerate(self.anl): 
      if nml_id==anl.nml_id:
        del self.anl[i]
        break
    self.store_analyzed_positions()
    self.lg.info('deleted item: {} from file: {}'.format(nml_id, self.analyzed_positions))

  def store_analyzed_positions(self,anl=None):
    if self.acq_e_h is not None:  
      while self.acq_e_h.not_writable:
        time.sleep(.1)
      
    ptfn=self.expand_base_path(fn=self.analyzed_positions)
    # append, one by one
    with  open(ptfn, 'w') as wfl:
      for anl in self.anl:
        wfl.write('{0}\n'.format(anl))
    
  def append_analyzed_position(self,acq=None,sxtr_ra=None,sxtr_dec=None,astr_ra=None,astr_dec=None):
    ptfn=self.expand_base_path(fn=self.analyzed_positions)
    # append, one by one
    with  open(ptfn, 'a') as wfl:
      wfl.write('{},{},{},{},{}\n'.format(acq,sxtr_ra,sxtr_dec,astr_ra,astr_dec))

  def fetch_analyzed_positions(self,sys_exit=None):
    # ToDo!
    # dt_utc=dt_utc - TimeDelta(rw['exp']/2.,format='sec') # exp. time is small

    ptfn=self.expand_base_path(fn=self.analyzed_positions)
    df_data = self.fetch_pandas(ptfn=ptfn,columns=cl_nms_anl,sys_exit=sys_exit)
    self.anl=list()
    if df_data is None:
      return
    self.anl=list()
    
    for i,rw in df_data.iterrows():
      # ToDo why not out_subfmt='fits'
      dt_begin=Time(rw['dt_begin'],format='iso', scale='utc',location=self.obs,out_subfmt='date_hms')
      dt_end=Time(rw['dt_end'],format='iso', scale='utc',location=self.obs,out_subfmt='date_hms')
      dt_end_query=Time(rw['dt_end_query'],format='iso', scale='utc',location=self.obs,out_subfmt='date_hms')

      # ToDo set best time point
      aa_nml=SkyCoord(az=rw['aa_nml_az'],alt=rw['aa_nml_alt'],unit=(u.radian,u.radian),frame='altaz',location=self.obs,obstime=dt_end)
      acq_eq=SkyCoord(ra=rw['eq_ra'],dec=rw['eq_dec'], unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
      acq_eq_woffs=SkyCoord(ra=rw['eq_woffs_ra'],dec=rw['eq_woffs_dec'], unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
      # replace icrs by cirs (intermediate frame, mount apparent coordinates)
      acq_eq_mnt=SkyCoord(ra=rw['eq_mnt_ra'],dec=rw['eq_mnt_dec'], unit=(u.radian,u.radian), frame='cirs',obstime=dt_end,location=self.obs)
      acq_aa_mnt=SkyCoord(az=rw['aa_mnt_az'],alt=rw['aa_mnt_alt'],unit=(u.radian,u.radian),frame='altaz',location=self.obs,obstime=dt_end)

      if pd.notnull(rw['sxtr_ra']) and pd.notnull(rw['sxtr_dec']):
        # ToDO icrs, cirs
        sxtr_eq=SkyCoord(ra=rw['sxtr_ra'],dec=rw['sxtr_dec'], unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
      else:
        sxtr_eq=None
        self.lg.debug('fetch_positions: sxtr None')
        
      if pd.notnull(rw['astr_ra']) and pd.notnull(rw['astr_dec']):
        # ToDO icrs, cirs
        astr_eq=SkyCoord(ra=rw['astr_ra'],dec=rw['astr_dec'], unit=(u.radian,u.radian), frame='icrs',obstime=dt_end,location=self.obs)
      else:
        astr_eq=None
        #self.lg.debug('fetch_positions: astr None,nml_d: {}, ra:{}, dec: {}'.format(i,rw['astr_ra'],rw['astr_dec']))
        # to create more or less identical plots:
        #continue
        
      anls=AnlPosition(
          nml_id=i,
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

    

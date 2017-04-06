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

positions classes for u_point

'''

__author__ = 'wildi.markus@bluewin.ch'

import numpy as np
from astropy.coordinates.representation import SphericalRepresentation

# http://scipy-cookbook.readthedocs.io/items/FittingData.html#simplifying-the-syntax
# This is the real big relief
class Parameter:
  def __init__(self, value):
    self.value = value
  def set(self, value):
    self.value = value
  def __call__(self):
    return self.value

# data structure
# u_point.py
class Point(object):
  def __init__(self,cat_lon=None,cat_lat=None,mnt_lon=None,mnt_lat=None,df_lat=None,df_lon=None,res_lat=None,res_lon=None,image_fn=None,nml_id=None):
    self.cat_lon=cat_lon
    self.cat_lat=cat_lat
    self.mnt_lon=mnt_lon
    self.mnt_lat=mnt_lat
    self.df_lat=df_lat
    self.df_lon=df_lon
    self.res_lat=res_lat
    self.res_lon=res_lon
    self.image_fn=image_fn
    self.nml_id=nml_id

class CatPosition(object):
  def __init__(self, cat_no=None,cat_ic=None,mag_v=None):
    self.cat_no=cat_no
    self.cat_ic=cat_ic
    self.mag_v=mag_v
    
# ToDo may be only a helper
class NmlPosition(object):
  def __init__(self, nml_id=None,nml_aa=None,count=1):
    self.nml_id=nml_id
    self.nml_aa=nml_aa # nominal position (grid created with store_nominal_altaz())
    self.count=count

class SkyPosition(object):
  def __init__(
      self,
      nml_id=None,
      cat_no=None,
      nml_aa=None,
      cat_ic=None,
      dt_begin=None,
      dt_end=None,
      dt_end_query=None,
      JD=None,
      mnt_ra_rdb=None,
      mnt_aa_rdb=None,
      image_fn=None,
      exp=None,
      pressure=None,
      temperature=None,
      humidity=None,
      eq_mount=None,
      transform_name='no_transform',
      refraction_method='no_refraction',
      cat_ll_ap=None,
      mnt_ll_sxtr=None,
      mnt_ll_astr=None):
    
    self.nml_id=nml_id
    self.cat_no=cat_no
    self.nml_aa=nml_aa # nominal position (grid created with store_nominal_altaz())
    self.cat_ic=cat_ic         # set catalog position, read back from variable ORI (rts2-mon) 
    self.dt_begin=dt_begin 
    self.dt_end=dt_end
    self.dt_end_query=dt_end_query
    self.JD=JD
    # ToDo may wrong name
    self.mnt_ra_rdb=mnt_ra_rdb # TEL, read back from encodes
    self.mnt_aa_rdb=mnt_aa_rdb # TEL_ altaz 
    self.image_fn=image_fn
    self.exp=exp
    self.pressure=pressure
    self.temperature=temperature
    self.humidity=humidity
    self.eq_mount=eq_mount
    self.transform_name=transform_name
    self.refraction_method=refraction_method
    self.cat_ll_ap=cat_ll_ap
    self.mnt_ll_sxtr=mnt_ll_sxtr
    self.mnt_ll_astr=mnt_ll_astr
    
  # ToDo still ugly
  def __str__(self):
    # ToDo a bit ugly, think about that
    # to compare sxtr and astr values sxtr is in RA,Dec!
    if self.cat_ll_ap is None:
      cat_ll_ap_lon=np.nan
      cat_ll_ap_lat=np.nan
    else:
      scat_ll_ap=self.cat_ll_ap.represent_as(SphericalRepresentation)
      cat_ll_ap_lon=scat_ll_ap.lon.radian
      cat_ll_ap_lat=scat_ll_ap.lat.radian

    if self.mnt_ll_sxtr is None:
      mnt_ll_sxtr_lon=np.nan
      mnt_ll_sxtr_lat=np.nan
    else:
      smnt_ll_sxtr=self.mnt_ll_sxtr.represent_as(SphericalRepresentation)
      mnt_ll_sxtr_lon=smnt_ll_sxtr.lon.radian
      mnt_ll_sxtr_lat=smnt_ll_sxtr.lat.radian
      
    if self.mnt_ll_astr is None:
      mnt_ll_astr_lon=np.nan
      mnt_ll_astr_lat=np.nan
    else:
      smnt_ll_astr=self.mnt_ll_astr.represent_as(SphericalRepresentation)
      mnt_ll_astr_lon=smnt_ll_astr.lon.radian
      mnt_ll_astr_lat=smnt_ll_astr.lat.radian

    cmt_str='# az: {0:12.6f},alt:{1:12.6f},icrs ra:{2:12.6f},dec:{3:12.6f}\n'.format(self.nml_aa.az.degree, self.nml_aa.alt.degree, self.cat_ic.ra.degree,self.cat_ic.dec.degree)
      
    anl_str='{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},{13},{14},{15},{16},{17},{18},{19},{20},{21},{22},{23},{24},{25},{26},{27}'.format(
      self.nml_id,#0
      self.cat_no,#1
      self.nml_aa.az.radian,#2
      self.nml_aa.alt.radian,#3
      self.cat_ic.ra.radian,#4
      self.cat_ic.dec.radian,#5
      self.dt_begin,#6
      self.dt_end,#7
      self.dt_end_query,#8
      self.JD,#9
      self.mnt_ra_rdb.ra.radian,#10
      self.mnt_ra_rdb.dec.radian,#11
      self.mnt_aa_rdb.az.radian,#12
      self.mnt_aa_rdb.alt.radian,#13
      self.image_fn,#14
      self.exp,#15
      self.pressure,#16
      self.temperature,#17
      self.humidity,#18
      self.eq_mount,#19
      self.transform_name,#20
      self.refraction_method,#21
      cat_ll_ap_lon,#22
      cat_ll_ap_lat,#23
      mnt_ll_sxtr_lon,#24
      mnt_ll_sxtr_lat,#25
      mnt_ll_astr_lon,#26
      mnt_ll_astr_lat,#27
    )
    return cmt_str + anl_str

# used for pandas
cl_nms= [
  'nml_id',#0
  'cat_no',#1
  'nml_aa_az',#2
  'nml_aa_alt',#3
  'cat_ic_ra',#4
  'cat_ic_dec',#5
  'dt_begin',#6
  'dt_end',#7
  'dt_end_query',#8
  'JD',#9
  'mnt_ra_rdb_ra',#10
  'mnt_ra_rdb_dec',#11
  'mnt_aa_rdb_az',#12
  'mnt_aa_rdb_alt',#13
  'image_fn',#14
  'exp',#15
  'pressure',#16
  'temperature',#17
  'humidity',#18
  'eq_mount',#19
  'transform_name',#20
  'refraction_method',#21
  'cat_ll_ap_lon',#22
  'cat_ll_ap_lat',#23
  'mnt_ll_sxtr_lon',#24
  'mnt_ll_sxtr_lat',#25
  'mnt_ll_astr_lon',#26
  'mnt_ll_astr_lat',#27
]
# legacy, will go away
cl_acq= [
  'nml_id',#0
  'cat_no',#1
  'nml_aa_az',#2
  'nml_aa_alt',#3
  'cat_ic_ra',#4
  'cat_ic_dec',#5
  'dt_begin',#6
  'dt_end',#7
  'dt_end_query',#8
  'JD',#9
  'mnt_ic_ra',#10
  'mnt_ic_dec',#11
  'mnt_aa_az',#12
  'mnt_aa_alt',#13
  'image_fn',#14
  'exp',#15
  'pressure',#16
  'temperature',#17
  'humidity',#28
]


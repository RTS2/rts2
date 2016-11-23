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
class Point(object):
  def __init__(self,cat_lon=None,cat_lat=None,mnt_lon=None,mnt_lat=None,df_lat=None,df_lon=None,res_lat=None,res_lon=None,image_fn=None):
    self.cat_lon=cat_lon
    self.cat_lat=cat_lat
    self.mnt_lon=mnt_lon
    self.mnt_lat=mnt_lat
    self.df_lat=df_lat
    self.df_lon=df_lon
    self.res_lat=res_lat
    self.res_lon=res_lon
    self.image_fn=image_fn

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
               pressure=None,
               temperature=None,
               humidity=None,
  ):
    self.nml_id=nml_id
    self.cat_no=cat_no
    self.aa_nml=aa_nml # nominal position (grid created with store_nominal_altaz())
    self.eq=eq         # set catalog position, read back from variable ORI (rts2-mon) 
    self.dt_begin=dt_begin 
    self.dt_end=dt_end
    self.dt_end_query=dt_end_query
    self.JD=JD
    self.eq_woffs=eq_woffs # OFFS, offsets set manually
    self.eq_mnt=eq_mnt # TEL, read back from encodes
    self.aa_mnt=aa_mnt # TEL_ altaz 
    self.image_fn=image_fn
    self.exp=exp
    self.pressure=pressure
    self.temperature=temperature
    self.humidity=humidity

  # ToDo still ugly
  def __str__(self):
    acq_str='{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},{13},{14},{15},{16},{17},{18},{19},{20}'.format(
      self.nml_id,
      self.cat_no,#1
      self.aa_nml.az.radian,#2
      self.aa_nml.alt.radian,#3
      self.eq.ra.radian,#4
      self.eq.dec.radian,#5
      self.dt_begin,#6
      self.dt_end,#7
      self.dt_end_query,#8
      self.JD,#9
      self.eq_woffs.ra.radian,#10
      self.eq_woffs.dec.radian,#11
      self.eq_mnt.ra.radian,#12
      self.eq_mnt.dec.radian,#13
      self.aa_mnt.az.radian,#14
      self.aa_mnt.alt.radian,#15
      self.image_fn,#16
      self.exp,#17
      self.pressure, 
      self.temperature, 
      self.humidity,#20 
    )
    return acq_str

class AnlPosition(AcqPosition):
  def __init__(
      self,
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
      pressure=None,
      temperature=None,
      humidity=None,
      sxtr=None,
      astr=None):
    
    AcqPosition.__init__(
      self,
      nml_id=nml_id,
      cat_no=cat_no,
      aa_nml=aa_nml,
      eq=eq,
      dt_begin=dt_begin,
      dt_end=dt_end,
      dt_end_query=dt_end_query,
      JD=JD,
      eq_woffs=eq_woffs,
      eq_mnt=eq_mnt,
      aa_mnt=aa_mnt,
      image_fn=image_fn,
      exp=exp,
      pressure=pressure,
      temperature=temperature,
      humidity=humidity,
    )

    self.sxtr=sxtr
    self.astr=astr
  # ToDo still ugly
  def __str__(self):
    anl_str='{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},{13},{14},{15},{16},{17},{18},{19},{20,{21},{22},{23},{24}'.format(
      self.nml_id[0],#0
      self.cat_no[0],#1
      self.aa_nml.az.radian,#2
      self.aa_nml.alt.radian,#3
      self.eq.ra.radian,#4
      self.eq.dec.radian,#5
      self.dt_begin,#6
      self.dt_end,#7
      self.dt_end_query,#8
      self.JD,#9
      self.eq_woffs.ra.radian,#10
      self.eq_woffs.dec.radian,#11
      self.eq_mnt.ra.radian,#12
      self.eq_mnt.dec.radian,#13
      self.aa_mnt.az.radian,#14
      self.aa_mnt.alt.radian,#15
      self.image_fn,#16
      self.exp,#17
      self.pressure,
      self.temperature,
      self.humidity,#20
      self.sxtr.ra.radian,
      self.sxtr.dec.radian,
      self.astr.ra.radian,
      self.astr.dec.radian,#24
    )
    return anl_str

# used for pandas
cl_nms_anl= [
  'nml_id',#0
  'cat_no',#1
  'aa_nml_az',#2
  'aa_nml_alt',#3
  'eq_ra',#4
  'eq_dec',#5
  'dt_begin',#6
  'dt_end',#7
  'dt_end_query',#8
  'JD',#9
  'eq_woffs_ra',#10
  'eq_woffs_dec',#11
  'eq_mnt_ra',#12
  'eq_mnt_dec',#13
  'aa_mnt_az',#14
  'aa_mnt_alt',#15
  'image_fn',#16
  'exp',
  'pressure',
  'temperature',
  'humidity',#20
  'sxtr_ra',
  'sxtr_dec',
  'astr_ra',#23
  'astr_dec']

# used for pandas
cl_nms_acq=cl_nms_anl[0:cl_nms_anl.index('sxtr_ra')]

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

__author__ = 'wildi.markus@bluewin.ch'

import sys
import numpy as np
from u_point.structures import Parameter
from model.model_base import ModelHADec

class Model(ModelHADec):
  def __init__(self,lg=None,parameters=None, obs_lat=None):
    ModelHADec.__init__(self,lg=lg)

    self.fit_title='u_point'
    if parameters is None:
      self.IH=Parameter(0.)
      self.ID=Parameter(0.)
      self.CH=Parameter(0.)
      self.NP=Parameter(0.)
      self.MA=Parameter(0.)
      self.ME=Parameter(0.)
      self.TF=Parameter(0.)
      self.FO=Parameter(0.)
      self.DAF=Parameter(0.)
    else:
      self.IH=Parameter(parameters[0]/3600./180.*np.pi)
      self.ID=Parameter(parameters[1]/3600./180.*np.pi)
      self.CH=Parameter(parameters[2]/3600./180.*np.pi)
      self.NP=Parameter(parameters[3]/3600./180.*np.pi)
      self.MA=Parameter(parameters[4]/3600./180.*np.pi)
      self.ME=Parameter(parameters[5]/3600./180.*np.pi)
      self.TF=Parameter(parameters[6]/3600./180.*np.pi)
      self.FO=Parameter(parameters[7]/3600./180.*np.pi)
      self.DAF=Parameter(parameters[8]/3600./180.*np.pi)


    self.parameters=[self.IH,self.ID,self.CH,self.NP,self.MA,self.ME,self.TF,self.FO,self.DAF]
    self.phi=obs_lat
    
  # T-point:
  # IH ha index error
  # ID delta index error
  # CH collimation error CH * sec(delta)
  # NP ha/delta non-perpendicularity NP*tan(delta)
  # MA polar axis left-right misalignment ha: -MA* cos(ha)*tan(delta), dec: MA*sin(ha)
  # ME polar axis vertical misalignment: ha: ME*sin(ha)*tan(delta), dec: ME*cos(ha)
  # TF tube flexure, ha: TF*cos(phi)*sin(ha)*sec(delta), dec: TF*(cos(phi)*cos(ha)*sind(delta)-sin(phi)*cos(delta)
  # FO fork flexure, dec: FO*cos(ha)
  # DAF dec axis flexure, ha: -DAF*(cos(phi)*cos(ha) + sin(phi)*tan(dec)
  def d_lon(self,cat_lons,cat_lats,d_lons):
    val= self.IH()\
         +self.CH()/np.cos(cat_lats)\
         +self.NP()*np.tan(cat_lats)\
         -self.MA()*np.cos(cat_lons)*np.tan(cat_lats)\
         +self.ME()*np.sin(cat_lons)*np.tan(cat_lats)\
         +self.TF()*np.cos(self.phi)*np.sin(cat_lons)/np.cos(cat_lats)\
         -self.DAF()*(np.cos(self.phi)*np.cos(cat_lons)+np.sin(self.phi)*np.tan(cat_lats))
    return val-d_lons

  def d_lat(self,cat_lons,cat_lats,d_lats):
    val=self.ID()\
         +self.MA()*np.sin(cat_lons)\
         +self.ME()*np.cos(cat_lons)\
         +self.TF()*(np.cos(self.phi)*np.cos(cat_lons)*np.sin(cat_lats)-np.sin(self.phi)*np.cos(cat_lats))\
         +self.FO()*np.cos(cat_lons) # see ME()
    return val-d_lats
                        
  def fit_model(self,cats=None,mnts=None,selected=None,**kwargs):
    # Todo pythonic?  
    if kwargs is None:
      self.lg.error('expected key word: obs, exiting'.format(name, value))
      sys.exit(1)

    for name, value in kwargs.items():
      if 'obs' in name:
        self.phi=value.latitude.radian
        break
      else:
        self.lg.error('got unexpected key word: {0}={1}, exiting'.format(name, value))
        sys.exit(1)
    
    res,stat=self.fit_helper(cats=cats,mnts=mnts,selected=selected)
      
    res=stat=1
    self.lg.info('-------------------------------------------------------------')
    if stat != 1:
      if stat==5:
        self.lg.warn('fit not converged, status: {}'.format(stat))
      else:
        self.lg.warn('fit converged with status: {}'.format(stat))

    self.log_parameters()
    return res
    
  def log_parameters(self): 
    self.lg.info('values:')
    self.lg.info('IH : ha index error                 :{0:+12.4f} [arcsec]'.format(self.IH()*180.*3600./np.pi))
    self.lg.info('ID : delta index error              :{0:+12.4f} [arcsec]'.format(self.ID()*180.*3600./np.pi))
    self.lg.info('CH : collimation error              :{0:+12.4f} [arcsec]'.format(self.CH()*180.*3600./np.pi))
    self.lg.info('NP : ha/delta non-perpendicularity  :{0:+12.4f} [arcsec]'.format(self.NP()*180.*3600./np.pi))
    self.lg.info('MA : polar axis left-right alignment:{0:+12.4f} [arcsec]'.format(self.MA()*180.*3600./np.pi))
    self.lg.info('ME : polar axis vertical alignment  :{0:+12.4f} [arcsec]'.format(self.ME()*180.*3600./np.pi))
    self.lg.info('TF : tube flexure                   :{0:+12.4f} [arcsec]'.format(self.TF()*180.*3600./np.pi))
    self.lg.info('FO : fork flexure                   :{0:+12.4f} [arcsec]'.format(self.FO()*180.*3600./np.pi))
    self.lg.info('DAF: dec axis flexure               :{0:+12.4f} [arcsec]'.format(self.DAF()*180.*3600./np.pi))
      

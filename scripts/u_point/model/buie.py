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
  def __init__(self,lg=None,parameters=None,obs_lat=None):
    ModelHADec.__init__(self,lg=lg)
    
    self.fit_title='buie2003'
    if parameters is None:
      self.Dd=Parameter(0.)
      self.gamma=Parameter(0.)
      self.theta=Parameter(0.)
      self.e=Parameter(0.)
      self.Dt=Parameter(0.)
      self.c=Parameter(0.)
      self.ip=Parameter(0.)
      self.l=Parameter(0.)
      self.r=Parameter(0.)
    else:
      self.Dd=Parameter(parameters[0]/3600./180.*np.pi)
      self.gamma=Parameter(parameters[1]/3600./180.*np.pi)
      self.theta=Parameter(parameters[2]/3600./180.*np.pi)
      self.e=Parameter(parameters[3]/3600./180.*np.pi)
      self.Dt=Parameter(parameters[4]/3600./180.*np.pi)
      self.c=Parameter(parameters[5]/3600./180.*np.pi)
      self.ip=Parameter(parameters[6]/3600./180.*np.pi)
      self.l=Parameter(parameters[7]/3600./180.*np.pi)
      self.r=Parameter(parameters[8]/3600./180.*np.pi)

    self.parameters=[self.Dd,self.gamma,self.theta,self.e,self.Dt,self.c,self.ip,self.l,self.r]
    self.phi=obs_lat
    
  # Marc W. Buie 2003
  def d_lon(self,cat_lons,cat_lats,d_lons):
    # ToDo check that:
    # val= Dt()-gamma()*np.sin(cat_lons-theta())*np.tan(cat_lats)+c()/np.cos(cat_lats)-ip()*np.tan(cat_lats) +\
    # e()*np.cos(phi)*1./np.cos(cat_lats)*np.sin(cat_lons-theta())+l()*(np.sin(phi) *np.tan(cat_lats) + np.cos(cat_lats)* np.cos(cat_lons-theta())) + r()*(cat_lons-theta())
    val= self.Dt()\
       -self.gamma()*np.sin(cat_lons-self.theta())*np.tan(cat_lats)\
       +self.c()/np.cos(cat_lats)\
       -self.ip()*np.tan(cat_lats)\
       +self.e()*np.cos(self.phi)/np.cos(cat_lats)*np.sin(cat_lons)\
       +self.l()*(np.sin(self.phi)*np.tan(cat_lats) + np.cos(cat_lats)* np.cos(cat_lons))\
       + self.r()*cat_lons

    return val-d_lons

  def d_lat(self,cat_lons,cat_lats,d_lats):
    # ToDo check that:
    #val=Dd()-gamma()*np.cos(cat_lons-theta())-e()*(np.sin(phi)*np.cos(cat_lats)-np.cos(phi)*np.sin(cat_lats)*np.cos(cat_lons-theta()))
    val=self.Dd()\
      -self.gamma()*np.cos(cat_lons-self.theta())\
      -self.e()*(np.sin(self.phi)*np.cos(cat_lats)-np.cos(self.phi)*np.sin(cat_lats)*np.cos(cat_lons))

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

    self.lg.info('-------------------------------------------------------------')
    if stat != 1:
      if stat==5:
        self.lg.warn('fit not converged, status: {}'.format(stat))
      else:
        self.lg.warn('fit converged with status: {}'.format(stat))

    self.log_parameters()
    return res
    
  def log_parameters(self): 
    self.lg.info('fitted values:')
    self.lg.info('Dd:    declination zero-point offset    :{0:+12.4f} [arcsec]'.format(self.Dd()*180.*3600./np.pi))
    self.lg.info('Dt:    hour angle zero-point offset     :{0:+12.4f} [arcsec]'.format(self.Dt()*180.*3600./np.pi))
    self.lg.info('ip:    angle(polar,declination) axis    :{0:+12.4f} [arcsec]'.format(self.ip()*180.*3600./np.pi))
    self.lg.info('c :    angle(optical,mechanical) axis   :{0:+12.4f} [arcsec]'.format(self.c()*180.*3600./np.pi))
    self.lg.info('e :    tube flexure away from the zenith:{0:+12.4f} [arcsec]'.format(self.e()*180.*3600./np.pi))
    self.lg.info('gamma: angle(true,instrumental) pole    :{0:+12.4f} [arcsec]'.format(self.gamma()*180.*3600./np.pi))
    self.lg.info('theta: hour angle instrumental pole     :{0:+12.4f} [deg]'.format(self.theta()*180./np.pi))
    self.lg.info('l :    bending of declination axle      :{0:+12.4f} [arcsec]'.format(self.l()*180.*3600./np.pi))
    self.lg.info('r :    hour angle scale error           :{0:+12.4f} [arcsec]'.format(self.r()*180.*3600./np.pi))

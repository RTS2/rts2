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

import numpy as np

from structures import Parameter
from model_base import ModelBase

class Model(ModelBase):
  def __init__(self,dbg=None,lg=None):
    ModelBase.__init__(self,dbg=dbg,lg=lg)

    self.C1=Parameter(0.)
    self.C2=Parameter(0.)
    self.C3=Parameter(0.)
    self.C4=Parameter(0.)
    self.C5=Parameter(0.)
    self.C6=Parameter(0.)
    self.C7=Parameter(0.)
    self.A0=Parameter(0.)
    self.A1=Parameter(0.)
    self.A2=Parameter(0.)
    self.A3=Parameter(0.)
    self.parameters=[self.C1,self.C2,self.C3,self.C4,self.C5,self.C6,self.C7,self.A0,self.A1,self.A2,self.A3]
    self.fit_plus_poly=None
    
  # J. Condon 1992
  # Hamburg:
  # PUBLICATIONS OF THE ASTRONOMICAL SOCIETY OF THE PACIFIC, 120:425–429, 2008 April
  # The Temperature Dependence of the Pointing Model of the Hamburg Robotic Telescope
  #
  def d_lon(self,azs,alts,d_lons):
    #    C1   C2                C3                C4                            C5             
    #                                            NO!! (see C4 below): yes, minus! see paper Hamburg
    #                                            |
    val=(self.C1()+self.C2()*np.cos(alts)+self.C3()*np.sin(alts)+self.C4()*np.cos(azs)*np.sin(alts)+self.C5()*np.sin(azs)*np.sin(alts))/np.cos(alts) 
    return val-d_lons

  def d_alt_plus_poly(self,azs,alts,d_lats):
    return self.A0() + self.A1() * alts + self.A2() * alts**2 + self.A3() * alts**3

  def d_lat(self,azs,alts,d_lats):
    val_plus_poly=0.
    # this the way to expand the pointing model
    if self.fit_plus_poly:
      val_plus_poly=self.d_alt_plus_poly(azs,alts,d_lats)
    # Condon 1992, see page 6, Eq. 5: D_N cos(A) - D_W sin(A)
    #              d_alt equation is wrong on p.7
    #   C6   C7                C4               C5          
    #                         minus sign here
    #                         |
    val=self.C6()+self.C7()*np.cos(alts)-self.C4()*np.sin(azs)+self.C5()*np.cos(azs)+val_plus_poly 
    return val-d_lats
  

  def fit_model(self,cats=None,mnts=None,selected=None,**kwargs):
    # Todo pythonic?    
    if kwargs is None:
      self.lg.error('expected key word: fit_plus_poly, exiting'.format(name, value))
      sys.exit(1)
    for name, value in kwargs.items():
      if 'fit_plus_poly' in name:
        self.fit_plus_poly=value
      else:
        self.lg.error('got unexpected key word: {0} = {1},exiting'.format(name, value))
        sys.exit(1)

    res,stat=self.fit_helper(cats=cats,mnts=mnts,selected=selected)

    self.lg.info('--------------------------------------------------------')
    if stat != 1:
      if stat==5:
        self.lg.warn('fit not converged, status: {}'.format(stat))
      else:
        self.lg.warn('fit converged with status: {}'.format(stat))
    # output used in telescope driver
    if self.dbg:
      for i,c in enumerate(pars):
        if i == 7 and not self.fit_plus_poly:
          break
      self.lg.info('C{0:02d}={1};'.format(i+1,res[i]))

    self.lg.info('fitted values:')
    self.lg.info('C1: horizontal telescope collimation:{0:+10.4f} [arcsec]'.format(self.C1()*180.*3600./np.pi))
    self.lg.info('C2: constant azimuth offset         :{0:+10.4f} [arcsec]'.format(self.C2()*180.*3600./np.pi))
    self.lg.info('C3: tipping-mount collimation       :{0:+10.4f} [arcsec]'.format(self.C3()*180.*3600./np.pi))
    self.lg.info('C4: azimuth axis tilt West          :{0:+10.4f} [arcsec]'.format(self.C4()*180.*3600./np.pi))
    self.lg.info('C5: azimuth axis tilt North         :{0:+10.4f} [arcsec]'.format(self.C5()*180.*3600./np.pi))
    self.lg.info('C6: vertical telescope collimation  :{0:+10.4f} [arcsec]'.format(self.C6()*180.*3600./np.pi))
    self.lg.info('C7: gravitational tube bending      :{0:+10.4f} [arcsec]'.format(self.C7()*180.*3600./np.pi))
    if self.fit_plus_poly:
      self.lg.info('A0: a0 (polynom)                    :{0:+10.4f}'.format(self.A0()))
      self.lg.info('A1: a1 (polynom)                    :{0:+10.4f}'.format(self.A1()))
      self.lg.info('A2: a2 (polynom)                    :{0:+10.4f}'.format(self.A2()))
      self.lg.info('A3: a3 (polynom)                    :{0:+10.4f}'.format(self.A3()))
      
    return res

#!/usr/bin/env python3
#
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
#
__author__ = 'wildi.markus@bluewin.ch'


import numpy as np

class Refraction(object):
  def __init__(self, lg=None,obs=None,refraction_method='built_in'):
      
    self.lg=lg
    self.obs_astropy=obs
    if self.obs_astropy is not None:
        longitude,latitude,height=self.obs_astropy.to_geodetic()
        self.latitude=latitude.radian
        # ToDo
        # print(type(height))
        # print(str(height))
        # 3236.9999999999477 m
        # print(height.meter)
        # AttributeError: 'Quantity' object has no 'meter' member
        self.height=float(str(height).replace('m','').replace('meter',''))

    self.refraction_method=refraction_method
    
  def refractive_index(self,pressure_qfe=None,temperature=None,humidity=None,obswl=0.5):
    # refraction: J.C. Owens, 1967, Optical Refractive Index of Air: Dependence on Pressure, Temperature and Composition  
    # pressure_t = pressure_o+1013.25*pow((1.-(0.0065* hm/288.)), 5.255)  
    # Calculate the partial pressures for H2O ONLY
    # may be soon we have to inclide the carbon
    #
    # temperature: degC
    if pressure_qfe==0.:
        return np.nan
  
    tdk=temperature + 273.15
    rh=humidity
    obswl *= 1000. # Angstroem
    a=   7.6 ;
    b= 240.7 ;
    if tdk > 273.15:
        a=   7.5
        b= 235.0
    
    p_w = rh*(6.107*pow(10,(a*(tdk-273.15)/(b+(tdk-273.15)))))
    p_s= pressure_qfe-p_w ;

    # Calculate the factors D_s and D_w 
    d_s= p_s/tdk*(1.+p_s*(57.9e-8-9.3250e-4/tdk+0.25844/pow(tdk, 2.)))
    d_w= p_w/tdk*(1.+p_w*(1.+3.7e-4*p_w)*(-2.37321e-3+2.23366/tdk-710.792/pow(tdk,2.) 
                                          +7.75141e4/pow(tdk,3.)))
  
    u1=d_s*(2371.34+683939.7/(130.-pow(obswl,-2.))+4547.3/(38.99-pow(obswl,-2.)))
    u2=d_w*(6487.31+58.058 * pow( obswl, -2.)-0.7115*pow(obswl,-4.)+0.08851*pow(obswl,-6.))

    n_minus_1=u1+u2
    return n_minus_1*1.e-8  ;

  def refraction_stone(self,alt=None,tem=None,pre=None,hum=None):
    # An Accurate Method for Computing Atmospheric Refraction
    # Ronald C. Stone
    # Publications of the Astronomical Society of the Pacific
    # 108: 1051-1058, 1996 November
  
    gamma=n_minus_1=self.refractive_index(pressure_qfe=pre,temperature=tem,humidity=hum,obswl=0.5)
    if np.isnan(n_minus_1) :
        return np.nan
    # eq (9)
    beta=0.001254*(273.15 + tem)/(273.15)
    tan_z_0=np.tan(np.pi/2.-alt)
    kappa= 1.+0.005302*pow(np.sin(self.latitude),2)-0.00000583*pow(np.sin(self.latitude),2)-0.000000315*self.height
  
    # eq (4)
    d_alt= kappa*gamma*((1.-beta)*tan_z_0-(beta-gamma/2.)*pow(tan_z_0,3))
    return d_alt

  def refraction_bennett(self,alt=None,tem=None,pre=None,hum=None):
    # Bennett, Meeus, section 16
    alt_deg=alt * 180./np.pi
    R_0_amin=1./np.tan((alt_deg + 7.31/(alt_deg+4.4))/180.*np.pi)
    R_amin= R_0_amin -0.06*np.sin((14.7 * R_0_amin + 13.)/180.*np.pi)
    # R is expressed in minutes of arc
    d_alt=R_amin/60./180.* np.pi # radian
    if d_alt < 0.:
        d_alt=0.
    return d_alt

  def refraction_saemundsson(self,alt=None,tem=None,pre=None,hum=None):
    # Saemundsson, Meeus, section 16
    alt_deg=alt * 180./np.pi
    R_0_amin=1.02 / np.tan((alt_deg + 10.3/(alt_deg+5.11))/180.*np.pi)
    # R is expressed in minutes of arc
    d_alt=R_0_amin /60./180. * np.pi
    if d_alt < 0.:
        d_alt=0.
    return d_alt


if __name__ == "__main__":

  rf=Refraction()
  tem=10.
  pre=1010.
  hum=0.5
  rf.latitude=-75.1/180.*np.pi
  rf.height=3237

  for ialt in range(0,95,5):
    alt=float(ialt)/180.*np.pi
    d_alt=rf.refraction_bennett(alt=alt,tem=tem,pre=pre,hum=hum)
    print('B: d_alt: {} arcmin, alt: {}, deg'.format(d_alt*60.*180./np.pi,alt * 180./np.pi))
    d_alt=rf.refraction_saemundsson(alt=alt,tem=tem,pre=pre,hum=hum)
    print('S: d_alt: {} arcmin, alt: {}, deg'.format(d_alt*60.*180./np.pi,alt * 180./np.pi))
    d_alt=rf.refraction_stone(alt=alt,tem=tem,pre=pre,hum=hum)
    print('O:d_alt: {} arcmin, alt: {}, deg'.format(d_alt*60.*180./np.pi,alt * 180./np.pi))

#!/usr/bin/env python3
#
# (C) 2017, Markus Wildi, wildi.markus@bluewin.ch
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

Call slitazimuth.c as shared library and compare Toshimi Taki with P.T. Wallace

'''
__author__ = 'wildi.markus@bluewin.ch'

import sys
import argparse
import math
import numpy as np
#
#
# Python bindings for shared libs
#
from ctypes import *
class TK_Geometry_obs(Structure):
  _fields_ = [("xd", c_double),("zd", c_double),("rdec", c_double),("rdome", c_double)]

class Geometry_obs(Structure):
  _fields_ = [("x_m", c_double),("y_m", c_double),("z_m", c_double),("p", c_double),("q", c_double),("r", c_double),("r_D", c_double)]

class LN_lnlat(Structure):
  _fields_ = [("lng", c_double),("lat", c_double)]

# add full path if it is not on LD_PATH
try:
  sl=cdll.LoadLibrary('./libslitazimuth.so')
except OSError as e:
  print('libslitazimuth.so not found, creating it')
  import subprocess
  cmd=[
    'cd', 
    '$HOME/rts2/scripts', 
    '&&',
    'gcc', 
    '-c', 
    '-Werror', 
    '-fpic', 
    '../lib/rts2/slitazimuth.c', 
    '-I../include/', 
    '&&', 
    'gcc', 
    '-shared', 
    '-o', 
    'libslitazimuth.so ', 
    'slitazimuth.o', 
    '-lnova', 
  ]
  # python3
  cmd_str= ' '.join(c for c in cmd)
  print(cmd_str)
  stdo,stde=subprocess.Popen(cmd_str,shell=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE).communicate()
  if len(stdo):
    print('gcc stdo: {}'.format(stdo.decode('utf-8')))
  if len(stde):
    print('gcc stde: {}'.format(stde.decode('utf-8')))
    sys.exit(1)
  sl=cdll.LoadLibrary('./libslitazimuth.so')
      
sl.dome_target_az.restype=c_double
sl.TK_dome_target_az.restype=c_double
sl.ln_get_julian_from_sys.restype=c_double
sl.ln_get_mean_sidereal_time.restype=c_double
sl.ln_get_mean_sidereal_time.restype=c_double

class LN_equ(Structure):
  _fields_ = [("ra", c_double),("dec", c_double)]

class LN_alt(Structure):
  _fields_ = [("az", c_double),("alt", c_double)]


class ScriptWallace_LB(object):
  def __init__(self, obs_lng=None, obs_lat=None,x_m=None,y_m=None,z_m=None,p=None,q=None,r=None,r_D=None):
    self.p_obs=LN_lnlat()
    self.p_obs.lng=obs_lng     # deg
    self.p_obs.lat=obs_lat     # deg
    #  
    self.g_obs=Geometry_obs()
    self.g_obs.x_m=x_m
    self.g_obs.y_m=y_m
    self.g_obs.z_m=z_m
    self.g_obs.p=p
    self.g_obs.q=q
    self.g_obs.r=r
    self.g_obs.r_D=r_D

  def calculate_slit_position(self, ra=None,dec=None):
    tel_eq=LN_equ()
    tel_eq.ra=ra
    tel_eq.dec=dec
    return sl.dome_target_az(tel_eq, self.p_obs, self.g_obs)


class ScriptWallace(object):
  '''
  DOME PREDICTIONS FOR AN EQUATORIAL TELESCOPE
  Patrick Wallace
  Tpoint Software UK
  ptw@tpointsw.uk
  2016 November 13
  http://www.tpointsw.uk/edome.pdf
  '''
  def __init__(self, obs_lng=None, obs_lat=None,x_m=None,y_m=None,z_m=None,p=None,q=None,r=None,r_D=None):
    self.obs_lng=obs_lng/180.*np.pi # rad 
    self.obs_lat=obs_lat/180.*np.pi
    #
    #
    self.x_m=x_m
    self.y_m=y_m
    self.z_m=z_m
    self.p=p
    self.q=q
    self.r=r
    self.r_D=r_D

  def calculate_slit_position(self, h=None,dec=None):
    
    y_0=self.p + self.r*np.sin(dec)
    x_mo= self.q*np.cos(h) + y_0*np.sin(h)
    y_mo=-self.q*np.sin(h) + y_0*np.cos(h)
    z_mo= self.r*np.cos(dec)
    
    x_do= self.x_m+x_mo
    y_do= self.y_m+y_mo*np.sin(self.obs_lat)+z_mo*np.cos(self.obs_lat)
    z_do= self.z_m-y_mo*np.cos(self.obs_lat)+z_mo*np.sin(self.obs_lat)
    # HA,dec
    x=-np.sin(h)*np.cos(dec)
    y=-np.cos(h)*np.cos(dec)
    z= np.sin(dec)
    # transform from HA,dec to AltAz
    x_s= x
    y_s= y*np.sin(self.obs_lat)+z*np.cos(self.obs_lat)
    z_s=-y*np.cos(self.obs_lat)+z*np.sin(self.obs_lat)

    s_dt= x_s*x_do+y_s*y_do+z_s*z_do
    t2_m= np.power(x_do,2)+np.power(y_do,2)+np.power(z_do,2) 
    w= np.power(s_dt,2)-t2_m+np.power(self.r_D,2)
    if w < 0.:
      print('no solution for: lat: {}, h: {}, dec:{}'.format(self.obs_lat,h,dec))
      return None,None
    
    f=-s_dt + np.sqrt(w)
    
    x_da= x_do+f*x_s
    y_da= y_do+f*y_s
    z_da= z_do+f*z_s
    # The dome (A, E) that the algorithm delivers follows the normal convention. Azimuth A
    # increases clockwise from zero in the north, through 90◦ (π/2 radians) in the east.
    if x_da==0 and y_da==0:
      print('x_da==0 and y_da==0')
      return None,None
    
    A= np.arctan2(x_da,y_da)
    E= np.arctan2(z_da,np.sqrt(np.power(x_da,2)+np.power(y_da,2)))
    #E= np.arcsin(z_da/np.sqrt(np.power(x_da,2)+np.power(y_da,2)+np.power(z_da,2)))
    return A*180./np.pi,E*180./np.pi
                  

    
class ScriptTaki(object):
  def __init__(self, obs_lng=None, obs_lat=None,xd=None,zd=None,rdec=None,rdome=None):
    self.p_obs=LN_lnlat()
    self.p_obs.lng=obs_lng     # deg
    self.p_obs.lat=obs_lat     # deg
    #  see above
    self.g_obs=TK_Geometry_obs()
    self.g_obs.xd=xd
    self.g_obs.zd=zd
    self.g_obs.rdec=rdec
    self.g_obs.rdome=rdome

  def dome_az(self, ra=None, dec=None):
    tel_eq=LN_equ()
    tel_eq.ra=ra
    tel_eq.dec=dec
    # [d_az]=deg
    # N=0., E=90. deg
    # cupola Obs. Vermes N=0, E=90 deg
    # 
    return sl.TK_dome_target_az(tel_eq, self.p_obs, self.g_obs)

  def eq_to_aa(self,ra=None,dec=None,jd=None):
    tel_eq=LN_equ()
    tel_eq.ra=ra
    tel_eq.dec=dec
    tel_aa=LN_alt()
    # S=0., W=90. deg
    sl.ln_get_hrz_from_equ(byref(tel_eq), byref(self.p_obs), c_double(jd), byref(tel_aa));  
    return tel_aa.az,tel_aa.alt

  def aa_to_eq(self,az=None,alt=None,jd=None):
    tel_aa=LN_alt()
    # Libnova   S: az=0., W: az=90.
    tel_aa.az=az
    tel_aa.alt=alt
    tel_eq=LN_equ()    
    sl.ln_get_equ_from_hrz(byref(tel_aa),byref(self.p_obs),c_double(jd),byref(tel_eq))
    return tel_eq.ra,tel_eq.dec


# helper for negative values
def arg_floats(value):
  return list(map(float, value.split()))

def arg_float(value):
  if 'm' in value:
    return -float(value[1:])
  else:
    return float(value)

if __name__ == "__main__":

    parser= argparse.ArgumentParser(prog=sys.argv[0], description='Test  RTS2\'s slitazimuth.c')
    parser.add_argument('--obs-longitude', dest='obs_lng', action='store', default=123.2994166666666,type=arg_float, help=': %(default)s [deg], observatory longitude + to the East [deg], negative value: m10. equals to -10.')
    parser.add_argument('--obs-latitude', dest='obs_lat', action='store', default=75.1,type=arg_float, help=': %(default)s [deg], observatory latitude [deg], negative value: m10. equals to -10.')
    parser.add_argument('--xd', dest='xd', action='store', default=-0.0684,type=arg_float, help=': %(default)s [m], x offset,  negative value: m10. equals to -10.')
    parser.add_argument('--zd', dest='zd', action='store', default=-0.1934,type=arg_float, help=': %(default)s [m], z offset,  negative value: m10. equals to -10.')
    parser.add_argument('--rdec', dest='rdec', action='store', default=0.338,type=arg_float, help=': %(default)s [m], rdec')
    parser.add_argument('--rdome', dest='rdome', action='store', default=1.265,type=arg_float, help=': %(default)s [m], rdome')
    parser.add_argument('--test-forth-back', dest='test_forth_back', action='store_true', default=False, help=': %(default)s, True: set initial values to 0., 1. see script')
    parser.add_argument('--test-wallace', dest='test_wallace', action='store_true', default=False, help=': %(default)s, True: check P.T. Wallace\'s example')
    parser.add_argument('--rdec-negative', dest='rdec_negative', action='store_true', default=False, help=': %(default)s, True: set Taki rdec=-args.rdec')
    args=parser.parse_args()

    print('DOMESLIT   AZ coordinate systen: N=0, E=90 deg')
    #print('MOUNT      AZ coordinate systen: S=0, W=90 deg')
    print('TAKI (ori) AZ coordinate systen: S=0, W=90 deg')
    print('WALLACE    AZ coordinate systen: N=0, E=90 deg')
    print('Libnova    AZ coordinate systen: S=0, W=90 deg')
    # Taki
    # x_d : positive to south
    #(y_d): positive to east
    # z_d : positive to zenith
    #args.xd=args.zd=args.rdec=0.
    #args.rdome=1
    rdec_tk=args.rdec
    if args.rdec_negative:
      rdec_tk= -args.rdec
      print('TAKI: negative rdec: {}'.format(rdec_tk))
      
    print('')
    print('TAKI    initial values: xd: {}, zd: {}, rdec: {}, rdome: {}'.format(args.xd,args.zd,rdec_tk,args.rdome))
    
    sctk=ScriptTaki(obs_lng=args.obs_lng,obs_lat=args.obs_lat,xd=args.xd,zd=args.zd,rdec=rdec_tk,rdome=args.rdome)
    # Wallace
    # x_m: positive to east
    # y_m: positive to north
    # z_m: positive to zenith
    x_m=0.
    y_m=-args.xd
    z_m=args.zd
    r=p=0
    q=args.rdec
    r_D=args.rdome
    print('WALLACE initial values: x_m: {}, y_m: {}, z_m: {}, r: {}, p: {}, q: {}, rdome: {}'.format(x_m,y_m,z_m,r,p,q,r_D))
    print()
    # check the values:
    # Az=0. (South), HA=0, ra:local sidereal time
    # Alt=0., dec: np.pi/2.-args.obs_lat
    jd_now=sl.ln_get_julian_from_sys()
    # ln_get_mean_sidereal_time: unit hours
    sidt= 15./180.*np.pi  * sl.ln_get_mean_sidereal_time(c_double(jd_now))
    # N: Az=0, E: Az=90
    # meridian:
    az=180.
    alt=0.
    # dome slit N: az=0., E: az=90.
    # Libnova   S: az=0., W: az=90.
    ra,dec=sctk.aa_to_eq(az=az-180.,alt=alt,jd=jd_now)
    sx=ra * 3600. /15.
    hx = int(sx / 3600)
    sx -= hx * 3600
    mx = int(sx / 60)
    sx -= mx * 60
    print('check values: dome slit AltAz, az=alt=0., obs lon: {}, lat:{}'.format(args.obs_lng, args.obs_lat))
    print('ra : {0} h {1} m {2:2.1f} s'.format(hx,mx,sx))
    print('ra : {0:4.3f} rad, local sidt: {1:4.3f} rad, diff: {2:4.3f} rad'.format(ra/180.*np.pi, sidt+args.obs_lng/180.*np.pi, ra/180.*np.pi-(sidt+args.obs_lng/180.*np.pi)))
    print('dec: {0:+2.1f}, -pi/2.+latitude: {1:2.1f}'.format(dec,-90.+args.obs_lat))
    HA= (sidt + args.obs_lng/180.*np.pi - ra/180.*np.pi) % 2. * np.pi 
    print('HA : {0:+2.1f} (south point)'.format(HA/np.pi*180.))
    print('')
    #
    scwl_lb=ScriptWallace_LB(obs_lng=args.obs_lng, obs_lat=args.obs_lat,x_m=x_m,y_m=y_m,z_m=z_m,p=p,q=q,r=r,r_D=r_D)
    scwl=ScriptWallace(obs_lng=args.obs_lng, obs_lat=args.obs_lat,x_m=x_m,y_m=y_m,z_m=z_m,p=p,q=q,r=r,r_D=r_D)
    az_r=np.linspace(0., 360., num=5)
    #alt_r=np.linspace(30., 90., num=4)
    #alt_r=np.linspace(0., 90., num=10)
    alt_r=np.linspace(0., 1., num=1)
    # this loop: Az=0: S, Az=90: W
    az_diff=3.
    print('optical axis is at HA-pi/2 ("east of the pier")')
    for az in az_r:
      # Test ./dome_target_az.py --obs-lat 36.18 --xd m370. --zd 1250. --rdec 505. --rdome 1900  --obs-lon 0. --test-w
      # Test az += 1.48
      for alt in alt_r:
        # ra,dec: deg
        # calculate ra,dec, no confusion about coordinate systems
        # obs_lng: positive to East
        ra,dec=sctk.aa_to_eq(az=az-180.,alt=alt,jd=jd_now)
        # Test dec=0.6615 * 180./np.pi
        
        HA= (sidt + args.obs_lng/180.*np.pi - ra/180.*np.pi)
        if HA < 0:
          HA += 2.*np.pi

        dec_tk=dec
        d_az_tk= sctk.dome_az(ra=ra, dec=dec_tk)

        HA_wl= HA
        dec_wl=dec/180.*np.pi
        
        d_az_wl, d_el_wl=scwl.calculate_slit_position(h=HA_wl, dec=dec_wl)
        d_az_wl_lb=scwl_lb.calculate_slit_position(ra=ra, dec=dec) # deg
        
        if d_az_tk < 0.:
          d_az_tk += 360.
          
        if d_az_wl < 0.:
          d_az_wl += 360.
          
        if d_az_wl_lb < 0.:
          d_az_wl_lb += 360.

        print('telescope az: {0:7.3f}, alt: {1:7.3f}, HA: {2:+8.3f}, dec: {3:+8.3f}'.format(az,alt,HA*180./np.pi,dec)) 
        if abs(d_az_tk-d_az_wl) > az_diff:
          print('diff tk-wl: {0:+8.3f} deg, larger than: {1:+8.3f} deg'.format(d_az_tk-d_az_wl, az_diff))
          
        print('TAKI      : dome az: {3:+8.3f} deg'.format(alt,az,HA*180./np.pi,d_az_tk)) 
        print('LB WALLACE: dome az: {0:+8.3f} deg'.format(d_az_wl_lb)) 
          
        print('WALLACE   : dome az: {0:+8.3f} deg, elevation: {1:7.3f}'.format(d_az_wl,d_el_wl)) 
        print('')
        
    if args.test_wallace:
      jd_now=sl.ln_get_julian_from_sys()
      # ln_get_mean_sidereal_time: unit hours
      sidt= 15./180.*np.pi  * sl.ln_get_mean_sidereal_time(c_double(jd_now))
      obs_lng=0.*180./np.pi
      obs_lat=+0.6315*180./np.pi

      x_m=-35.
      y_m=370.    
      z_m=1250.
      r=p=0.
      q=505.
      r_D=1900.
      
      scwl=ScriptWallace(obs_lng=obs_lng, obs_lat=obs_lat,x_m=x_m,y_m=y_m,z_m=z_m,p=p,q=q,r=r,r_D=r_D)
      scwl_lb=ScriptWallace_LB(obs_lng=obs_lng, obs_lat=obs_lat,x_m=x_m,y_m=y_m,z_m=z_m,p=p,q=q,r=r,r_D=r_D)

      HA_east=h=+0.0436
      dec=+0.6615
      
      d_az_wl, d_el_wl=scwl.calculate_slit_position(h=h, dec=dec)
      ra= sidt + obs_lng/180.*np.pi - HA_east
      d_az_wl_lb=scwl_lb.calculate_slit_position(ra=ra*180./np.pi, dec=dec*180./np.pi)
      
      print('')
      print('')
      print('example values from the paper:')
      print('longitude: {0:5.2f}, latitude: {1:5.2f}, h:{2:5.2f}, dec:{3:5.2f}'.format(obs_lng, obs_lat, h*180./np.pi,dec*180./np.pi))
      print('WALLACE   : x_m: {}, y_m: {}, z_m: {}, r: {}, p: {}, q: {}, rdome: {}'.format(x_m,y_m,z_m,r,p,q,r_D))
      print('WALLACE   : lat: {0:7.3f}, h: {1:7.3f} arcmin, dec: {2:7.3f}, dome az: {3:7.3f}/50.3694111 deg, el: {4:7.3f}/72.051742 deg'.format(obs_lat,h/np.pi*180.*60., dec/np.pi*180.,d_az_wl,d_el_wl))
      print('WALLACE_LB:                                                dome az: {0:7.3f}/50.3694111 deg'.format(d_az_wl_lb))

      print('')
      # . . . and the predicted (A, E) is:
      # (50.369411◦, 72.051742◦).
      #
      scwl=ScriptWallace(obs_lng=obs_lng, obs_lat=obs_lat,x_m=x_m,y_m=y_m,z_m=z_m,p=p,q=q,r=r,r_D=r_D)
      scwl_lb=ScriptWallace_LB(obs_lng=obs_lng, obs_lat=obs_lat,x_m=x_m,y_m=y_m,z_m=z_m,p=p,q=q,r=r,r_D=r_D)
      # paper says -3.098
      h=-3.098   # -(pi-0.0436)

      dec_lb=dec=2.480  # 180. -37.9
      d_az_wl, d_el_wl=scwl.calculate_slit_position(h=h, dec=dec)

      ra_lb= sidt + obs_lng/180.*np.pi - HA_east # done in the lib
      d_az_wl_lb=scwl_lb.calculate_slit_position(ra=ra_lb*180./np.pi, dec=dec_lb*180./np.pi)
      if math.isnan(d_az_wl_lb):
        print('nan')
      if d_az_wl < 0.:
        d_az_wl += 360.
      if d_az_wl_lb < 0.:
        d_az_wl_lb += 360.
      
      print('WALLACE   : x_m: {} m, y_m: {} m, z_m: {} m, r: {}, p: {}, q: {}, rdome: {} m'.format(x_m,y_m,z_m,r,p,q,r_D))
      print('WALLACE   : dome az: {0:7.3f}/305.595067 deg, el: {1:7.3f}/68.824495 deg'.format(d_az_wl,d_el_wl)) 
      print('WALLACE_LB: dome az: {0:7.3f}/305.595067 deg'.format(d_az_wl_lb)) 


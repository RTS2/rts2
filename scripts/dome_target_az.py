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
Call slitazimuth.c as shared library                                                                                                                                                                                                 cd ~/rts2/lib/rts2
 gcc -c  -Werror -fpic  slitazimuth.c -I../../include/ && gcc -shared -o libslitazimuth.so slitazimuth.o -lnova
                                     

'''

__author__ = 'wildi.markus@bluewin.ch'

import sys
import argparse
import numpy as np
#
# cd ~/rts2/lib/rts2
# gcc -c  -Werror -fpic  slitazimuth.c -I../../include/ && gcc -shared -o libslitazimuth.so slitazimuth.o -lnova
#
# Python bindings for shared libs
from ctypes import *
class Geometry_obs(Structure):
  _fields_ = [("xd", c_double),("zd", c_double),("rdec", c_double),("rdome", c_double)]

class LN_lnlat(Structure):
  _fields_ = [("lng", c_double),("lat", c_double)]

# add full path if it is not on LD_PATH
try:
    sl=cdll.LoadLibrary('libslitazimuth.so')
except Exception as e:
    print('libslitazimuth.so not found')
    print('set full path, e.g.: /your/home/rts2/lib/rts2/libslitazimuth.so')
    sys.exit(1)


sl.dome_target_az.restype=c_double
sl.ln_get_julian_from_sys.restype=c_double
sl.ln_get_mean_sidereal_time.restype=c_double

class LN_equ(Structure):
  _fields_ = [("ra", c_double),("dec", c_double)]

class LN_alt(Structure):
  _fields_ = [("az", c_double),("alt", c_double)]


class Script(object):
  def __init__(self, obs_lng=None, obs_lat=None,xd=None,zd=None,rdec=None,rdome=None):
    self.p_obs=LN_lnlat()
    self.p_obs.lng=obs_lng     # deg
    self.p_obs.lat=obs_lat     # deg
    #  see above
    self.g_obs=Geometry_obs()
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
    return sl.dome_target_az(tel_eq, self.p_obs, self.g_obs)-180.

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
    parser.add_argument('--test', dest='test', action='store_true', default=False, help=': %(default)s, True: set initial values to 0., 1. see script')
    args=parser.parse_args()

    jd_now=sl.ln_get_julian_from_sys()
    print('AZ coordinate systen: S=0, W=90 deg')
    if args.test:
      print('test transformation forth and back: telescope(az)-dome(az)=0 for xd=zd=rdec=0., rdome=1.')
      xd=zd=rdec=0.
      rdome=1.
      sc=Script(obs_lng=args.obs_lng,obs_lat=args.obs_lat,xd=xd,zd=zd,rdec=rdec,rdome=rdome)
      print('az: {}, alt: {}'.format(0.,0.))
      ra,dec=sc.aa_to_eq(az=0.,alt=0.,jd=jd_now)
      d_az=sc.dome_az(ra=ra, dec=dec)
      t_az,t_alt=sc.eq_to_aa(ra=ra, dec=dec,jd=jd_now)
      print('telescope az: {}, alt: {}'.format(t_az %360., t_alt))
      print('telescope az: {}, dome az: {}'.format( t_az % 3360., d_az % 360.))
      sys.exit(1)

    sc=Script(obs_lng=args.obs_lng,obs_lat=args.obs_lat,xd=args.xd,zd=args.zd,rdec=args.rdec,rdome=args.rdome)
    print('xd: {} m, zd: {} m, rdec: {} m, rdome: {} m'.format(args.xd,args.zd,args.rdec,args.rdome))
    az_r=np.linspace(0., 270., num=4)
    alt_r=np.linspace(0., 80., num=9)
    for az in az_r:
      for alt in alt_r:
        ra,dec=sc.aa_to_eq(az=az,alt=alt,jd=jd_now)
        #print(az,alt)
        #print(ra,dec)
        d_az=sc.dome_az(ra=ra, dec=dec)
        print('telescope alt: {0:7.3f}, az: {1:7.3f}, dome az: {2:7.3f} deg'.format(alt, az % 360.,d_az % 360.)) 

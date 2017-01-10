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
'''

Multiprocesser Worker

'''
from multiprocessing import current_process
import numpy as np
import sys
sys.path.append('../')

import u_point.astrometry_3 as astrometry_3

class SolverResult():
  """Results of astrometry.net including necessary fits headers"""
  def __init__(self, ra=None, dec=None, fn=None):
    self.ra= ra
    self.dec=dec
    self.fn= fn

class Solver():
  """Solve a field with astrometry.net """
  def __init__(self, lg=None, blind=False, scale=None, radius=None, replace=None, verbose=None,timeout=None):
    self.lg = lg
    self.blind= blind
    self.scale  = scale/np.pi*3600.*180. # solve-field -u app arcsec/pixel
    self.radius = radius
    self.replace= replace
    self.verbose= verbose
    self.timeout=timeout
    
  def solve_field(self,fn=None, ra=None, dec=None):
    try:
      self.solver = astrometry_3.AstrometryScript(lg=self.lg,fits_file=fn)
    except Exception as e:
      self.lg.debug('Solver: solver died, file: {}, exception: {}'.format(fn, e))
      return None
      
    # base class method
    if self.blind:
      center=self.solver.run(scale=self.scale, replace=self.replace,timeout=self.timeout,verbose=self.verbose,wrkr=current_process().name)
    else:
      # ToDo
      center=self.solver.run(scale=self.scale,ra=ra,dec=dec,radius=self.radius,replace=self.replace,timeout=self.timeout,verbose=self.verbose,wrkr=current_process().name)

      if center!=None:
        if len(center)==2:
          return SolverResult(ra=center[0],dec=center[1],fn=fn)
      return None



# really ugly!
def arg_floats(value):
  return list(map(float, value.split()))

def arg_float(value):
  if 'm' in value:
    return -float(value[1:])
  else:
    return float(value)

if __name__ == "__main__":
  sys.path.append('../')
  import dateutil.parser
  sys.path.append('../')
  from u_point.structures import SkyPosition
  import argparse,logging
  parser= argparse.ArgumentParser(prog=sys.argv[0], description='Analyze observed positions')
  parser.add_argument('--level', dest='level', default='WARN', help=': %(default)s, debug level')
  parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
  parser.add_argument('--obs-longitude', dest='obs_lng', action='store', default=123.2994166666666,type=arg_float, help=': %(default)s [deg], observatory longitude + to the East [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-latitude', dest='obs_lat', action='store', default=-75.1,type=arg_float, help=': %(default)s [deg], observatory latitude [deg], negative value: m10. equals to -10.')
  parser.add_argument('--obs-height', dest='obs_height', action='store', default=3237.,type=arg_float, help=': %(default)s [m], observatory height above sea level [m], negative value: m10. equals to -10.')
  parser.add_argument('--base-path', dest='base_path', action='store', default='/tmp/u_point/',type=str, help=': %(default)s , directory where images are stored')
  parser.add_argument('--pixel-scale', dest='pixel_scale', action='store', default=1.7,type=float, help=': %(default)s [arcsec/pixel], arcmin/pixel of the CCD camera')
  parser.add_argument('--timeout', dest='timeout', action='store', default=120,type=int, help=': %(default)s [sec], astrometry timeout for finding a solution')
  parser.add_argument('--radius', dest='radius', action='store', default=1.,type=float, help=': %(default)s [deg], astrometry search radius')
  parser.add_argument('--verbose-astrometry', dest='verbose_astrometry', action='store_true', default=False, help=': %(default)s, use astrometry in verbose mode')
  parser.add_argument('--fits-image', dest='fits_image', action='store', default=None,type=str, help=': %(default)s fits image to analyze')
  parser.add_argument('--blind', dest='blind', action='store_true', default=False, help=': %(default)s, use astrometry in blind mode')
  parser.add_argument('--replace', dest='replace', action='store_true', default=False, help=': %(default)s, write result to image file')
  parser.add_argument('--ra', dest='ra', action='store', default=None,type=arg_float, help=': %(default)s [rad], center ra')
  parser.add_argument('--dec', dest='dec', action='store', default=None,type=arg_float, help=': %(default)s [rad], center dec')
  args=parser.parse_args()
  
  if args.toconsole:
    args.level='DEBUG'

  filename='/tmp/{}.log'.format(sys.argv[0].replace('.py','')) # ToDo datetime, name of the script
  logformat= '%(asctime)s:%(name)s:%(levelname)s:%(message)s'
  logging.basicConfig(filename=filename, level=args.level.upper(), format= logformat)
  logger=logging.getLogger()
    
  if args.toconsole:
    # http://www.mglerner.com/blog/?p=8
    soh=logging.StreamHandler(sys.stdout)
    soh.setLevel(args.level)
    logger.addHandler(soh)

  px_scale=args.pixel_scale/3600./180.*np.pi

  solver= Solver(lg=logger,blind=False,scale=px_scale,radius=args.radius,replace=args.replace,verbose=args.verbose_astrometry,timeout=args.timeout)
  sr= solver.solve_field(fn=args.fits_image,ra=args.ra,dec=args.dec)

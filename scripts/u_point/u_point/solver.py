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


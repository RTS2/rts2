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
import scipy.optimize
from astropy.coordinates.representation import SphericalRepresentation


class ModelBase(object):
  def __init__(self,dbg=None,lg=None):
    self.dbg=dbg
    self.lg=lg

  def fit(self,cat_lons=None,cat_lats=None,d_lons=None, d_lats=None):
    f_lon=self.d_lon(cat_lons,cat_lats,d_lons)
    f_lat=self.d_lat(cat_lons,cat_lats,d_lats) 
    # this is the casus cnactus
    # http://stackoverflow.com/questions/23532068/fitting-multiple-data-sets-using-scipy-optimize-with-the-same-parameters
    return np.concatenate((f_lon,f_lat))

  def fit_helper(self,cats=None,mnts=None,selected=None):
    # ToDo I do not like that
    def local_f(params):
      for i,p in enumerate(self.parameters):
        p.set(params[i])

      return self.fit(cat_lons=cat_lons,cat_lats=cat_lats,d_lons=d_lons, d_lats=d_lats)

    no=len(selected)
    cat_lons=np.zeros(shape=(no))
    cat_lats=np.zeros(shape=(no))
    d_lons=np.zeros(shape=(no))
    d_lats=np.zeros(shape=(no))
    # make it suitable for fitting
    for i,v in enumerate(selected):
      # x: catlog
      cts=cats[v].represent_as(SphericalRepresentation)
      mts=mnts[v].represent_as(SphericalRepresentation)
      #print(mts.lon.radian,mts.lat.radian)
      cat_lons[i]=cts.lon.radian # ToDo ev. use Representation
      cat_lats[i]=cts.lat.radian
      #
      # y:  catalog_apparent-star
      d_lons[i]=cts.lon.radian-mts.lon.radian
      d_lats[i]=cts.lat.radian-mts.lat.radian
      #print(d_lons[i],d_lats[i])
      #import sys
      #sys.exit(1)

    p = [param() for param in self.parameters]
    return scipy.optimize.leastsq(local_f, p)#,full_output=True)

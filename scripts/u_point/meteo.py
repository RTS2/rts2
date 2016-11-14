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

acquire externel meteo data, this is a stub

'''
__author__ = 'wildi.markus@bluewin.ch'

# This class must implemented as well as the retrieve method
class Meteo(object):
  def __init__(
    self, 
    lg=None,
  ):
    self.lg=lg
    self.pressure=636.
    self.temperature=-55.6
    self.humidity=0.5
    
  def retrieve(self,atmosphere=False): # False: used for simulation
    # fetch the data from external source, e.g. RTS2 weather station
    if atmosphere:
      return self.pressure,self.temperature,self.humidity
    else:
      return 0.,0.,0.

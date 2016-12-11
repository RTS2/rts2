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

acquire Dome Concordia meteo data (on site only),

'''
__author__ = 'wildi.markus@bluewin.ch'

import sys
import requests
import argparse
import logging
from lxml import html

dome_c_base_url='http://intranet.concordiastation.aq/index.php/meteo'

class Meteo(object):
  def __init__(
    self, 
    lg=None,
  ):
    self.lg=lg
    
  def retrieve(self,atmosphere=True):
    if atmosphere:
      # <tr>
      # <td class=data_name>Temperature</td>
      # <td class=data_value>-36.6 &deg;C</td> ----
      # <td class=data_name>Pressure</td>
      # <td class=data_value>658.6 hPa</td> ----
      #</tr>
      #...
      # <tr>
      # <td class=data_name>Windchill</td>
      # <td class=data_value>-50.3 &deg;C</td>
      # <td class=data_name>Relative Humidity</td>
      # <td class=data_value>65 %</td> ----
      # </tr>
      r=requests.get(dome_c_base_url, stream=True)
      tree = html.fromstring(r.content)
      data_nm = tree.xpath('//td[@class="data_name"]/text()')
      data_rw = tree.xpath('//td[@class="data_value"]/text()')
      self.lg.debug('N: {}'.format(data_nm))
      self.lg.debug('R: {}'.format(data_rw))
      data=[float(x.split()[0].replace('°','')) for x in data_rw]
      self.lg.debug('V: {}'.format(data))
      # N: ['Temperature', 'Pressure', 'Windchill', 'Relative Humidity', 'Wind Direction', 'Dewpoint', 'Wind Speed']
      # R: ['-39.9 °C', '658.6 hPa', '-53.1 °C', '61 %', '173°', '-44.5 °C', '3.9 m/s', '7.5 knots']
      # V: [-39.9, 658.6, -53.1, 61.0, 173.0, -44.5, 3.9, 7.5return data
      #
      # sanity check:
      sane=True
      # temperature
      if data[0] < -90. or -10. < data[0]: # Vostok, Antarctica
        sane=False
      # pressure
      if data[1] < 590. or 680. < data[1]:
        sane=False
      # humidity is always ok
      return data[1],data[0],data[3]
    else:
      return 0.,0.,0.


if __name__ == "__main__":

  parser= argparse.ArgumentParser(prog=sys.argv[0], description='Acquire not yet observed positions')
  parser.add_argument('--level', dest='level', default='WARN', help=': %(default)s, debug level')
  parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
  args=parser.parse_args()


  filename='/tmp/{}.log'.format(sys.argv[0].replace('.py','')) # ToDo datetime, name of the script
  logformat= '%(asctime)s:%(name)s:%(levelname)s:%(message)s'
  logging.basicConfig(filename=filename, level=args.level.upper(), format= logformat)
  logger=logging.getLogger()
    
  if args.toconsole:
    # http://www.mglerner.com/blog/?p=8
    soh=logging.StreamHandler(sys.stdout)
    soh.setLevel(args.level)
    logger.addHandler(soh)

  mt=Meteo(lg=logger)
  temperature,pressure,humidity=mt.retrieve()
  logger.debug('received temperature: {0:+7.2f}'.format(temperature))
  logger.debug('received pressure   : {0:+7.2f}'.format(pressure))
  logger.debug('received humidity   : {0:+7.2f}'.format(humidity))

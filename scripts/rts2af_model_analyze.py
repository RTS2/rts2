#!/usr/bin/python
#   rts2af_model_analyze.py is the nucleus of modelling the
#   temperature dependency of the focus of a telescope.
#
#   As of 2011-05-29 it is work in progress.
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

__author__ = 'markus.wildi@one-arcsec.org'

import sys
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable


x = np.arange(96, dtype=float)
y = np.arange(96, dtype=float)

with open( 'result.log', 'r') as frfn:
    i= 0
    for line in frfn:
        line.strip()
#3.475742 18.997C 3852.31810103 3.453986 1306374986.0
        items= line.split()

        try:
            chi2= float(items[3])
        except:
            continue
        if((chi2 < 100.) and ( chi2 >0.)):
            x[i]=float(items[2])
            yfloat= float(items[1].replace('C', ''))
            y[i]= yfloat
#            print '----{0}, {1}, {2} {3}'.format(i, x[i], y[i], items[1])
            i += 1
        else:
           # print 'line not used {0}-----------{1}'.format(line, items[3])
            pass


fig = plt.figure(1, figsize=(5.5,5.5))
axScatter = plt.subplot(111)
axScatter.scatter(x, y)
axScatter.set_title("Temperature model for AP Astrofire F/7")
plt.xlabel('FOC_POS [ticks]')
plt.ylabel('Temperature [C]')
plt.grid(True,linestyle='-',color='0.75')
plt.axis([ 2600,4600,0,20])
plt.draw()
plt.show()

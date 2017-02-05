# Library for random path on alt-az coordinates.
# (C) 2016 Petr Kubanek
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

import numpy as np
import random

def random_path(altitudes=[25.3,36.2,45.1,54.7,63.2,73.1,82.2],azimuth_separation=36.3):
	"""Generate random path."""
	altaz=[]
	for al in altitudes:
		for a in np.arange(0 + al/10.0, 360.0, azimuth_separation * (np.sin(np.radians(al)))):
			altaz.append([round(al+random.random() * 2,3), round(a + random.random() * 3, 3)])

	#pole..
	altaz.append([89.2,123.34])

	path=[]

	while len(altaz) > 0:
		i=int(round(random.random() * len(altaz)))
		if i==len(altaz):
			i=len(altaz) - 1
		path.append(altaz[i])
		del altaz[i]
	
	return path

def constant_path(altitudes=[20,40,75]):
	"""Generate constant path at given altitudes."""
	path=[]
	az_r=range(0,360,30)
	for alt in altitudes:
		for az in az_r:
			path.append([alt,az])
	path.append([90,0])
	return path

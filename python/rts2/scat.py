# List catalogue entries
#
# (C) 2017 Petr Kubanek <petr@kubanek.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

import struct
import numpy
import rts2.libnova
import gzip

class Star:
	"""Star entry, read from the TBD catalogue."""
	def __init__(self,sent):
		self.xno, self.sra0, self.sdec0, self.isp, self.mag, self.xrpm, self.xdpm, self.name=struct.unpack('!idd2shff6s',sent)
		self.mag = self.mag / 100.0

	def __str__(self):
		return '{0} {1:.6f} {2:.6f} mag {3}'.format(self.xno, numpy.degrees(self.sra0), numpy.degrees(self.sdec0), self.mag)
	
	def get_coord(self):
		return (self.sra0, self.sdec0)

	def get_separation(self, ra, dec):
		return rts2.libnova.angular_separation_rad(ra, dec, self.sra0, self.sdec0)

def search_catalogue(ra0,dec0,degdist=0.5,mag0=None,mag1=None):
	#fn=astropy.utils.data.download_file('http://tdc-www.harvard.edu/catalogs/hipparcos.tar.gz',True)
	#f = gzip.GzipFile(fn,'r')

	f = open('/home/petr/hipparcos/hipparcos','rb')

	star0,star1,starn,stnum,mprop,nmag,nbent=struct.unpack('!iiiiiii',f.read(28))

	sn = star1

	ra = numpy.radians(ra0)
	dec = numpy.radians(dec0)
	rad_dist = numpy.radians(degdist)

	aret = []

	while sn < starn:
		astar = Star(f.read(nbent))
		if astar.get_separation(ra, dec) < rad_dist:
			aret.append(astar)
		sn += 1
	
	return aret

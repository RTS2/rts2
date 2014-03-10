#!/usr/bin/python
#
# Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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
#
import imgprocess
import scriptcomm

sc = scriptcomm.Rts2Comm()

class Centering(scriptcomm.Rts2Comm):
	"""Implements intelligend starfield search. Take an exposure, run image processing on it. If the field is not matched, offset in cross pattern around center and try again. Search in spiral pattern around expected position."""

	def __init__(self):
		scriptcomm.Rts2Comm.__init__(self)
		self.telname = self.getDeviceByType(scriptcomm.DEVICE_TELESCOPE)
		self.imgproc = imgprocess.ImgProcess()
		self.imgn = None
	
	def findOffset(self,ra_o,dec_o,exptime):
		self.setValue('OFFS','{0} {1}'.format(ra_o,dec_o),self.telname)
		ret = self.waitIdle(self.telname,300)
		self.log('D','offseting telescope {0} returns {1}'.format(self.telname,ret))
		self.setValue('exposure',exptime)
		self.imgn = self.exposure()
		offs = self.imgproc.run(self.imgn)
		self.log('I','returned offsets {0}'.format(str(offs)))
		self.setValue('CORR_','{0} {1}'.format(offs[2],offs[3]),self.telname)
		self.setValue('OFFS','0 0',self.telname)
		self.toArchive(self.imgn)

	def run(self,ra_offset,dec_offset,exptime,path=[[-1,0],[0,-1],[1,0],[0,1]]):
		"""ra_offset - offset size in degrees"""
		# try offsets in cross around postion
		for x in path:
			try:
				self.findOffset(x[0] * ra_offset,x[1] * dec_offset,exptime)
				return
			except Exception,ex:
				self.log('W','attempt on offset for {0} {1} failed:{2}'.format(x[0] * ra_offset, x[1] * dec_offset,ex))
				if self.imgn:
					self.toTrash(self.imgn)
		self.log('E','cannot match on given offsets, giving up')
		self.targetTempDisable(1200)
		self.endScript()

if __name__ == '__main__':
	c = Centering()
	c.run(1,1,10)

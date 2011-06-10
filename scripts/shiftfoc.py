#!/disk-a/obs12/kubanek/bin/python
#
# Autofocosing routines using shift-store.
#
# You will need: scipy matplotlib sextractor
# This should work on Debian/ubuntu:
# sudo apt-get install python-matplotlib python-scipy python-pyfits sextractor
#
# If you would like to see sextractor results, get DS9 and pyds9:
#
# http://hea-www.harvard.edu/saord/ds9/
#
# Please be aware that current sextractor Ubuntu packages does not work
# properly. The best workaround is to install package, and the overwrite
# sextractor binary with one compiled from sources (so you will have access
# to sextractor configuration files, which program assumes).
#
# (C) 2011      Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

import rts2comm
import shiftstore

class ShiftFoc (rts2comm.Rts2Comm):
	"""Take and process focussing data."""

	def __init__(self):
		self.exptime = 10 # 60 # 10
		self.step = 0.03 # 0.2
		self.attempts = 9
		self.focuser = self.getValue('focuser')

	def beforeReadout(self):
		self.current_focus = self.getValueFloat('FOC_POS',self.focuser)
		if (self.num == self.attempts):
			self.setValue('FOC_TOFF',0,self.focuser)
		else:
			self.off += self.step
			self.setValue('FOC_TOFF',self.off,self.focuser)

	def takeImages(self):
		self.setValue('exposure',self.exptime)
		self.setValue('WINDOW','1000 1000 100 100')
		self.setValue('SHUTTER','LIGHT')
		self.off = -1 * self.step * (self.attempts / 2)
		self.setValue('FOC_TOFF',self.off,self.focuser)
		# must be overwritten in beforeReadout
		self.current_focus = None

		# test exposure..
		self.log('I','starting empty 10 sec exposure')
		self.exposure()

		self.setValue('clrccd',0)
		self.setValue('WINDOW','0 0 2048 50')

		tries = {}

		for self.num in range(1,self.attempts):
		  	self.log('I','starting {0}s exposure on offset {1}'.format(self.exptime,self.off))
			img = self.exposure(self.beforeReadout)
			tries[self.current_focus] = img

		self.num = self.attempts

		self.log('I','last exposure with full readout')
		self.setValue('WINDOW','-1 -1 -1 -1')
		lastimg = self.exposure(self.beforeReadout)

		self.log('I','all focusing exposures finished, processing data')
		lastimg = self.rename(lastimg,'/disk-b/obs12_images/%N/focusing/%f')

		sc = shiftstore.ShiftStore()
		
		return sc.runOnImage(lastimg,False)

	def run(self):
		# send to some other coordinates if you wish so, or disable this for target for fixed coordinates
		#a.altaz (82,10)

		b,fit = self.takeImages()
		# transform value..
		tar = self.getValueFloat('FOC_DEF',self.focuser) + (b - (self.attempts / 2)) * self.step
		self.log('I','calculated {0} from {1} offset'.format(tar,b))
		self.setValue('FOC_DEF',tar,self.focuser)

if __name__ == "__main__":
	a = ShiftFoc()
	a.run()

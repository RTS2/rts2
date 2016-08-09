#!/usr/bin/python
#
# (C) 2016 Petr Kubanek <petr@kubanek.net>
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

class Spiral:
	def __init__(self,step_size_x=1,step_size_y=1):
		self.step = 0;
		self.step_size_x = step_size_x
		self.step_size_y = step_size_y
		self.step_size = step_size_x * 2 + step_size_y;
		self.up_d = 1;

	def get_next_step(self):
		if self.step == self.step_size:
			self.up_d *= -1
			if self.up_d == 1:
				self.step_size_x += 1
			self.step_size_y += 1
			self.step_size = self.step_size_x * 2 + self.step_size_y
			self.step = 0
		y = 0
		x = 0
		if self.step < self.step_size_x:
			y = 1
		elif self.step < self.step_size_x + self.step_size_y:
			x = 1
		else:
			y = -1
		y *= self.up_d
		x *= self.up_d
		self.step += 1
		return x,y

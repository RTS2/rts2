#!/usr/bin/python
# (C) 2004-2012, Markus Wildi, wildi.markus@bluewin.ch
#
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

__author__ = 'wildi.markus@bluewin.ch'

import sys
import rts2pa
import rts2.scriptcomm
r2c= rts2.scriptcomm.Rts2Comm()

class Measure(rts2pa.PAScript):
# __init__ of base class is executed
    def run(self):
        print '------{0}'.format(self.runTimeConfig.cf['CONFIGURATION_FILE'])
        


if __name__ == "__main__":

    measure= Measure(scriptName=sys.argv[0])
    measure.run()

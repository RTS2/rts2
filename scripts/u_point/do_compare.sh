#!/bin/bash
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

# 2016-12-07, wildi.markus@bluewin.ch
#
# do_it_all_dss.sh: set $alt_az_steps to decent value
#
cd $HOME/rts2/scripts/u_point
#
#./do_it_all_dss.sh
#
# -s: skip data acquisition 
#
#./do_it_all_dss.sh -s
#
#
#
./do_it_all_dss.sh -a -t transform_taki_san  -m bennett -o transform_taki_san_bennett
#
./do_it_all_dss.sh -s -a -t transform_taki_san  -m stone -o transform_taki_stone
#
./do_it_all_dss.sh -s -a -t transform_taki_san  -m saemundsson -o transform_taki_saemundsson
#
./do_it_all_dss.sh -s -a -t transform_libnova  -m bennett -o transform_libnova_bennett
#
./do_it_all_dss.sh -s -a -t transform_libnova  -m stone -o transform_libnova_stone
#
./do_it_all_dss.sh -s -a -l 47.5 -t transform_libnova  -m saemundsson -o transform_libnova_saemundsson
#
./do_it_all_dss.sh -s -a -t transform_astropy -o transform_astropy
# 
./do_it_all_dss.sh -s -a -t transform_pyephen -o transform_pyephen
#

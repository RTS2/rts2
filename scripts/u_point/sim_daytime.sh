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

# 2016-12-11, wildi.markus@bluewin.ch
#
# Demonstrate day time acquisition avoiding position
# within --sun-separation 
#
BASE_PATH=/tmp/u_point
alt_az_steps="--alt-step 5 --az-step 10"
LATITUDE="--obs-latitude m75.1"
LONGITUDE="--obs-longitude 350."
SUN_SEPARATION="--sun-separation 30."
# altitude limits 
LOW=10.
HIGH=50.
# do not fetch images
FETCH_DSS_IMAGE=
rm -fr $BASE_PATH
cd $HOME/rts2/scripts/u_point
./u_acquire.py --base-path $BASE_PATH $LATITUDE --create $alt_az_steps --altitude-interval "$LOW $HIGH"
./u_acquire.py --base-path $BASE_PATH  $LATITUDE $LONGITUDE --plot --animate $SUN_SEPARATION&
./u_acquire.py --base-path $BASE_PATH  $LATITUDE $LONGITUDE $FETCH_DSS_IMAGE $SUN_SEPARATION 



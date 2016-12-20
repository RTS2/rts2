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
# within --sun-separation and usin bright stars
#
BASE_PATH=/tmp/u_point
alt_az_steps="--alt-step 10 --az-step 40"
LATITUDE="--obs-latitude m75.1"
LONGITUDE="--obs-longitude 123.1"
SUN_SEPARATION="--sun-separation 40."
# altitude limits 
ALT_LOW=0.
ALT_HIGH=80.
AZ_LOW=0.
AZ_HIGH=360.
#AZ_LOW=130.
#AZ_HIGH=270.
# brightness limits
MAG_LOW=4.
MAG_HIGH=-5.
# do not fetch images
FETCH_DSS_IMAGE=

cd $HOME/rts2/scripts/u_point
while getopts ":d" opt; do
    case ${opt} in
	d )
	    DELETE=true
	    ;;
	\? ) echo "usage: sim_daytime_bright_stars.sh  OPTION"
	     echo ""
	     echo "OPTION:"
	     echo ""
	     echo "-d delete $BASE_PATH"
	     echo ""
	     echo ""
	     exit 1
	     ;;

    esac
done
if  [ -z ${DELETE} ]; then
    echo "do not delete base directory $BASE_PATH"
else
    echo "delete base directory $BASE_PATH"
    if [ -d "$BASE_PATH" ]; then
	if [[ $BASE_PATH == "/tmp"* ]]
	then
	    echo "delete inside /tmp"
	    rm -fr $BASE_PATH;
	else
	    echo "do not delete outside /tmp, exiting"
	    exit
	fi
    fi
    ./u_select.py --base-path $BASE_PATH --brightness-interval "$MAG_HIGH $MAG_LOW" $LATITUDE --plot > /dev/null 2>&1 &
    sleep 10
    ./u_acquire.py --base-path $BASE_PATH $LATITUDE --create $alt_az_steps --altitude-interval "$ALT_LOW $ALT_HIGH"
fi
./u_acquire.py --base-path $BASE_PATH  $LATITUDE $LONGITUDE --plot --animate $SUN_SEPARATION &
./u_acquire.py --base-path $BASE_PATH  $LATITUDE $LONGITUDE $FETCH_DSS_IMAGE $SUN_SEPARATION --use-bright-stars --toconsole



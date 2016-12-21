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
cd $HOME/rts2/scripts/u_point
#
SIM_SETTINGS="./sim/sim_settings.sh"

while getopts ":rs:" opt; do
    case ${opt} in
	r )
	    RTS2=true
	    ;;
	s )

	    SIM_SETTINGS=$OPTARG
	    ;;
	\? ) echo "usage: do_it_all_dss.sh  OPTION"
	     echo ""
	     echo "OPTION: $OPTARG"
	     echo ""
	     echo "-r use RTS2 devices"
	     echo "-s path to simulation settings"
	     echo ""
	     exit
    esac
done
if [ -f "$SIM_SETTINGS" ]; then
    source $SIM_SETTINGS
else
    echo "$0 is executed in $HOME/rts2/script/u_point"
    echo "specify settings path relative to the main"
    echo "u_point directory"
    echo "$SIM_SETTINGS does not exist, exiting"
    exit
fi

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
echo "if no sun is visible, change longitide"

if ! [ -z ${RTS2+x} ]; then
    echo "using real or dummy RTS2 devices"
    ./u_acquire.py --base-path $BASE_PATH $LATITUDE --create $alt_az_steps --latitude-interval "$ALT_LOW $ALT_HIGH" --longitude-interval "$AZ_LOW $AZ_HIGH"
    #
    plt_script=" $HOME/rts2/scripts/u_point/rts2_script/sim_u_acquire_plot.sh "
    #
    rts2-scriptexec -d C0 -s " exe $plt_script " &
    #
    acq_script=" $HOME/rts2/scripts/u_point/rts2_script/sim_u_acquire_no_fetch_dss_continuous.sh "
    #
    #
    rts2-scriptexec -d C0 -s " exe $acq_script "  &
else
    echo "using built in simulation device: DeviceDss"
    #
    ./u_acquire.py --base-path $BASE_PATH $LATITUDE --create $alt_az_steps --latitude-interval "$ALT_LOW $ALT_HIGH" --longitude-interval "$AZ_LOW $AZ_HIGH"
    set -x
    ./u_acquire.py --base-path $BASE_PATH $LATITUDE $LONGITUDE --plot --animate $SUN_SEPARATION &
    #
    ./u_acquire.py --base-path $BASE_PATH $LATITUDE $LONGITUDE $FETCH_DSS_IMAGE $SUN_SEPARATION --toconsole
fi





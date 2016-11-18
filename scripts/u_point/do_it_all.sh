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

# 2016-11-13, wildi.markus@bluewin.ch
#
# Demonstrate u_point in simulation mode without RTS2.
#
# Taking in almost all cases the argument's default values.
# Site: Dome Concordia, Antarctica
#
# No RTS2 installation required
#
export BASE_DIRECTORY=/tmp/u_point
echo "clean sweep in base directory $BASE_DIRECTORY"
# clean sweep
rm -fr $BASE_DIRECTORY
# since u_point is not yet a python package
cd $HOME/rts2/scripts/u_point
#
echo "------------------------------------------------"
echo "first step, create site specific observable objects catalog and plot it"
echo "fetching 9109 objects, close plot window to continue"
echo "./u_select.py --base-path $BASE_DIRECTORY --brightness-interval "5.0 7.0" --plot --level DEBUG --toconsole"
#
./u_select.py --base-path $BASE_DIRECTORY --brightness-interval "5.0 7.0" --plot --level DEBUG --toconsole
#
echo "------------------------------------------------"
echo "second step, define a nominal grid of alt az positions and plot it"
echo "the grid as an az step of 40. deg (fewer objects)"
echo "close window to continue"
echo "./u_acquire.py --base-path $BASE_DIRECTORY --create --plot --az-step 40"
#
./u_acquire.py --base-path $BASE_DIRECTORY --create --plot --az-step 40 --toconsole
#
echo "DONE grid creation"
echo "------------------------------------------------"
echo "third step, acquire positions from DSS: http://archive.eso.org/dss/dss"
echo "since we do not use real equipment"
echo "dummy meteo data are provided by meteo.py"
echo "progress report: once u_acquire.py is running:"
echo "- open an new terminal and do a: tail -f /tmp/u_acquire.log"
echo "- create a progress plot:  ./u_acquire.py --base-path $BASE_DIRECTORY   --plot"
echo "./u_acquire.py --base-path $BASE_DIRECTORY  --fetch-dss-image  --mode-continues --level DEBUG"
#
./u_acquire.py --base-path $BASE_DIRECTORY  --fetch-dss-image  --mode-continues --level DEBUG --toconsole
#
echo "DONE position acqusition"
echo "------------------------------------------------"
echo "forth step, analyze the acquired position"
echo "in this demo astrometry.net is disabled, since it is too slow for this purpose"
echo "progress report: once u_analyze.py is running:"
echo "- open an new terminal and do a: tail -f /tmp/u_analzse.log"
echo "- create a progress plot:  ./u_analyze.py --base-path $BASE_DIRECTORY  --plot"
echo "./u_analyze.py  --base-path $BASE_DIRECTORY  --do-not-use-astrometry --level DEBUG"
#
./u_analyze.py  --base-path $BASE_DIRECTORY  --do-not-use-astrometry --level DEBUG --toconsole
#
echo "DONE analysis of all positions"
echo "------------------------------------------------"
echo "fifth step, create pointing model"
echo "./u_point.py --base-path $BASE_DIRECTORY --mount-data  u_point_positions_sxtr.anl --plot --level DEBUG"
#
./u_point.py --base-path $BASE_DIRECTORY --mount-data  u_point_positions_sxtr.anl --plot --level DEBUG --toconsole

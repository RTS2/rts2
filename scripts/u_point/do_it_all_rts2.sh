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

# 2016-11-22, wildi.markus@bluewin.ch
#
# Demonstrate u_point in simulation mode with RTS2 dummy devices, see README.
#
# Taking in almost all cases the argument's default values.
# Site: Dome Concordia, Antarctica
#
# No RTS2 installation required
#
quit()
{
    echo "signal received, exiting"
    if ! [ -z ${pid_u_acquire+x} ];
    then
	kill  -s SIGKILL $pid_u_acquire
    fi
    if ! [ -z ${pid_u_analyze+x} ];
    then
	kill  -s SIGKILL $pid_u_analyze
    fi
    echo "my PID: " $$
    kill -s SIGKILL $$
    exit 0
}

trap 'quit' QUIT
trap 'quit'  INT
trap 'quit'  HUP

export BASE_PATH=/tmp/u_point
##export BASE_PATH=$HOME/u_point
echo "clean sweep in base directory $BASE_PATH"
# clean sweep
rm -fr $BASE_PATH
# since u_point is not yet a python package
cd $HOME/rts2/scripts/u_point
#
echo "------------------------------------------------"
echo "first step, create site specific observable objects catalog and plot it"
echo "fetching 9109 objects"
echo "./u_select.py --base-path $BASE_PATH --brightness-interval "5.0 7.0" --plot --level DEBUG --toconsole"
#
echo
echo "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"
echo "close plot window to continue"
echo "takes several 30 seconds..."

./u_select.py --base-path $BASE_PATH --brightness-interval "1.0 5.0" --plot #--toconsole
#
echo "------------------------------------------------"
echo "second step, define a nominal grid of alt az positions and plot it"
echo "the grid as an az step of 40. deg (fewer objects)"
echo
echo "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"
echo "close plot window to continue"
echo "./u_acquire.py --base-path $BASE_PATH --create --plot --az-step 40"
#
./u_acquire.py --base-path $BASE_PATH --create --plot --az-step 40 --toconsole
#
echo "DONE grid creation"
echo "------------------------------------------------"
echo "third step, acquire positions from DSS: http://archive.eso.org/dss/dss"
echo "since we do not use real equipment"
echo "dummy meteo data are provided by meteo.py"
echo "progress report: once u_acquire.py is running:"
echo "- open an new terminal and do a: tail -f /tmp/u_acquire.log"
echo "./u_acquire.py --base-path $BASE_PATH  --fetch-dss-image  --mode-continues --level DEBUG"
#
acq_script=" $HOME/rts2/scripts/u_point/u_acquire_fetch_dss_continuous.sh "
#
rts2-scriptexec -d C0 -s " exe $acq_script "  &
#
export pid_u_acquire=$!
plt_script=" $HOME/rts2/scripts/u_point/u_acquire_plot.sh "
#
# to see the mount coordinates in progress plot
#
rts2-scriptexec -d C0 -s " exe $plt_script " &
echo
echo "<<<<<<<<<<<<<<<<<<<<<<<"
echo "leave plot window open"
echo "click on blue points to see FITS being displayed with DS9"
#
if ps -p $pid_u_acquire >/dev/null; then

    echo ""
    echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
    echo "waiting for u_acquire.py to terminate"
    wait $pid_u_acquire
fi
echo
echo "<<<<<<<<<<<<<<<<<<<<<<<<"
echo "DONE u_acquire.py"
echo
echo "------------------------------------------------"
echo "forth step, analyze the acquired position"
echo "in this demo astrometry.net is disabled, since it is too slow for this purpose"
echo "progress report: once u_analyze.py is running:"
echo "- open an new terminal and do a: tail -f /tmp/u_analzse.log"
echo "./u_analyze.py  --base-path $BASE_PATH  --do-not-use-astrometry --level DEBUG"
#
./u_analyze.py  --base-path $BASE_PATH  --do-not-use-astrometry --level DEBUG --toconsole &
## use astrometry.net
##./u_analyze.py  --base-path $BASE_PATH   --level DEBUG --toconsole &
export pid_u_analyze=$!
./u_analyze.py --base-path $BASE_PATH --plot --ds9-display --animate &
#
echo
echo "<<<<<<<<<<<<<<<<<<<<<<<<"
echo "leave plot window open"
echo "click on blue points to see FITS being displayed with DS9"
echo "since option u_acquire.py --use-bright-stars was disabled"
echo "magenta points (result from SExtractor) will be not"
echo "excactly centered."
if ps -p $pid_u_analyze >/dev/null; then
    echo ""
    echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
    echo "waiting for u_analyze.py to terminate"
    wait $pid_u_analyze
fi
#
echo "DONE u_analyze.py"
echo "------------------------------------------------"
echo "fifth step, create pointing model"
echo "./u_point.py --base-path $BASE_PATH --mount-data  u_point_positions_sxtr.anl --plot --level DEBUG"
echo
echo "<<<<<<<<<<<<<<<<<<<<<<<<"
echo "leave plot windows open and click on those labled CLICK ME"
echo "click on blue points to see FITS being displayed with DS9"
echo
echo
script=`basename "$0"`
echo "To terminate script $script, type CTRL-Z followd by 'kill %PID', PID appears at bottom left as [PID]"
echo "as e.g. [1]+  Stopped  ./do_it_all.sh"
echo
echo
#
# use file u_point_positions_sxtr.anl only for demonstration purposes
# 
./u_point.py --base-path $BASE_PATH --mount-data  u_point_positions_sxtr.anl --plot --level DEBUG --toconsole --ds9
#
# with results from astrometry.net
#./u_point.py --base-path $BASE_PATH --mount-data  u_point_positions_astr.anl --plot --level DEBUG --toconsole --ds9

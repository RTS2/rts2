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

while getopts ":apr" opt; do
    case ${opt} in
	a ) # process option a
	    ASTROMETRY=true
	;;
	p ) # process option a
	    PRESERVE=true
	;;
	r ) # process option l
	    RTS2=true
	;;
	\? ) echo "usage: do_it_all_dss.sh [-a]: use astrometry.net (slow) [-p]: do not delete base path [-r]: use RTS2 "
	     exit
      ;;
  esac
done

export BASE_PATH=/tmp/u_point
##export BASE_PATH=$HOME/u_point
if ! [ -z ${PRESERVE+x} ]; then
    echo "do not delete base directory $BASE_PATH"
    echo "skipping u_select.py, u_acquire.py --create"
    skip=true
else
    echo "clean sweep in base directory $BASE_PATH"
    # clean sweep
    rm -fr $BASE_PATH
fi
#
if ! [ -z ${RTS2+x} ]; then
    echo "using device u_point DeviceRts2"
else
    echo "using device u_point DeviceDss"
fi

if ! [ -z ${ASTROMETRY+x} ]; then
    echo "using SExtractor and astrometry.net"
else
    echo "using SExtractor only"
fi
#
# since u_point is not yet a python package
cd $HOME/rts2/scripts/u_point
#
if [ -z ${skip+x} ]; then
    #
    echo "------------------------------------------------"
    echo "first step, create site specific observable objects catalog and plot it"
    echo "fetching 9109 objects"
    echo "./u_select.py --base-path $BASE_PATH --brightness-interval "5.0 7.0" --plot"
    #
    echo ""
    echo "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
    echo "takes several 15 seconds..."
    echo "close plot window to continue"
    echo ""
    echo "./u_select.py --base-path $BASE_PATH --brightness-interval "5.5 6.5" --plot"
    #
    ./u_select.py --base-path $BASE_PATH --brightness-interval "5.5 6.5" --plot 
    #rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
    #
    echo "DONE selecting stars"
fi
if [ -z ${skip+x} ]; then
    echo "------------------------------------------------"
    echo "second step, define a nominal grid of alt az positions and plot it"
    echo "the grid as an az step of 40. deg (fewer objects)"
    echo ""
    echo "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
    echo "close plot window to continue"
    echo ""
    echo "./u_acquire.py --base-path $BASE_PATH --create --plot --az-step 40"
    #
    ./u_acquire.py --base-path $BASE_PATH --create --plot --alt-step 5 #--az-step 40
    #rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
    #
    echo "DONE grid creation"
fi
echo "------------------------------------------------"
echo "third step, acquire positions from DSS: http://archive.eso.org/dss/dss"
echo "since we do not use real equipment"
echo "dummy meteo data are provided by meteo.py"
echo "once u_acquire.py is running:"
echo "- open an new terminal and do a: tail -f /tmp/u_acquire.log"
echo "or just watch the progress in the plot window"
echo ""
if ! [ -z ${RTS2+x} ]; then
    #
    acq_script=" $HOME/rts2/scripts/u_point/u_acquire_fetch_dss_continuous.sh "
    #
    echo "rts2-scriptexec -d C0 -s " exe $acq_script "  &"
    #
    rts2-scriptexec -d C0 -s " exe $acq_script "  &
    export pid_u_acquire=$!
else
    echo "./u_acquire.py --base-path $BASE_PATH  --fetch-dss-image --use-bright-stars &"
    #
    ./u_acquire.py --base-path $BASE_PATH  --fetch-dss-image  --use-bright-stars &
    #
    # no bright stars:
    #./u_acquire.py --base-path $BASE_PATH  --fetch-dss-image --toconsole &
    #
    # no internet connection:
    #./u_acquire.py --base-path $BASE_PATH --toconsole &
    #
    export pid_u_acquire=$!
fi
if ! [ -z ${RTS2+x} ]; then

    plt_script=" $HOME/rts2/scripts/u_point/u_acquire_plot.sh "
    #
    echo "rts2-scriptexec -d C0 -s " exe $plt_script " &"
    #
    # to see the mount coordinates in progress plot
    #
    rts2-scriptexec -d C0 -s " exe $plt_script " &
else
    ./u_acquire.py --base-path $BASE_PATH --plot --ds9-display --animate --delete --toconsole --level DEBUG &
fi
echo ""
echo "^^^^^^^^^^^^^^^^^^^^^^"
echo "leave plot window open"
echo "click on blue points to see FITS being displayed with DS9"
#
if ps -p $pid_u_acquire >/dev/null; then
    echo ""
    echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
    echo "waiting for u_acquire to terminate, PID $pid_u_acquire"
    echo "if plot is not clickable, like in my case, open another,"
    echo "issue: "
    echo " cd $HOME/rts2/scripts/u_point ; ./u_acquire.py --base-path $BASE_PATH --plot --ds9-display --animate --delete --toconsole "
    wait $pid_u_acquire
    #rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
fi
echo ""
echo "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
echo "DONE u_acquire.py"
echo
echo "------------------------------------------------"
echo "forth step, analyze the acquired position"
echo "in this demo astrometry.net is disabled, since it is too slow for this purpose"
echo "once u_analyze.py is running:"
echo "- open an new terminal and do a: tail -f /tmp/u_analzse.log"
echo "or just watch the progress in the plot window"
echo ""
if ! [ -z ${ASTROMETRY+x} ]; then
    echo "./u_analyze.py  --base-path $BASE_PATH --toconsole --mount-type-eq&"
    #
    ./u_analyze.py  --base-path $BASE_PATH   --timeout 30 --toconsole --mount-type-eq&
    export pid_u_analyze=$!
else
    echo "./u_analyze.py  --base-path $BASE_PATH  --do-not-use-astrometry --toconsole --mount-type-eq&"
    #
    ./u_analyze.py  --base-path $BASE_PATH  --do-not-use-astrometry --toconsole --mount-type-eq&
    export pid_u_analyze=$!
fi
#
./u_analyze.py --base-path $BASE_PATH --plot --ds9-display --animate --delete&
echo ""
echo "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
echo "leave plot window open"
echo "click on blue points to see FITS being displayed with DS9"
if ps -p $pid_u_analyze >/dev/null; then
    echo ""
    echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
    echo "waiting for u_analyze.py to terminate, PID $pid_u_analyze"
    wait $pid_u_analyze
    #rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
fi
#
echo "DONE u_analyze.py"
echo "------------------------------------------------"
echo "fifth step, create pointing model"
echo ""
echo "display results from SExtractor"

echo "./u_point.py --base-path $BASE_PATH  --plot --ds9-display --delete --toconsole"
./u_point.py --base-path $BASE_PATH  --plot --ds9-display --delete --toconsole  &
#rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
if ! [ -z ${ASTROMETRY+x} ]; then
    echo ""
    echo "to continue with SExtractor analysis, press any key."
    echo "pherhaps close all u_point.py plot windows first."
    echo ""
    read cont
    
    echo ""
    echo "now displaying results from SExtractor, wait a moment"
    echo "./u_point.py --base-path $BASE_PATH --fit-sextr --ds9-display --delete --plot --toconsole"
    ./u_point.py --base-path $BASE_PATH  --fit-sextr --plot --ds9-display --delete --toconsole --ds9 &
fi
echo ""
echo "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
echo "leave plot windows open"
echo "click on blue points to see FITS being displayed with DS9"
echo ""
echo ""

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
export latitude="--obs-latitude 89.5"
#export latitude=""
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

function cont_exit {
    "$@"
    local status=$?
    if [ $status -ne 0 ]; then
	echo "error with $1, status $status" >&2
	exit
    fi
    return $status
}

while getopts :sapr:t:m: opt; do
    case ${opt} in
	a )
	    ASTROMETRY=true
	    ;;
	p ) 
	    PRESERVE=true
	    ;;
	s )
	    SKIP_ACQUISITION=true
	    ;;
	r ) 
	    RTS2=true
	    ;;
	m )
	    REFRACTION_METHOD=$OPTARG
	    echo $REFRACTION_METHOD
	    ;;
	t )
	    TRANSFORMATION_CLASS=$OPTARG
	    echo $TRANSFORMATION_CLASS
	    ;;
       
	\? ) echo "usage: do_it_all_dss.sh  OPTION"
	     echo ""
	     echo "OPTION:"
	     echo ""
	     echo "-a use astrometry.net and SExtractor"
	     echo "-p do not delete base path"
	     echo "-r use RTS2 devices"
	     echo "-m refraction method, if applicable (see -t), see u_analyze.py --help: --refraction-method"
	     echo "-t transformation class, see u_analyze.py --help --help: --transform-class "
	     echo "   transform_taki_san: a refraction method must be specified"
	     echo "   transform_libnova : a refraction method can be specified"
	     echo "   transform_pyephem : a refraction method can not yet be specified"
	     echo "   transform_astropy : no refraction method can be specified"
	     exit 1
	     ;;
    esac
done
echo""
export BASE_PATH=/tmp/u_point
##export BASE_PATH=$HOME/u_point

if ! [ -z ${SKIP_ACQUISITION+x} ]; then
    echo "do not delete base directory $BASE_PATH"
    echo "skipping u_select.py, u_acquire.py --create and u_acquire.py (no data fetching)"
    skip_acquisition=true
    PRESERVE=true
fi
echo ""
if ! [ -z ${RTS2+x} ]; then
    echo "using real or dummy RTS2 devices"
else
    echo "using built in simulation device: DeviceDss"
fi
echo ""
if ! [ -z ${ASTROMETRY+x} ]; then
    echo "using SExtractor and astrometry.net"
else
    echo "using SExtractor only"
fi
echo ""
if  [ -z ${REFRACTION_METHOD+x} ]; then
    echo "using refraction method: default"
    refraction_method=
else
    echo "using refraction method: $REFRACTION_METHOD"
    refraction_method="--refraction-method $REFRACTION_METHOD"
fi
echo ""
if  [ -z ${TRANSFORMATION_CLASS+x} ]; then
    echo "using transformation class: default"
    transform_class=
else
    echo "using transformation class: $TRANSFORMATION_CLASS"
    transform_class="--transform-class $TRANSFORMATION_CLASS"
fi
echo ""
if  [ -z ${PRESERVE+x} ] ; then
    echo "clean sweep in base directory $BASE_PATH"
    rm -fr $BASE_PATH
else
    echo "do not delete base directory $BASE_PATH"
fi
#
#
#
# since u_point is not yet a python package
cd $HOME/rts2/scripts/u_point
#
if [ -z ${skip_acquisition+x} ]; then
    #
    echo "-----------------------------------------------------------------"
    echo "first step, create site specific observable objects catalog and plot it"
    echo "fetching 9109 objects"
    #
    echo ""
    echo "-----------------------------------------------------------------"
    echo "takes several 5 seconds..."
    echo "CLOSE plot window to continue"
    echo ""
    #echo "./u_select.py --base-path $BASE_PATH --brightness-interval "5.5 6.5" --plot"
    #
    #cont_exit ./u_select.py --base-path $BASE_PATH --brightness-interval "5.5 6.5" $latitude --plot > /dev/null 2>&1
    cont_exit ./u_select.py --base-path $BASE_PATH --brightness-interval "6.0 7.0" $latitude --plot > /dev/null 2>&1
    #
    echo "DONE selecting stars"
fi
if [ -z ${skip_acquisition+x} ]; then
    echo "-----------------------------------------------------------------"
    echo "second step, define a nominal grid of alt az positions and plot it"
    echo "the grid has an az step of 40. deg (fewer objects)"
    echo ""
    echo "-----------------------------------------------------------------"
    echo "CLOSE plot window to continue"
    echo ""
    #echo "cont_exit ./u_acquire.py --base-path $BASE_PATH --create --plot --az-step 40"
    #
    cont_exit ./u_acquire.py --base-path $BASE_PATH $latitude --create --plot --az-step 40
    #
    echo "DONE grid creation"
fi
echo "-----------------------------------------------------------------"
echo "third step, acquire positions from DSS: http://archive.eso.org/dss/dss"
echo "since we do not use real equipment"
echo "dummy meteo data are provided by meteo.py"
echo ""
if  [ -z ${SKIP_ACQUISITION+x} ]; then
    if ! [ -z ${RTS2+x} ]; then
	#
	acq_script=" $HOME/rts2/scripts/u_point/u_acquire_fetch_dss_continuous.sh "
	#
	#echo "rts2-scriptexec -d C0 -s " exe $acq_script "  &"
	#
	rts2-scriptexec -d C0 -s " exe $acq_script "  &
	export pid_u_acquire=$!
    else
	#echo "cont_exit ./u_acquire.py --base-path $BASE_PATH  --fetch-dss-image --use-bright-stars &"
	#
	cont_exit ./u_acquire.py --base-path $BASE_PATH  $latitude --fetch-dss-image  --use-bright-stars --toconsole&
	#
	# no bright stars:
	#cont_exit ./u_acquire.py --base-path $BASE_PATH  --fetch-dss-image --toconsole &
	#
	# no internet connection:
	#cont_exit ./u_acquire.py --base-path $BASE_PATH --toconsole &
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
	cont_exit ./u_acquire.py --base-path $BASE_PATH $latitude --plot --ds9-display --animate --delete --toconsole --level DEBUG &
    fi

    echo ""
    echo "-----------------------------------------------------------------"
    echo "leave plot window open"
    echo "click on blue points to see FITS being displayed with DS9"
    #
    if ps -p $pid_u_acquire >/dev/null; then
	echo ""
	echo "-----------------------------------------------------------------"
	echo "waiting for u_acquire to terminate, PID $pid_u_acquire"
	echo "if plot is not clickable, issue: "
	echo ""
	echo " cd $HOME/rts2/scripts/u_point ; ./u_acquire.py --base-path $BASE_PATH --plot --ds9-display --animate --delete --toconsole "
	echo "-----------------------------------------------------------------"
	wait $pid_u_acquire
    fi
    echo ""
    echo "-----------------------------------------------------------------"
    echo "DONE u_acquire.py"
    echo "-----------------------------------------------------------------"
fi
echo "forth step, analyze the acquired position"
if ! [ -z ${ASTROMETRY+x} ]; then
    #
    set -x
    cont_exit ./u_analyze.py  --base-path $BASE_PATH  $latitude  --timeout 30 --toconsole $transform_class $refraction_method &
    set +x
    export pid_u_analyze=$!
else
    #
    set -x
    cont_exit ./u_analyze.py  --base-path $BASE_PATH $latitude --do-not-use-astrometry  --toconsole $transform_class $refraction_method &
    set +x
    export pid_u_analyze=$!
fi
#
cont_exit ./u_analyze.py --base-path $BASE_PATH --plot --ds9-display --animate --delete $transform_class $refraction_method &
echo ""
echo "-----------------------------------------------------------------"
echo "leave plot window open"
echo "click on blue points to see FITS being displayed with DS9"
if ps -p $pid_u_analyze >/dev/null; then
    echo ""
    echo "-----------------------------------------------------------------"
    echo "waiting for u_analyze.py to terminate, PID $pid_u_analyze"
    wait $pid_u_analyze
fi
#
echo "-----------------------------------------------------------------"
echo "DONE u_analyze.py"
echo ""
echo "-----------------------------------------------------------------"
echo "fifth step, create pointing model"
echo ""
if ! [ -z ${ASTROMETRY+x} ]; then
    echo ""
    echo "displaying results from astrometry.net"
    cont_exit ./u_point.py --base-path $BASE_PATH  $latitude --plot --ds9-display --delete --toconsole --model-class model_u_point --ds9& 
else
    echo "displaying results from SExtractor"
    cont_exit ./u_point.py --base-path $BASE_PATH  $latitude --plot --ds9-display --delete --toconsole  --model-class model_u_point --fit-sxtr& 

fi
echo ""
echo "-----------------------------------------------------------------"
echo "leave plot windows open"
echo "click on blue points to see FITS being displayed with DS9"
echo ""
echo ""

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
alt_az_steps="--alt-step 5 --az-step 20"
alt_az_steps="--alt-step 10 --az-step 40"
#LATITUDE="--obs-latitude m75.1"
PLOT="--plot"
SKIP_ACQUISITION=
BASE_PATH=/tmp/u_point
MODEL_CLASS="--model-class point"
FETCH_DSS_IMAGE="--fetch-dss-image"

trap "exit 1" TERM
export TOP_PID=$$
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
	#echo "error with $1, status $status" >&2
	kill -s TERM $TOP_PID > /dev/null 2>&1
    fi
    return $status
}

while getopts ":apsrdil:o:m:t:f:c:" opt; do
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
	d )
	    PLOT=
	    ;;
	i )
	    FETCH_DSS_IMAGE=
	    ;;
	l )
	    LATITUDE="--obs-latitude $OPTARG"
	    ;;
	o )
	    FN=$OPTARG
	    ANALYSIS_OUTPUT_FILE="--analyzed-positions $FN"
	    ;;
	m )
	    REFRACTION_METHOD=$OPTARG
	    ;;
	t )
	    TRANSFORMATION_CLASS=$OPTARG
	    ;;
	f )
	    BASE_PATH=$OPTARG
	    ;;
	c )
	    MODEL_CLASS="--model-class $OPTARG"
	    ;;

	\? ) echo "usage: do_it_all_dss.sh  OPTION"
	     echo ""
	     echo "OPTION:"
	     echo ""
	     echo "-a use astrometry.net and SExtractor"
	     echo "-p do not delete base path"
	     echo "-s skip selecting stars, creation of nominal position and acquisition of data"
	     echo "-r use RTS2 devices"
	     echo "-d do not plot intermediate steps"
	     echo "-i do not fetch DSS image from http://archive.eso.org/dss/dss"
	     echo "-l observatory latitude"
	     echo "-o analysis output filename"
	     echo "-m refraction method, if applicable (see -t), see u_analyze.py --help: --refraction-method"
	     echo "-t transformation class, see u_analyze.py --help --help: --transform-class "
	     echo "   u_taki_san: a refraction method must be specified"
	     echo "   u_libnova : a refraction method can be specified"
	     echo "   u_pyephem : a refraction method can not yet be specified"
	     echo "   u_astropy : no refraction method can be specified"
	     echo "-f base path"
	     echo "-c pointing model, one of (point|buie|altaz)"
	     exit 1
	     ;;
	:)
	    echo "Option -$OPTARG requires an argument." >&2
	    exit 1
    esac
done

if ! [ -z ${SKIP_ACQUISITION} ]; then
    echo "do not delete base directory $BASE_PATH"
    echo "skipping u_select.py, u_acquire.py --create and u_acquire.py (no data fetching)"
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
if  [ -z ${PRESERVE} ] ; then
    echo "clean sweep in base directory $BASE_PATH"
    rm -fr $BASE_PATH
else
    echo "do not delete base directory $BASE_PATH"
fi
#
# since u_point is not yet a python package
cd $HOME/rts2/scripts/u_point
if [ -z ${SKIP_ACQUISITION} ]; then
    #
    if  [ -z ${PRESERVE} ] ; then

	echo "-----------------------------------------------------------------"
	echo "first step, create site specific observable objects catalog"
	echo "fetching 9109 objects"
	echo "takes several 5 seconds..."
	if ! [ -z ${PLOT} ]; then
	    echo "plotting the results"
	    echo "-----------------------------------------------------------------"
	    echo "takes several 5 seconds..."
	    echo "CLOSE plot window to continue"
	    echo ""
	    cont_exit ./u_select.py --base-path $BASE_PATH --brightness-interval "6.0 7.0" $LATITUDE $PLOT > /dev/null 2>&1
	else
	    cont_exit ./u_select.py --base-path $BASE_PATH --brightness-interval "6.0 7.0" $LATITUDE  > /dev/null 2>&1
	fi
	#
	echo "DONE selecting stars"
	echo "-----------------------------------------------------------------"
	if ! [ -z ${PLOT} ]; then
	    echo "second step, define a nominal grid of alt az positions and plot it"
	    echo "-----------------------------------------------------------------"
	    echo "CLOSE plot window to continue"
	    echo ""
	    cont_exit ./u_acquire.py --base-path $BASE_PATH $LATITUDE --create $PLOT $alt_az_steps
	else
	    echo "second step, define a nominal grid of alt az positions"
	    cont_exit ./u_acquire.py --base-path $BASE_PATH $LATITUDE --create $alt_az_steps    
	fi
	#
	echo "DONE grid creation"
    fi
    echo "-----------------------------------------------------------------"
    echo "third step, acquire positions from DSS: http://archive.eso.org/dss/dss"
    echo "since we do not use real equipment"
    echo "dummy meteo data are provided by meteo.py"
    echo ""
    if ! [ -z ${RTS2+x} ]; then
	#
	acq_script=" $HOME/rts2/scripts/u_point/rts2_script/u_acquire_fetch_dss_continuous.sh "
	#
	#echo "rts2-scriptexec -d C0 -s " exe $acq_script "  &"
	#
	rts2-scriptexec -d C0 -s " exe $acq_script "  &
	export pid_u_acquire=$!
    else
	#
	cont_exit ./u_acquire.py --base-path $BASE_PATH  $LATITUDE $FETCH_DSS_IMAGE  --use-bright-stars --toconsole &
	export pid_u_acquire=$!
	#
	# no bright stars:
	#cont_exit ./u_acquire.py --base-path $BASE_PATH  --fetch-dss-image --toconsole &
	#
	# no internet connection:
	#cont_exit ./u_acquire.py --base-path $BASE_PATH --toconsole &
	#
    fi
    if ! [ -z ${RTS2+x} ]; then
	
	plt_script=" $HOME/rts2/scripts/u_point/rts2_script/u_acquire_plot.sh "
	#
	echo "rts2-scriptexec -d C0 -s " exe $plt_script " &"
	#
	# to see the mount coordinates in progress plot
	#
	rts2-scriptexec -d C0 -s " exe $plt_script " &
    else
	echo ""
	if ! [ -z ${PLOT} ]; then
	    # nap is necessary for callback to work
	    cont_exit ./u_acquire.py --base-path $BASE_PATH $LATITUDE $PLOT --ds9-display --animate --delete --level DEBUG &
	    echo "-----------------------------------------------------------------"
	    echo "leave acquire plot window open"
	    echo "click on blue points to see FITS being displayed with DS9"
	fi
    fi
    #
    if ps -p $pid_u_acquire >/dev/null; then
	echo ""
	echo "-----------------------------------------------------------------"
	echo "waiting for u_acquire to terminate, PID $pid_u_acquire"
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
    cont_exit ./u_analyze.py  --base-path $BASE_PATH  $LATITUDE  --timeout 30 --toconsole $ANALYSIS_OUTPUT_FILE $transform_class $refraction_method &
    export pid_u_analyze=$!
else
    #
    cont_exit ./u_analyze.py  --base-path $BASE_PATH $LATITUDE --do-not-use-astrometry  --toconsole $ANALYSIS_OUTPUT_FILE $transform_class $refraction_method &
    export pid_u_analyze=$!
fi
#
if ! [ -z ${PLOT} ]; then
    cont_exit ./u_analyze.py --base-path $BASE_PATH $PLOT --ds9-display --animate --delete $ANALYSIS_OUTPUT_FILE $transform_class $refraction_method &
    echo ""
    echo "-----------------------------------------------------------------"
    echo "leave analysis plot window open"
    echo "click on blue points to see FITS being displayed with DS9"
fi
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
    cont_exit ./u_model.py --base-path $BASE_PATH  $LATITUDE --plot --ds9-display --delete --toconsole $MODEL_CLASS --ds9 $ANALYSIS_OUTPUT_FILE& 
else
    echo "displaying results from SExtractor"
    cont_exit ./u_model.py --base-path $BASE_PATH  $LATITUDE --plot --ds9-display --delete --toconsole  --fit-sxtr $MODEL_CLASS  $ANALYSIS_OUTPUT_FILE& 

fi

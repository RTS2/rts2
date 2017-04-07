#!/bin/bash
#
# wildi.markus@bluewin.ch
#
# Helper script used by
#
# rts2-scriptexec -d C0 -s " exe ./u_acquire_fetch_dss_continuous.sh"
#
# exe can not deal with arguments
#
# ATTENTION: these settings are used for demonstration and debug
#            purposes ONLY.
# 
BASE_PATH="/tmp/u_point_unittest"
LATITUDE="--obs-latitude m75.1"
LONGITUDE="--obs-longitude 123.1"
#USE_BRIGHT_STARS="--use-bright-stars"
USE_BRIGHT_STARS=
FETCH_DSS_IMAGES="--fetch-dss-image"
SUN_SEPARATION="--sun-separation 40."
DO_QUICK_ANALYSIS="--do-quick-analysis"
#
#
# same with DSS image retrieval
../u_acquire.py --base-path  $BASE_PATH $LATITUDE  $LONGITUDE --device DeviceRts2 $USE_BRIGHT_STARS $FETCH_DSS_IMAGES $SUN_SEPARATION $DO_QUICK_ANALYSIS  --level DEBUG 


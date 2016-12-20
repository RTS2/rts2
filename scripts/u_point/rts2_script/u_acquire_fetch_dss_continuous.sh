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
cd $HOME/rts2/scripts/u_point/
source ./rts2_script/u_acquire_settings.sh
# overriding
USE_BRIGHT_STARS="--use-bright-stars"
#
#
# same with DSS image retrieval
./u_acquire.py --base-path  $BASE_PATH $LATITUDE  $LONGITUDE --device DeviceRts2 $USE_BRIGHT_STARS $FETCH_DSS_IMAGES $SUN_SEPARATION $DO_QUICK_ANALYSIS $ACQUIRE_DS9_DISPLAY --level DEBUG 


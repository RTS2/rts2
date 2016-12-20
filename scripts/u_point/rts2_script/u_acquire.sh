#!/bin/bash
#
# wildi.markus@bluewin.ch
#
# Helper script used by
#
# rts2-scriptexec -d C0 -s " exe ./u_acquire.sh "
#
# exe can not deal with arguments
#
# This is the production version.
#
cd $HOME/rts2/scripts/u_point/
source ./rts2_script/u_acquire_settings.sh
# close stderr
./u_acquire.py --base-path  $BASE_PATH $LATITUDE  $LONGITUDE --device-class DeviceRts2 $USE_BRIGHT_STARS $FETCH_DSS_IMAGES $SUN_SEPARATION $DO_QUICK_ANALYSIS $ACQUIRE_DS9_DISPLAY --level DEBUG
#

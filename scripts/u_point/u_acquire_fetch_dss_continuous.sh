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
#
#
# same with DSS image retrieval
./u_acquire.py --base-path  /tmp/u_point --device DeviceRts2 --mode-continues --fetch-dss-image --level DEBUG 


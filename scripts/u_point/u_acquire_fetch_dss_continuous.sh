#!/bin/bash
#
# wildi.markus@bluewin.ch
#
# Helper script used by
#
# rts2-scriptexec -d C0 -s " exe ./u_acquire_simulate_continuous.sh "
#
# exe can not deal with arguments
#
# ATTENTION: these settings are used for demonstration and debug
#            purposes ONLY.
# 
cd $HOME/rts2/scripts/u_point/
#
# fastest demonstration/DEBUG mode, no DSS images retrieval
#                         
#./u_acquire.py --level DEBUG --simulate-image --mode-continues  
#
# same with DSS image retrieval
./u_acquire.py --level DEBUG --mode-continues --fetch-dss-image

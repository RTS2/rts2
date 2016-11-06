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
#
./u_acquire.py --level DEBUG

#!/bin/bash
#
#
#
#
echo "check log file /var/log/rts2-debug.log"

rts2-scriptexec -d C0 -s ' exe ../rts2pa_measurement.py '

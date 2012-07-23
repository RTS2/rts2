#!/bin/bash
#
#
#
#
#
echo "check log file /tmp/rts2af-offline.log and /var/log/rts2-debug.log"
echo "In case mathplotlib does not display the results see /tmp/rts2af-fit-rts2af-X-*.png"
echo "Success, if the line:"
echo "rts2af_feed_acquire.py: from rts2af_acquire.py: log I rts2af_acquire: ----------------focus line: FOCUS: 17020.0, FWHM: 3.5, TEMPERATURE: 17.2C, OBJECTS: 88 DATAPOINTS: 20"
echo "appears"
echo
echo
echo
echo
echo
echo "stop this script with CTRL-C once rts2af_acquire.py has finished"
echo
echo
echo
#
rts2af_feed_acquire.py

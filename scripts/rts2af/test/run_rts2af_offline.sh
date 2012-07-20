#!/bin/bash
#
#
#
#
#
echo "check log file /tmp/rts2af-offline.log and /var/log/rts2-debug.log"
echo "In case mathplotlib does not display the results see /tmp/rts2af-fit-rts2af-X-*.png"
#
rts2af_offline.py  --config ./rts2af-offline.cfg --ref 20120324001509-875-RA-reference.fits
#
# fail due to to few focuser positions ("moving stars")
#
rts2af_offline.py  --config ./rts2af-offline.cfg --ref 20120714044002-154-RA-reference.fits

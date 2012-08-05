#!/bin/bash
#
#
#
#
#
echo "check log files /tmp/rts2af-offline.log and /var/log/rts2-debug.log"
echo "In case mathplotlib does not display the results see /tmp/rts2af-fit-rts2af-X-*.png"
#
rts2af_analysis.py --config ./rts2af-offline.cfg <<EOF
20120324001509-875-RA-reference.fits
./focus/2012-06-23T11:56:24.022192/X/20120324001749-374-RA.fits
./focus/2012-06-23T11:56:24.022192/X/20120324001528-966-RA.fits            
./focus/2012-06-23T11:56:24.022192/X/20120324001804-100-RA.fits
./focus/2012-06-23T11:56:24.022192/X/20120324001602-509-RA.fits            
./focus/2012-06-23T11:56:24.022192/X/20120324001820-993-RA.fits
./focus/2012-06-23T11:56:24.022192/X/20120324001625-517-RA.fits            
./focus/2012-06-23T11:56:24.022192/X/20120324001839-935-RA.fits
./focus/2012-06-23T11:56:24.022192/X/20120324001646-407-RA.fits            
./focus/2012-06-23T11:56:24.022192/X/20120324001900-940-RA.fits
./focus/2012-06-23T11:56:24.022192/X/20120324001705-200-RA.fits            
./focus/2012-06-23T11:56:24.022192/X/20120324001926-496-RA.fits
./focus/2012-06-23T11:56:24.022192/X/20120324001722-024-RA.fits            
./focus/2012-06-23T11:56:24.022192/X/20120324002007-746-RA-proof.fits
./focus/2012-06-23T11:56:24.022192/X/20120324001736-702-RA.fits
Q
EOF
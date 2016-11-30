#!/bin/bash
#
# 2016-11-30, wildi
#
# display and anaylze while acquiring
#

./u_analyze.py --base /tmp/u_point --plot  --animate --ds9-display --delete &

while :
do
    ./u_analyze.py --base /tmp/u_point  --toconsole
    echo "going on"
done

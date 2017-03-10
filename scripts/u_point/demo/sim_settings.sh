#!/bin/bash

BASE_PATH=/tmp/u_point
lat_lon_steps="--lat-step 10 --lon-step 20"
LATITUDE="--obs-latitude m75.1"
LONGITUDE="--obs-longitude 123.1"
SUN_SEPARATION="--sun-separation 40."
# latitude limits 
ALT_LOW=0.
ALT_HIGH=80.
AZ_LOW=0.
AZ_HIGH=360.
#AZ_LOW=130.
#AZ_HIGH=270.
# brightness limits
MAG_LOW=3.
MAG_HIGH=-5.
# do not fetch images
FETCH_DSS_IMAGE=
echo ""
echo "if no sun is visible, change LONGITUDE, current value $LONGITUDE"
echo ""

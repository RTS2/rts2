#!/bin/bash

BASE_PATH=/tmp/u_point
alt_az_steps="--alt-step 10 --az-step 20"
LATITUDE="--obs-latitude .1"
LONGITUDE="--obs-longitude 20.1"
SUN_SEPARATION="--sun-separation 45."
# altitude limits 
ALT_LOW=50.
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

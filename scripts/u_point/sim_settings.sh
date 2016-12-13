#!/bin/bash

BASE_PATH=/tmp/u_point
alt_az_steps="--alt-step 10 --az-step 40"
LATITUDE="--obs-latitude m75.1"
LONGITUDE="--obs-longitude 123.1"
SUN_SEPARATION="--sun-separation 40."
# altitude limits 
LOW=0.
HIGH=80.
AZ_LOW=0.
AZ_HIGH=360.
#AZ_LOW=130.
#AZ_HIGH=270.
# brightness limits
MAG_LOW=3.
MAG_HIGH=-5.
# do not fetch images
FETCH_DSS_IMAGE=

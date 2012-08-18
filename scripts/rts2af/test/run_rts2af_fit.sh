#!/bin/bash
#
# Test program for fitting
#
#
echo "In case mathplotlib does not display the results see /tmp/rts2af-2012-07-17T16:27:24.336448.png and /tmp/rts2af-fail.png"
#
#
rts2af_fit.py 1  UNK 12.11C 2012-07-09T11:22:33 13 ./rts2af-fit-autofocus-X-2012-07-17T16:27:24.336448-FWHM_IMAGE.dat ./rts2af-fit-autofocus-X-2012-07-17T16:27:24.336448-FLUX_MAX.dat /tmp/rts2af-2012-07-17T16:27:24.336448.png
#
# fit fails
#
rts2af_fit.py 1  UNK 12.11C 2012-07-09T11:22:33 13 ./test-fail-FWHM_IMAGE.dat ./test-fail-FLUX_MAX.dat /tmp/rts2af-fail.png
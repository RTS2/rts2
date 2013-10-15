RTS2 integration
================


RTS2 environment test
---------------------

To make sure that the rts2saf_focus.py is executable within RTS2 environment use: 

  | cd ~/rts-2/scripts/rts2saf/
  | rts2-scriptexec -d C0 -s ' exe ./rts2saf_focus.py '

and check the log file /tmp/rts2saf_focus.py.log in case the results are not
what you are expecting.

To see script imgp_analysis.py working use:

  | cd ~/rts-2/scripts/rts2saf/
  | ../imgp_analysis.py  samples/20071205025911-725-RA.fits

The output goes to /var/log/rts2-debug.

Integration of rts2saf
----------------------

In /etc/rts2/rts2.ini section
 | [imgproc]
 |  astrometry = "/your/path/to/imgp_analysis.py"

imgp_analysis.py calls rts2saf_fwhm.py which measures the FWHM of
each image after it has been stored on disk. If FWHM is above threshold it 
writes tar_id 5 into selector's focusing queue. Next executed target will 
be tar_id 5 that's 'OnTargetFocus'.

rts2-centrlad configuration
---------------------------
Restore the real /etc/rts2/devices file in case you wish them be present


rts2-selector configuration
---------------------------
Configure selector (SEL), replace the default in /etc/rts2/services with

  selector        SEL    --add-queue plan --add-queue focusing --add-queue manual

You might have additional queue names hence add them.

Command line execution, night time
----------------------------------
Execute 

  rts2saf_focus.py --toconsole --displayfit --displayds9

and after a while the matplotlib window opens and after closing
it the DS9 window.

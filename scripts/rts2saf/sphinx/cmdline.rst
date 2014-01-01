.. _sec_scripts-label:


Scripts
=======


rts2saf_focus.py
--------------------
.. program-output:: rts2saf_focus.py --help

Fetch the sample FITS files

.. code-block:: bash
 
 cd ~/rts-2/scripts/rts2saf
 wget http://azug.minpet.unibas.ch/~wildi/rts2saf-test-focus-2013-09-14.tgz
 tar zxvf rts2saf-test-focus-2013-09-14.tgz
 rts2saf_focus.py --toc --conf configs/dummy-bootes-2/rts2saf.cfg  --focde 0 --dry ./samples

You might specify ``--conf configs/one-filter-wheel/rts2saf.cfg`` or a different one depending on the file ``/etc/rts2/devices``. Output:

.. code-block:: bash

  create:  F0 setting internal limits from configuration file and ev. default values!
  create:  F0 setting internal limits for --blind :[-12, 15], step size: 1
  create:  COLWFLT, empty slot:open
  create:  COLWFLT, dropping empty slot:empty8
  create:  COLWGRS, empty slot:open
  create:  COLWSLT, empty slot:open
  rts2saf_focus: starting scan at: 2013-10-27T18:51:35.276180
  Focus: using dry FITS files
  acquire: focPosCalc:-14, focPos: 0, speed:1.0, sleep: 14.00 sec
  ...
  sextract: no FILTC name information found, /tmp/rts2saf-focus/2013-10-27T18:51:35.276180/COLWFLT/open/20071205025915-945-RA.fits
  Focus: pos:    16, objects:   87, file: /tmp/rts2saf-focus/2013-10-27T18:51:35.276180/COLWFLT/open/20071205025915-945-RA.fits
  Focus:  5374: weightmedMeanObjects, filter wheel:COLWFLT, filter:open
  Focus:  5388: weightedMeanFwhm,     filter wheel:COLWFLT, filter:open
  Focus:  5376: weightedMeanStdFwhm,  filter wheel:COLWFLT, filter:open
  Focus:  5391: weightedMeanCombined, filter wheel:COLWFLT, filter:open
  Focus:  5425: minFitPos,            filter wheel:COLWFLT, filter:open
  Focus:  2.24: minFitFwhm,           filter wheel:COLWFLT, filter:open
  Focus: filter: open not setting FOC_DEF (see configuration)


The above output is shows only a small fraction of the log output. 

rts2saf_fwhm.py
---------------
.. program-output:: rts2saf_fwhm.py --help

Fetch the sample FITS files

.. code-block:: bash
 
 cd ~/rts-2/scripts/rts2saf/imgp
 wget http://azug.minpet.unibas.ch/~wildi/20131011054939-621-RA.fits
 rts2saf_fwhm.py  --fitsFn 20131011054939-621-RA.fits --toc

Output:

.. code-block:: bash

 rts2af_fwhm: no focus run  queued, fwhm: 10.84 < 35.00 (threshold)
 rts2af_fwhm: DONE

The FWHM is  10.84 and well below threshold (see FWHM_LOWER_THRESH).

rts2saf_analyze.py
------------------
.. program-output:: rts2saf_analyze.py --help

Fetch the sample FITS files

.. code-block:: bash
 
 cd ~/rts-2/scripts/rts2saf
 wget http://azug.minpet.unibas.ch/~wildi/rts2saf-test-focus-2013-09-14.tgz
 tar zxvf rts2saf-test-focus-2013-09-14.tgz
 rts2saf_analyze.py --toc --deb --base ./samples

Output:

.. code-block:: bash

 extract: no FILTA name information found, ./samples/20071205025901-389-RA.fits
 analyze: processed  focPos:  5260, fits file: ./samples/20071205025901-389-RA.fits
 sextract: no FILTA name information found, ./samples/20071205025920-958-RA.fits
 ...
 analyze: FOC_DEF:  5363 : weighted mean derived from sextracted objects
 analyze: FOC_DEF:  5377 : weighted mean derived from FWHM
 analyze: FOC_DEF:  5367 : weighted mean derived from std(FWHM)
 analyze: FOC_DEF:  5382 : weighted mean derived from Combined
 analyze: FOC_DEF:  5436 : fitted minimum position,  2.2px FWHM, NoTemp ambient temperature
 analyzeRuns: ('NODATE', 'NOFTW') :: NOFT 14 
 rts2saf_analyze: no ambient temperature available in FITS files, no model fitted

The fit converged and the minimum FWHM is at  5436.

rts2saf_imgp.py
---------------
.. program-output:: rts2saf_imgp.py --help

Fetch this FITS with

.. code-block:: bash

 cd ~/rts-2/scripts/rts2saf/imgp
 wget http://azug.minpet.unibas.ch/~wildi/20131011054939-621-RA.fits
 rts2saf_imgp.py ./20131011054939-621-RA.fits --toc

Output:

.. code-block:: bash

 rts2saf_imgp.py: starting
 rts2saf_imgp.py, rts2-astrometry-std-fits.net: corrwerr 1 0.3624045465 39.3839441225 -0.0149071686 -0.0009854536 0.0115640672
 corrwerr 1 0.3624045465 39.3839441225 -0.0149071686 -0.0009854536 0.0115640672
 rts2saf_imgp.py, rts2-astrometry-std-fits.net: double real_ra "[hours] image ra as calculated from astrometry" 0.362404546535 
 rts2saf_imgp.py, rts2-astrometry-std-fits.net: double real_dec "[deg] image dec as calculated from astrometry" 39.3839441225 
 rts2saf_imgp.py, rts2-astrometry-std-fits.net: double tra "[hours] telescope ra" 0.347497377906 
 rts2saf_imgp.py, rts2-astrometry-std-fits.net: double tdec "[deg] telescope dec" 39.3829586689 
 rts2saf_imgp.py, rts2-astrometry-std-fits.net: double ora "[arcdeg] offsets ra ac calculated from astrometry" -0.0149071686287 
 rts2saf_imgp.py, rts2-astrometry-std-fits.net: double odec "[arcdeg] offsets dec as calculated from astrometry" -0.000985453568909 
 rts2saf_imgp.py, rts2-astrometry-std-fits.net: string object "astrometry object" kelt-1b
 rts2saf_imgp.py, rts2-astrometry-std-fits.net: integer img_num "last astrometry number" 1470
 rts2saf_imgp.py, rts2-astrometry-std-fits.net: 
 rts2saf_imgp.py: ending

``rts2saf_imgp.py`` is executed by ``IMGP`` and the line ``corrwerr...`` was written on ``stdout`` and is read back by it. The script doing astrometry is configurable (see ``SCRIPT_ASTROMETRY``).



rts2saf_reenable_exec.py
------------------------

``rts2saf_reenable_exec.py`` is executed by ``rts2saf_focus.py`` if the variable ``REENABLE_EXEC`` is set to

.. code-block:: bash

 [basic]
 REENABLE_EXEC=True

As of 2014-01-01 EXEC does not return to normal operations after a script, executed as ``' exe some_script '`` has finished. To reenable EXEC this script dis- and enables EXEC. This is the current default.

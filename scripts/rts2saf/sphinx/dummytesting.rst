Testing with dummy devices
==========================

Note: since RTS2 dummy devices behave like ordinary devices this chapter
applies to real devices as well. Keep on reading.

RTS2 provides dummy (software only) devices which can be used to together with
the services, like ``rts2-centrald``, to learn almost anything about operations
without the need to wait for a specific state change (e.g. E.g., if
one wants to start the observation right before state ``evening`` adjust the
parameter ``longitude`` to a decent value or if you really need a long  night
set ``latitude = +/- 80.0`` depending if it is winter/summer on the northern
hemisphere.


In the following description ``rts2saf.cfg`` from example ``configs/one-filter-wheel`` 
is used. This is a configuration with a CCD, a focuser and one filter wheel with 
bunch of filters.  In file ``/etc/rts2/devices`` add dummy devices (at least these entries)  	

.. code-block:: bash

 #device	type	device_name	options
 #
 camd	        dummy	C0	--wheeldev W0  --filter-offsets 1:2:3:4:5:6:7:8  --focdev F0 --width 400 --height 500 
 filterd	dummy	W0	-F "open:R:g:r:i:z:Y:empty8" -s 10 --localhost localhost
 focusd	        dummy	F0      --modefile /etc/rts2/f0.modefile 

or copy ``~/rts-2/scripts/rts2saf/configs/one-filter-wheel/devices`` to ``/etc/rts2``.
The ``rts2saf`` configuration needs to be copied:

.. code-block:: bash

   

   sudo cp ~/rts-2/scripts/rts2saf/configs/one-filter-wheel/rts2saf.cfg /usr/local/etc/rts2/rts2saf
   cd ~/rts-2/conf/rts2saf/
   sudo cp rts2saf.cfg rts2saf-sex.cfg rts2saf-sex.nnw rts2saf/f0.modefile /usr/local/etc/rts2/rts2saf

The focuser configuration file ``f0.modefile`` sets ``focstep`` of the dummy focuser to
a reasonable value which shortens execution time.

After exposure ``rts2-camd-dummy`` provides ordinary fits files with random numbers
as content. In order to test the whole analysis chain download the 'dry FITS files', with

.. code-block:: bash

 cd ~/rts-2/scripts/rts2saf
 wget http://azug.minpet.unibas.ch/~wildi/rts2saf-test-focus-2013-09-14.tgz
 tar zxvf rts2saf-test-focus-2013-09-14.tgz

All scripts, with the exception of ``rts2saf_analyze.py``, try to log to ``/var/log/rts2-debug`` and if 
they fail to ``/tmp/rts2saf_log/rts2-debug`` if no option (``--topath`` or ``--logfile``) is specified. 
``rts2saf_analyze.py`` logs by default to the current directory.
If the scripts are executed on the command line logging to console is enabled with option 
``--toconsole`` and if more detailed output is required enable option ``--level DEBUG`` together with 
``--debug`` or ``--verbose``  if available. 

Now start RTS2 with ``/etc/init.d/rts2 start`` and check if all devices and services appeared 
(e.g. with ``rts2-mon``).  Check if the ``rts2saf`` configuration matches RTS2: 

.. code-block:: bash

  rts2saf_focus.py --toconsole --check

and if no errors are reported execute 

.. code-block:: bash

  rts2saf_focus.py --toconsole

Alternatively watch in a new console the log file with

.. code-block:: bash

  tail -f /tmp/rts2saf_log/rts2-debug

In order to get real results inject the 'dry FITS files' into the image acquisition, use
the following commands

.. code-block:: bash

  cd ~/rts-2/scripts/rts2saf
  rts2saf_focus.py --toconsole --dryfitsfiles  ./samples/  --exp 1.  --fitdisplay --ds9display

After a while a matplotlib window appears containing the fit. After closing it 
a ``DS9`` window appears showing which stars have been selected (green) for a given image.
The above command honors limits defined in ``rts2saf.cfg`` section ``[filter properties]``

.. code-block:: bash

 flt1 = [ R, -10, 10, 1, 11.1]
 flt2 = [ g, -12, 13, 2, 1.]
 flt3 = [ r, -12, 13, 2, 1.]
 flt4 = [ i, -14, 15, 3, 1.]
 flt5 = [ z, -14, 15, 3, 1.]
 flt6 = [ Y, -14, 15, 3, 1.]
 flt7 = [ empty8, -14, 15, 3, 1.]
 flt8 = [ open, -14, 15, 3, 1.]

That implies that the dummy focuser travels e.g. for filter ``open`` from tick -14 to 15 in steps of 3. The last
number is a multiplier for the parameter ``BASE_EXPOSURE``. In order to make this filter active specify it
in section

.. code-block:: bash

 [filter wheel]
 fltw1 = [ W0, open ]

Add more entries from the filter properties list in case you want measure the filter offsets in
respect to the empty slot:

.. code-block:: bash

 [filter wheel]
 fltw1 = [ W0, open, R, g, r, i, z, Y ]

The range that the focuser should travel is highly dependent on the 
optics. As rule of thumb: if the FWHM minimum is 6 pixel wide then choose
the limits of the range so that the FWHM does not exceed 18 pixel intra- and
extra focal.

Finally define which filter wheel is used

.. code-block:: bash

 [filter wheels]
 inuse = [ W0 ]
 EMPTY_SLOT_NAMES = [ open, empty8 ]

Since it is difficult to retrieve valid information about the filter wheel slots specify which of the
names denote empty slots as arguments of ``EMPTY_SLOT_NAMES``.

If the the focus position is not known one can scan the whole available focuser range
whith the option ``--blind``

.. code-block:: bash

  rts2saf_focus.py  --toconsole --dryfitsfiles  ./samples/ --exp 1. --blind

This command honors the following entries

.. code-block:: bash

 FOCUSER_STEP_SIZE = 1
 FOCUSER_LOWER_LIMIT = -12
 FOCUSER_UPPER_LIMIT = 15

which means that the focuser travels from -12 to 15 in steps of 1 tick taking 28 images.
Set the absolute limits

.. code-block:: bash

 FOCUSER_ABSOLUTE_LOWER_LIMIT = -16
 FOCUSER_ABSOLUTE_UPPER_LIMIT = 19

so that the sum of ``FOC_DEF`` and eventual filter offsets does not exceed either lower or upper limits of the real focuser. 
If a focuser can travel within [0,7000] as e.g. the FLI PDF, appropriate values
are

.. code-block:: bash

 FOCUSER_LOWER_LIMIT = 1000
 FOCUSER_UPPER_LIMIT = 5500
 FOCUSER_STEP_SIZE   = 500

If unsure set them to the hardware limits.  Again specify which filters are used

.. code-block:: bash

 [filter wheel]
 fltw1 = [ W0, open ]

In blind mode it is recommended to measure only one empty slot.

Normally the fit convergences but it does often not represent the minimum in ``--blind`` mode. Therefore
an estimator based on the weighted mean is the best guess. These
values appear as 

.. code-block:: bash

 analyze: FOC_DEF:   258: weighted mean derived from sextracted objects
 analyze: FOC_DEF:   286: weighted mean derived from FWHM
 analyze: FOC_DEF:   305: weighted mean derived from std(FWHM)
 analyze: FOC_DEF:   342: weighted mean derived from Combined

on the console. Under normal circumstances the ``weighted mean derived from Combined``
is the closest approximation of the true value.

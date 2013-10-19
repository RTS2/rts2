
rts2saf Bootes-2 configuration with comments
============================================


.. code-block:: bash

 [basic]
 EMPTY_SLOT_NAMES = [ empty8, open ]

Bootes-2 has three filter wheels which have empty slots named ``empty8`` and ``open``.

.. code-block:: bash

 # keep novaraketa tidy
 BASE_DIRECTORY = /home/wildi/rts2saf-focus

This is the path where the images are stored.

.. code-block:: bash

 TEMP_DIRECTORY = /home/wildi/tmp

Do not clutter system directories.

.. code-block:: bash

 [ccd]
 CCD_NAME = andor3
 BASE_EXPOSURE= 5.
 ENABLE_JSON_WORKAROUND = True

Expose empty slots for 5 seconds, JSON interface creates files with identical names, we skip that.

.. code-block:: bash

 [mode] 
 SET_FOC_DEF = True
 WRITE_FILTER_OFFSETS = False

After a successful fit on an empty slot FOC_DEF will be set but not filter offsets

.. code-block:: bash

 [filter properties]
 flt1 = [ R,      -60, 60, 15, 3.]
 flt2 = [ g,      -60, 60, 15, 3.]
 flt3 = [ r,      -60, 60, 15, 3.]
 flt4 = [ i,      -60, 60, 15, 6.]
 flt5 = [ z,      -60, 60, 15, 6.]
 flt6 = [ Y,      -60, 60, 15, 6.]
 flt7 = [ empty8, -60, 60, 15, 1.]
 flt8 = [ open,   -60, 60, 15, 1.]

The star images degrade quickly and we do not want too many pictures. ``FOC_TOFF`` is varied by p/m 60
in steps of 15 tick. The last number defines the exposure factor and in case of filter flt5 the
exposure is 30 seconds (``BASE_EXPOSURE`` * 6.)

.. code-block:: bash


 [filter wheel]
 fltw1 = [ COLWFLT, open, R, g, r, i, z, Y, empty8 ]
 

The filter wheel named COLWFLT contains filters with the names ``open``, ``R``, ``g``, ``r``, ``i``, ``z``, ``Y``, ``empty8``. Slots ``open`` and ``empty8`` have no glass (no focus offset). This example is best used on the command line to define the filter offsets.

.. code-block:: bash

 fltw1 = [ COLWFLT, open]
 fltw2 = [ COLWGRS, open]
 fltw3 = [ COLWSLT, open]

There are three filter wheels ``COLWFLT``, ``COLWGRS`` and ``COLWSLT``. ``COLWGRS`` and ``COLWSLT`` have no 'real' filters, that means no filter offsets have to be measured. This is an example for the autonomous operations: to get the focus faster only ``open`` slots are measured assuming filter offsets have been defined earlier. 

.. code-block:: bash

 [filter wheels]
 inuse = [COLWFLT,COLWGRS,COLWSLT]
 EMPTY_SLOT_NAMES = [ open, empty8 ]

All filter wheels are ``inuse`` in order that the other two (``COLWGRS``, ``COLWSLT``) can be set to ``open``. If no filter offsets are known they need to be defined by ``EMPTY_SLOT_NAMES``. These filters  names are used on all filter wheels.

.. code-block:: bash

 [focuser properties]
 FOCUSER_NAME = F0
 FOCUSER_RESOLUTION = 3.

The focuser resolution is defined as focuser tick difference to make a difference of 1 pixel in FWHM.

.. code-block:: bash

 FOCUSER_SPEED = 100.0

The acquistion routine has to wait for a period of time until the focuser reaches target position.  

.. code-block:: bash

 FOCUSER_TEMPERATURE_COMPENSATION = False

Only driver ``flitc.cpp`` has that.

.. code-block:: bash

 FOCUSER_ABSOLUTE_LOWER_LIMIT = 100
 FOCUSER_ABSOLUTE_UPPER_LIMIT = 2000

If he focuser can travel between e.g. between -500 to 5000 tick the above entries limit it to 100, 2000.

.. code-block:: bash

 FOCUSER_LOWER_LIMIT = 100
 FOCUSER_UPPER_LIMIT = 700

These limits are used in case a ``--blind`` focus run is carried out.

.. code-block:: bash

 FOCUSER_STEP_SIZE = 100

In case a ``--blind`` focus run is carried out the interval (``FOCUSER_LOWER_LIMIT``, ``FOCUSER_UPPER_LIMIT``) is stepped in ``FOCUSER_STEP_SIZE`` [tick].  

.. code-block:: bash

 [SExtractor]
 SEXPATH = /home/wildi/downloads/sextractor-2.8.6/src/sex
 SEXCFG = /usr/local/etc/rts2/rts2saf/rts2saf-sex.cfg
 STARNNW_NAME = /home/wildi/downloads/sextractor-2.8.6/config/default.nnw
 #
 [fits header mapping]
 AMBIENTTEMPERATURE = DAVIS.DOME_TMP

In case your FITS header key words differ they are remapped (contact the author!).

.. code-block:: bash

 [queue focus run]
 FWHM_LOWER_THRESH = 9.

Do not trigger a focus run if FWHM is below thresh (rts2saf_fwhm.py uses that).

.. code-block:: bash

 [IMGP analysis]
 FILTERS_TO_EXCLUDE = [ FILTC:grism1, FILTB:closed, FILTB:slit1, FILTB:slit2, FILTB:hartmann, FILTB:pinhole ]

Do not try to determine for these filter wheels and filters a FWHM or an astrometrical calibration .

.. code-block:: bash

 SCRIPT_ASTROMETRY = rts2-astrometry.net
 #SCRIPT_ASTROMETRY = rts2-astrometry-std-fits.net                                                                                                                                                      
 # this is the old version in use at Bootes-2
 #SCRIPT_ASTROMETRY = /etc/rts2/img_process

Choose the script that fulfills your purpose.

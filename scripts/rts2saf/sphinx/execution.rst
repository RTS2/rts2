Manual Execution
================

All scripts have an :ref:`on line help <sec_scripts-label>` and all arguments 
have a decent default value which enables them to run in autonomous mode where 
appropriate. 


There are several ways to carry out a focus run "right now".
For an immediate run start ``rts2-mon`` and bring RTS2 to state ``ON`` (press  ``F4``)
then  move to tab EXEC and check or change to

.. code-block:: bash

 ignore_day        true
 enabled           true
 selector_next     false
 auto_loop         false
 default_auto_loop false

To execute target ``tar_id=5`` type ``now 5`` and watch the log file

.. code-block:: bash

  tail -f /var/log/rts2-debug 

To queue a focus run through ``rts2-selector`` (SEL)  (re-)enable SEL and EXEC 
use either

.. code-block:: bash

 rts2-queue --queue focusing OnTargetFocus

or

.. code-block:: bash

 rts2saf_fwhm.py --fitsFn FITS_FILE --toc

on a recently acquired defocused fits file (FITS_FILE).  
While the observatory is operational (night time) and target selection is disabled, execute

.. code-block:: bash

  rts2saf_focus.py --toconsole --fitdisplay --ds9display --debug --level DEBUG

and after a while a ``matplotlib`` window appears with fitted data and after closing it the 
``DS9`` window appears displaying the accepted (green) and rejected (red) sources.


During acquisition in autonomous operations, ``rts2saf_focus.py`` is being executed by EXEC 
in the background, no plots or images are displayed. To get an idea how an ongoing focus run 
looks like use

.. code-block:: bash

 rts2saf_analyze.py --toconsole --fitdisplay --ds9display --basepath  BASE_DIRECTORY/DATE 

where ``BASE_DIRECTORY`` refers to the variable in the configuration file and ``DATE`` to 
the start time (normally it is sufficient to do a ``ls -lrt`` within ``BASE_DIRECTORY``
and change to the last listed directory). Acquisition and analysis processes do not interfere 
at all.


The measurement of the filter offsets  is done on the command line and the results are manually written to file ``/etc/rts2/devices``:

.. code-block:: bash

 camd     fli    CCD_FLI     --focdev FOC_FLI --wheeldev FTW_FLI --filter-offsets 1644:1472:1346:1349:1267:0:701
 filterd  fli    FTW_FLI     -F "U:B:V:R:I:X:H"

The focus travel range is defined by the values given in section ``[filter properties]``
as explained in next section.

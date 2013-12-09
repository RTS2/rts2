RTS2 integration
================


In ``/etc/rts2/rts2.ini`` section

.. code-block:: bash

 [imgproc]
 astrometry = "/usr/local/bin/rts2saf_imgp.py"

``rts2saf_imgp.py`` calls ``rts2saf_fwhm.py`` which measures the FWHM of
each image after it has been stored on disk. If FWHM is above threshold it 
writes ``tar_id 5`` into selector's ``focusing`` queue. Next executed target will 
be ``tar_id 5`` that's ``OnTargetFocus``.

rts2-centrald configuration
---------------------------
Restore the real ``/etc/rts2/devices`` file in case you wish them to be present


rts2-selector configuration
---------------------------
Configure ``selector`` (SEL), replace the default in ``/etc/rts2/services`` with

.. code-block:: bash

  selector        SEL    --add-queue plan --add-queue focusing --add-queue manual

You might have additional queue names hence add them.

RTS2 environment test
---------------------

To make sure that the ``rts2saf_focus.py`` is executable within RTS2 environment use: 

.. code-block:: bash

      cd ~/rts-2/scripts/rts2saf/
      rts2-scriptexec -d C0 -s ' exe ./rts2saf_focus.py '

and check the log file ``rts2saf_focus.py.log`` in case the results are not
what you are expecting.

To see script ``rts2saf_imgp.py`` working use:

.. code-block:: bash

  cd ~/rts-2/scripts/rts2saf/
  ./rts2saf_imgp.py  imgp/20131011054939-621-RA.fits --toc

The output goes to ``/var/log/rts2-debug``.

Monitoring
----------

During acquisition, ``rts2saf_focus.py`` is being executed by EXEC in the background, 
no plots or images are displayed. To get an idea how an ongoing focus run looks like use

.. code-block:: bash

 rts2saf_analyze.py --toconsole --fitdisplay --ds9display --basepath  BASE_DIRECTORY/DATE 

where ``BASE_DIRECTORY`` refers to the configuration file and ``DATE`` to the start time.
The processes do not interfere at all.

Command line execution, night time
----------------------------------
Execute 

.. code-block:: bash

  rts2saf_focus.py --toconsole --fitdisplay --ds9display

and after a while a ``matplotlib`` window appears with data and the fit and after closing
it the ``DS9`` window appears.

RTS2 EXEC manual start
----------------------

Start ``rts2-mon`` and bring RTS2 to state ``ON``, press 

.. code-block:: bash

 F4

Then  move to tab EXEC and check or change to

.. code-block:: bash

 ignore_day        true
 enabled           true
 selector_next     false
 auto_loop         false
 default_auto_loop false

To execute target ``tar_id=5`` now type

.. code-block:: bash

  now 5

and watch the log files

.. code-block:: bash

  tail -f /var/log/rts2-debug ./rts2saf_focus.py.log

RTS2 manual queuing
-------------------

To queue a focus run through ``rts2-selector`` (SEL) use

.. code-block:: bash

 rts2-queue --queue focusing OnTargetFocus

(Re-)enable SEL and EXEC, or use 


.. code-block:: bash

 rts2saf_fwhm.py --fitsFn FITS_FILE --toc

on a recently acquired fits file (FITS_FILE).  

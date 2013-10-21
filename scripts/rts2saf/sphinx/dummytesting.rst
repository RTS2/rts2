Testing with dummy devices
==========================


All scripts have their own log file which is written by default to ``.`` or ``/tmp``. 
The log file name is 'script_name.py.log', e.g. ``rts2saf_focus.py.log``. While
executing the scripts directly on the command line enable logging to terminal with
option ``--toconsole`` and if more detailed output is required enable ``--debug``
or ``--verbose`` if available. 

In the following description ``rts2saf.cfg`` from example ``configs/one-filter-wheel`` 
is used.


RTS2 configuration
------------------

In file ``/etc/rts2/devices`` add dummy devices (at least these entries)  	

.. code-block:: bash

 #device	type	device_name	options
 #
 camd	        dummy	C0	--wheeldev W0  --filter-offsets 1:2:3:4:5:6:7:8  --focdev F0 --width 400 --height 500 
 filterd	dummy	W0	-F "open:R:g:r:i:z:Y:empty8" -s 10 --localhost localhost
 focusd	        dummy	F0      

or copy ``~/rts-2/scripts/rts2saf/configs/one-filter-wheel/devices`` to ``/etc/rts2``

This is a configuration with a CCD, one filter wheel and bunch of filters. 

Command line execution, day time testing with 'dry fits files'
--------------------------------------------------------------

Start RTS2 

.. code-block:: bash

 /etc/init.d/rts2 start

and check if all devices appeared. Fetch the sample focus run files, these
are the 'dry fits files', with

.. code-block:: bash

 cd ~/rts-2/scripts/rts2saf
 wget http://azug.minpet.unibas.ch/~wildi/rts2saf-test-focus-2013-09-14.tgz
 tar zxvf rts2saf-test-focus-2013-09-14.tgz

Check first if the configured devices are present

.. code-block:: bash

  ./rts2saf_focus.py --toconsole --check

and if no errors are reported

.. code-block:: bash

  ./rts2saf_focus.py --toconsole --dryfitsfiles  ./samples/  --exp 1.

A lot of messages appear on the terminal...

If you want to see the fitted minimum and then selected objects:

.. code-block:: bash

  ./rts2saf_focus.py --toconsole --dryfitsfiles  ./samples/ --exp 1. --displayfit --displayds9

or

.. code-block:: bash

   ./rts2saf_analyze.py --toconsole --basepath ./samples/ --displayfit --displayds9

After a while a matplotlib window appears containing the fit. After closing it 
a DS9 window appears showing which stars have been selected for a given image.
The latter example does only carry out the analysis omitting acquisition and is
therefore faster.

Command line execution, day time testing
----------------------------------------

Execute 

.. code-block:: bash

  rts2saf_focus.py 

and change the terminal and watch the log file

.. code-block:: bash

  tail -f /tmp/rts2saf_focus.py.log

The dummy CCD provides only "noisy" FITS files and no analysis
is carried out.

.. _sec_unittest-label:

Testing individual components
=============================

``unittest``
-----------
The ``unittest`` rely on the presence of the files in directories ``samples`` and ``imgp``. Download the archives

.. code-block:: bash

 cd ~/rts-2/scripts/rts2saf
 wget http://azug.minpet.unibas.ch/~wildi/rts2saf-test-focus-2013-09-14.tgz
 tar zxvf rts2saf-test-focus-2013-09-14.tgz
 cd imgp
 wget http://azug.minpet.unibas.ch/~wildi/20131011054939-621-RA.fits

To verify the inistallation use

.. code-block:: bash

 cd ~/rts-2/scripts/rts2saf
 ./rts2saf_unittest.sh

Every test should be ``ok``. From time to time tests using JSON/proxy may fail. Please repeat the test once more. If the failure persists contact the author. 

To run a complete focus run within an RTS2 environment created on the fly change to 

.. code-block:: bash

 cd ~/rts-2/scripts/rts2saf/unittest

and edit in file ``test_focus.py`` the line from

.. code-block:: python 
 
 @unittest.skip('this unittest performs a complete focus run, by default it is disabled')

to

.. code-block:: python 

 #@unittest.skip('this unittest performs a complete focus run, by default it is disabled')

then run it with

.. code-block:: bash

 python test_focus.py

and make a 

.. code-block:: bash

 tail -f /tmp/unittest.log

to watch the output. RTS2 devices (CCD, focuser, filter wheels) are created as specified in ``./rts2saf-bootes-2.cfg`` in the same directory.

If you create this documentation

.. code-block:: bash

 cd ~/rts-2/scripts/rts2saf/sphinx
 make -f Makefile.sphinx  html
 
the commands ``rts2saf_unittest.sh`` and ``rts2saf_coverage.sh`` within ``unittest.rst`` (source of this page) produce the below presented outputs. The output of the tests go to file  ``/tmp/unittest.log``.

.. |date| date::
.. |time| date:: %H:%M:%S

``rts2saf unittest`` were executed on |date| at |time| on

.. program-output:: uname -a

through

.. code-block:: bash

  rts2saf_unittest.sh

.. program-output:: ../rts2saf_unittest.sh ../unittest


``coverage``
------------

You need to install ``coverage`` to see how the tests cover the code.

.. code-block:: bash

  rts2saf_coverage.sh

.. program-output:: ../rts2saf_coverage.sh ../unittest


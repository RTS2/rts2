Testing individual components ``unittest``, ``coverage``
========================================================
The ``unittest`` rely on the presence of the files in directories ``samples`` and ``imgp``. Download the archives

.. code-block:: bash

 cd ~/rts-2/scripts/rts2saf
 wget http://azug.minpet.unibas.ch/~wildi/rts2saf-test-focus-2013-09-14.tgz
 tar zxvf rts2saf-test-focus-2013-09-14.tgz
 cd imgp
 wget http://azug.minpet.unibas.ch/~wildi/20131011054939-621-RA.fits

If you execute 

.. code-block:: bash

 cd ~/rts-2/scripts/rts2saf/sphinx
 make -f Makefile.sphinx  html
 
to create this documatation the command ``rts2saf_unittest.sh`` and ``rts2saf_coverage.sh`` within ``unittest.rst`` (source of this page) produces the below presented output.

.. |date| date::
.. |time| date:: %H:%M:%S

Python ``unittest`` were executed on |date| at |time| on

.. program-output:: uname -a


through

.. code-block:: bash

  rts2saf_unittest.sh

.. program-output:: ../rts2saf_unittest.sh ../unittest

Output of 

.. code-block:: bash

  rts2saf_coverage.sh

.. program-output:: ../rts2saf_coverage.sh ../unittest


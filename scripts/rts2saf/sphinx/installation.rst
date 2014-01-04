.. _sec_installation-label:

Installation
============

Prerequisites
-------------

The :ref:`workaround for EXEC requires RTS2 at revision <sec_introduction-label>` ``r11758`` or later.

For the following description I assume you did

.. code-block:: bash

  cd ~
  svn co https://rts-2.svn.sf.net/svnroot/rts-2/trunk/rts-2 rts-2

followed by a full RTS2 installation including ``Postgres`` and setup of the RTS2 dummy devices. 


1) ``DS9`` from http://hea-www.harvard.edu/RD/ds9/site/Home.html
2) `SExtractor`` (if installation from source is required, include cblas, cblas-devel, libatlas3, libatlas3-devel,
   libatlas3-sse3m, libatlas3-sse3-devel, libatlas3-sse-common-devel) 


Update to Python 2.7.x (mandatory) and various Python packages:

3) ``SciPy``
4) ``matplotlib``
5) ``pyds9`` from the same site
6) ``numpy``, ``numpy-devel``
7) ``psutil``
8) ``astropy``
9) ``astrometry.net``
10) ``python-pytest`` 

For the documentation install ``sphinx`` and

10) ``sphinxcontrib-programoutput``

Recommended but not necessary install

7) ``coverage`` from https://pypi.python.org/pypi/coverage
8) ``python-pytest-cov`` from https://pypi.python.org/pypi/pytest-cov

During RTS2 installation the rts2saf executable are installed to 

.. code-block:: bash

  /usr/local/bin 

and the modules to

.. code-block:: bash

  /usr/local/lib/python2.7/dist-packages/rts2saf/

In case you modify a rts2saf module issue

.. code-block:: bash

 cd ~/rts-2/scripts
 sudo make install

``rts2saf unittest`` 
--------------------

Not yet complete but 

.. code-block:: bash

 cd ~/rts-2/scripts/rts2saf
 ./rts2saf_unittest.sh


may discover the most common installation problems. Before executing them edit in both files 
``unittest/rts2saf-bootes-2.cfg`` and ``unittest/rts2saf-no-filter-wheel.cfg`` the lines

.. code-block:: bash

 [SExtractor]
 SEXPATH = /usr/local/bin/sex
 SEXCFG = /usr/local/etc/rts2/rts2saf/rts2saf-sex.cfg

according to your ``SExtractor`` installation. If not  all tests are ``ok`` please 
send the output together with the log file ``/tmp/unittest.log`` to the author. The execution of a complete focus 
run within a real RTS2 environment created and distroyed on the fly is explained in 
:ref:`Testing individual components <sec_unittest-label>`.


RTS2 configuration and log file
-------------------------------

In section ``[xmlrpcd]`` you must add at least

.. code-block:: bash

  [xmlrpcd]
  images_name = "%f"

to get a unique FITS file name. If the files should have a different path add, e.g.

.. code-block:: bash

  images_path = "/images/b2/xmlrpcd/%N"

Further edit in section ``[observatory]`` ``altitude``, ``longitude`` and ``latitude``. 
To acquaint oneself with rts2saf use RTS2 dummy devices. Save  ``/etc/rts2/devices`` and replace it with
 
.. code-block:: bash

 cd /etc/rts2/
 mv devices devices.save
 ln -s ~/rts-2/scripts/rts2saf/configs/one-filter-wheel/devices  # you might specify full path

.. code-block:: bash
 
 sudo cp ~/rts-2/conf/rts2.ini  /etc/rts2
 sudo chown UID.GID /etc/rts2/rts2.ini  # insert your UID and GID for convenience
 sudo touch /var/log/rts2-debug
 sudo chown GID /var/log/rts2-debug # insert your GID
 sudo chmod g+w  /var/log/rts2-debug

If you want to run unit tests setting GID on file /var/log/rts2-debug
is mandatory.


rts2saf configuration files
---------------------------
rts2saf needs three configuration files to be present in ``/usr/local/etc/rts2/rts2saf``:

1) ``rts2saf.cfg``
2) ``rts2saf-sex.cfg``
3) ``rts2saf-sex.nnw``

.. code-block:: bash

 cd ~/rts-2/conf/
 sudo mkdir -p /usr/local/etc/rts2/
 sudo cp -a rts2saf /usr/local/etc/rts2/


Edit ``/usr/local/etc/rts2/rts2saf/rts2saf.cfg``  and check if  ``SExtractor`` binary is found.

``rts2saf.cfg`` is used by rts2saf and ``rts2saf-sex.cfg`` by ``SExtractor``. A usable example for the latter is stored in ``~/rts-2/conf/rts2saf``. In directory ``~/rts-2/scripts/rts2saf/configs``

.. code-block:: bash

  dummy-bootes-2
    devices
    rts2saf.cfg
  dummy-bootes-2-autonomous
    devices
    rts2saf.cfg
  no-filter-wheel
    devices
    rts2saf.cfg
  one-filter-wheel
    devices
    rts2saf.cfg
  one-filter-wheel-autonomous
    devices
    rts2saf.cfg

are four sets of rts2saf example configuration files with their
associated device files. The postfix ``-autonomous`` denotes configurations
which are used while rts2saf is integrated in RTS2.


Postgres DB tables targets and scripts entries
----------------------------------------------
The dummy devices are usually added  by the script 
``rts-2/scripts/ubuntu-rts2-install`` to the Postgres DB, in case not execute as user postgres:

.. code-block:: bash

  cd ~/rts-2/src/sql
  ./rts2-configdb stars -t T0
  ./rts2-configdb stars -c C0
  ./rts2-configdb stars -f W0

The filters are stored in the Postgres DB table ``filters``. These entries are not strictly necessary 
but it is recommended to add them.

As user postgres:

.. code-block:: bash

 psql stars  
 insert into targets values ('5', 'o', 'OnTargetFocus', null, null, 'this target does not change the RA/DEC values', 't', '1');
 insert into scripts values ('5', 'YOUR_CAMERA_NAME', ' exe /usr/local/bin/rts2saf_focus.py ');


Pitfalls: cfitsio
-----------------

Use the following commands to install cfitsio

.. code-block:: bash

 cd ~/cfitsio
 ./configure --prefix=/usr/local
 sudo make install
 sudo ldconfig 

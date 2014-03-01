.. _sec_installation-label:

Installation
============

Prerequisites
-------------

The :ref:`workaround for EXEC requires RTS2 at revision <sec_introduction-label>` ``r11758`` or later.

For the following description I assume you did

.. code-block:: bash

  cd ~
  svn co https://svn.code.sf.net/p/rts-2/code/trunk/  rts-2
  cd rts-2
  ./autogen.sh
  ./configure ...
  make && sudo make install


followed by installation and configuration of the ``Postgres`` DB including the setup of the RTS2 dummy devices (see  ``~/rts-2/scripts/ubuntu-rts2-install``). 


1) ``DS9`` from http://ds9.si.edu/site/Download.html
2) ``SExtractor`` (if installation from source is required, include cblas, cblas-devel, libatlas3, libatlas3-devel,
   libatlas3-sse3m, libatlas3-sse3-devel, libatlas3-sse-common-devel) 


Update to Python 2.7.x (mandatory) and various Python packages (use ``pip`` or ``easy_install`` from your distro, or follow the installation package instructions):

3) ``SciPy`` from your distro
4) ``matplotlib`` from your distro
5) ``pyds9`` form http://ds9.si.edu/download/pyds9/pyds9-1.7.tar.gz, for latest version check http://ds9.si.edu/site/Download.html
6) ``numpy``, ``numpy-devel`` from your distro
7) ``psutil`` from your distro or https://pypi.python.org/pypi?:action=display&name=psutil#downloads
8) ``astropy`` see http://docs.astropy.org/en/stable/install.html
9) ``astrometry.net`` http://astrometry.net/use.html
10) ``pyfits`` version >= 3.2,  see http://www.stsci.edu/institute/software_hardware/pyfits/Download
11) ``python-pytest`` from your distro

For the documentation install ``sphinx`` and

12) ``sphinxcontrib-programoutput`` with ``pip install sphinxcontrib-programoutput``


Recommended but not necessary install

13) ``coverage`` from https://pypi.python.org/pypi/coverage
14) ``python-pytest-cov`` from https://pypi.python.org/pypi/pytest-cov


``rts2saf unittest`` 
--------------------

Not yet complete but 

.. code-block:: bash

 cd ~/rts-2/scripts/rts2saf
 ./rts2saf_unittest.sh

may discover the most common installation problems. Before executing, install configure ``rts2saf`` and then check if
``SExtractor`` version >= 2.8.6 is available as

.. code-block:: bash

 /usr/local/bin/sex

If not  all tests are ``ok`` please  send the output together with the contents of 
``/tmp/rts2saf_log/`` to the author. The execution of a complete focus 
run within a real RTS2 environment created and distroyed on the fly is explained in 
:ref:`Testing individual components <sec_unittest-label>`.


RTS2 configuration
------------------

Changing in ``/etc/rts2/rts2.ini`` section 

.. code-block:: bash

 [imgproc]
 astrometry = "/usr/local/bin/rts2saf_imgp.py"

will activate ``rts2saf`` during autonomous operations.
``rts2saf_imgp.py`` calls ``rts2saf_fwhm.py`` which measures the FWHM of
each image after it has been stored on disk. If FWHM is above threshold it 
writes ``tar_id 5`` into selector's ``focusing`` queue. Next executed target will 
be ``tar_id 5`` that's ``OnTargetFocus``.
Add at least in section 

.. code-block:: bash

  [xmlrpcd]
  auth_localhost = false
  images_name = "%f"

to get a unique FITS file name. If the files should have a different path add, e.g.

.. code-block:: bash

  images_path = "/images/b2/xmlrpcd/%N"


Configure ``selector`` (SEL), replace the default in ``/etc/rts2/services`` with

.. code-block:: bash

  selector        SEL    --add-queue plan --add-queue focusing --add-queue manual

You might have additional queue names hence add them.



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
In directory ``~/rts-2/scripts/rts2saf/configs``

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

are four sets of example configuration files with their associated device files. 
The postfix ``-autonomous`` denotes configurations which are used while rts2saf 
is integrated in RTS2.


Postgres DB
-----------
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

where ``YOUR_CAMERA_NAME`` is either ``C0`` or any other chosen name. 

.. code-block:: bash

 createuser  uid  # where  uid is the user name which executes the unittest
 psql stars  
 GRANT ALL PRIVILEGES ON cameras TO  uid # the above uid

and very likely

.. code-block:: bash

 GRANT ALL ON TABLE targets to uid ;
 GRANT ALL ON TABLE scripts to uid ;

.. _sec_installation-label:

Installation
============

Prerequisites
-------------

The :ref:`workaround for EXEC requires RTS2 at revision <sec_introduction-label>` ``r11893`` or later.

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
2) ``SExtractor`` version >= **2.8.6** (if installation from source is required, include cblas, cblas-devel, libatlas3, libatlas3-devel,
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
12) ``python-psycopg2`` from your distro

For the documentation install ``sphinx`` and

13) ``sphinxcontrib-programoutput`` with ``pip install sphinxcontrib-programoutput``


Recommended but not necessary install

14) ``coverage`` from https://pypi.python.org/pypi/coverage
15) ``python-pytest-cov`` from https://pypi.python.org/pypi/pytest-cov


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
``rts-2/scripts/ubuntu-rts2-install`` to the Postgres DB. In case not or your device names ``T0`` and ``C0`` differ, execute as user postgres:

.. code-block:: bash

  cd ~/rts-2/src/sql
  ./rts2-configdb stars -t T0 
  ./rts2-configdb stars -c C0
  ./rts2-configdb stars -c andor # used only for unittest, see below

The filters are stored in the Postgres DB table ``filters``. These entries are not strictly necessary 
but it is recommended to add them.

As user postgres:

.. code-block:: bash

 postgres@localhost:~$ psql stars  

 INSERT INTO targets values ('5', 'o', 'OnTargetFocus', null, null, 'this target does not change the RA/DEC values', 't', '1');
 INSERT INTO scripts values ('5', 'YOUR_CAMERA_NAME', ' exe /usr/local/bin/rts2saf_focus.py E 1 ');

where ``YOUR_CAMERA_NAME`` is either ``C0`` or is the name configured in ``/etc/rts2/devices``. If an authorized
connection to XMLRPC, this is the default in ``rts2.ini``, is mandatory create an RTS2 user:

.. code-block:: bash

 postgres@localhost:~$ rts2-user -a YOUR_RTS2_USER # recommendation use default user: rts2saf
 User password: YOUR_PASSWD
 User email (can be left empty): YOUR_REAL_EMAIL@some.host # in case RTS2 sends emails

Specify an email address despite the dialog suggests that it can be left empty. The permission
to write to focuser, CCD, and filter wheel are granted with

.. code-block:: bash

 UPDATE users SET usr_execute_permission='t', allowed_devices = 'F0 C0 W0' WHERE usr_login='YOUR_RTS2_USER' ;

if default device names are configured in ``/etc/rts2/devices``. At this point an authorized
connection to XMLRPC with

.. code-block:: bash

 /etc/init.d/rts2 start
 your_browser http://127.0.0.1:8889/devices/F0

can be established, after entering YOUR_RTS2_USER, YOUR_PASSWD. If you receive ``Bad request Cannot find specified device``
the authorization took place but the device in question is not present. Check your ``/etc/rts2/devices`` file. To check if
all necessary devices are present and writable use
 
.. code-block:: bash

 rts2saf_focus.py --toc --check

which creates an output like

.. code-block:: bash

 wildi@nausikaa:~/rts-2/scripts/rts2saf/sphinx> rts2saf_focus.py --toc --check
 logging to: /tmp/rts2saf_log/rts2-debug instead of /var/log/rts2-debug
 create:  F0 setting internal limits from configuration file and ev. default values!
 create:  F0 has    FOC_DEF set, breaking
 check : focuser device: F0 present, breaking
 create:  COLWGRS, empty slot:open
 create:  COLWSLT, empty slot:open
 create:  COLWFLT, empty slot:open
 check: filter wheel device: COLWGRS present, breaking
 check: filter wheel device: COLWSLT present, breaking
 check: filter wheel device: COLWFLT present, breaking
 check: camera device: andor present, breaking
 filterOffsets: andor, COLWGRS no filter offsets could be read from CCD, but filter wheel/filters are present
 filterOffsets: andor, filter wheel COLWGRS defined filters [u'open']
 filterOffsets: andor, filter wheel COLWGRS used    filters ['open']
 filterOffsets: andor, COLWSLT no filter offsets could be read from CCD, but filter wheel/filters are present
 filterOffsets: andor, filter wheel COLWSLT defined filters [u'open']
 filterOffsets: andor, filter wheel COLWSLT used    filters ['open']
 filterOffsets: andor, filter wheel COLWFLT defined filters [u'open', u'R', u'g', u'r', u'i', u'z', u'Y', u'empty8']
 filterOffsets: andor, filter wheel COLWFLT used    filters ['open']
 checkBounds: open has no defined filter offset, setting it to ZERO
 checkBounds: COLWGRS open 0
 checkBounds: open has no defined filter offset, setting it to ZERO
 checkBounds: COLWSLT open 0
 checkBounds: open has no defined filter offset, setting it to ZERO
 checkBounds: COLWFLT open 0
 summaryDevices: focus run without multiple empty slots:
 summaryDevices: COLWGRS : open     6 steps, FOC_TOFF: [   -6,     9], FOC_POS: [   -6,    9], FOC_DEF:     0, Filter Offset:     0

 summaryDevices: COLWSLT : ['open'] has only empty slots
 summaryDevices: COLWFLT : ['open'] has only empty slots
 summaryDevices: taking 6 images in total
 deviceWriteAccess: this may take approx. a minute
 deviceWriteAccess: all devices are writable
 rts2saf_focus: configuration check done for config file:/usr/local/etc/rts2/rts2saf/rts2saf.cfg, exiting

and in case a device is not writeable

.. code-block:: bash

 wildi@nausikaa:~/rts-2/scripts/rts2saf/sphinx> rts2saf_focus.py --toc  --check
 logging to: /tmp/rts2saf_log/rts2-debug instead of /var/log/rts2-debug
 
 ...
 
 check: camera device: andor4 not yet present
 check : camera device: andor4 not present
 rts2saf_focus: could not create object for CCD: andor4, exiting



The following part until the end of the section is only necessary if you want to execute
the ``unittest``, which is recommended.

Create the Postgres database user 

.. code-block:: bash

 postgres@localhost:~$ createuser  YOUR_UID      # the user who executes rts2saf unittest

Grant access to database ``stars`` for YOUR_UID

.. code-block:: bash

   ALTER GROUP observers ADD USER YOUR_UID ;

This Postgres user is necessary since an almost complete RTS2 environment is created on the fly during 
the execution of the ``unittest``. A better way to execute ``unittest`` would be to create and destroy the Postgres DB
on  the fly as well - this is a ToDo.

Configure pg_haba.conf (Ubuntu: /etc/postgresql/9.1/main/pg_hba.conf) to

.. code-block:: bash

   # TYPE  DATABASE        USER          ADDRESS    METHOD
   # "local" is for Unix domain socket connections only                                                                                                                                                                               
   local   all             postgres                 peer # not strictly necessary
   local   stars           YOUR_UID                 trust # YOUR_UID is the user who executes rts2saf's unittest

The devices ``F0``, ``andor``, ``COLWFLT``, ``COLWGRS`` and ``COLWSLT``  are required by ``unittest``. Grant write
access with

.. code-block:: bash

 postgres@localhost:~$ psql stars
 UPDATE users SET usr_execute_permission='t', allowed_devices = 'F0 andor COLWFLT COLWGRS COLWSLT' WHERE usr_login='YOUR_RTS2_USER' ; # default: rts2saf

In case you want to execute rts2saf through ``unittest`` and EXEC use 

.. code-block:: bash

 postgres@localhost:~$ psql stars
 UPDATE users SET usr_execute_permission='t', allowed_devices = 'W0 C0 F0 andor COLWFLT COLWGRS COLWSLT' WHERE usr_login='YOUR_RTS2_USER' ;

if default device names are configured in ``/etc/rts2/devices``.


``rts2saf unittest`` 
--------------------

Not yet complete but 

.. code-block:: bash

 cd ~/rts-2/scripts/rts2saf
 ./rts2saf_unittest.py

may discover the most common installation problems. Before execution, edit the configuration
file ``~/rts-2/scripts/rts2saf/unittest/rts2saf-bootes-2-autonomous.cfg``

.. code-block:: bash

 [connection]
 USERNAME = YOUR_DB_USER
 PASSWORD = YOUR_PASSWD

according to your choice of the previous section. Check if ``SExtractor`` version >= 2.8.6 is available as

.. code-block:: bash

 /usr/local/bin/sex

E.g.,  create a link ``sudo ln -s /usr/bin/sextractor /usr/local/bin/sex``.

If not  all tests are ``ok`` please  send the output together with the contents of 
``/tmp/rts2saf_log/`` to the author. The execution of a complete focus 
run within a real RTS2 environment created and destroyed on the fly is explained in 
:ref:`Testing individual components <sec_unittest-label>`.



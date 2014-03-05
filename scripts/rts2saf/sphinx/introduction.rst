Introduction
============

Status and open issues (2014-03-01)
-----------------------------------
This description is not yet meant to be complete. Comments and corrections are very welcome.

The documentation describes 

1) the rts2saf installation and its integration into ``RTS2``
2) usage and testing with dummy devices
3) offline analysis of previously acquired data

Items which need further attention:

1) target selection for focus run: the focus run is carried out at the current
   telescope position, e.g. goto nearest Landolt target
2) finding the appropriate exposure 
3) further, e.g. faster methods to determine the FWHM minimum: currently about 6...8 images are taken see e.g. Petr's script ``focsing.py``
4) strategies if a focus run fails, e.g. widen the interval in regular mode, fall back to blind mode
5) ``SExtractor``'s filter  option
6) replacemenmt of ``matplotlib`` as plotting engine (it does'nt work well in the background and within threads)
7) many ToDos in the code

.. _sec_introduction-label:

7) RTS2 EXEC does not continue to select targets after an external script, like ``rts2saf_focus.py``, has finished. To reenable RTS2 EXEC a workaround has been created (``rts2saf_reenable_exec.py``) which is executed as a detached subprocess of ``rts2saf_focus.py``.



In case of questions, or if you need support, contact the author and
in case of failures execute

.. code-block:: bash

 ./rts2saf_unittest.py

and send its output together with the contents ``/tmp/rts2saf_log`` (best as ``tar.gz`` archive).




Quick hands on analysis
-----------------------

After  :ref:`installing all helper programs, Python packages <sec_installation-label>`  and the data files:

.. code-block:: bash

 wget http://azug.minpet.unibas.ch/~wildi/rts2saf-test-focus-2013-09-14.tgz
 tar zxvf rts2saf-test-focus-2013-09-14.tgz
 wget http://azug.minpet.unibas.ch/~wildi/rts2saf-test-focus-2013-11-10.tgz
 tar zxvf rts2saf-test-focus-2013-11-10.tgz

 wget http://azug.minpet.unibas.ch/~wildi/20131011054939-621-RA.fits

execute in the main rts2saf directory either

.. code-block:: bash

 ./rts2saf_hands_on_examples.sh

or some of these commands

.. program-output:: grep -vE "(\#|DONE|tail)"  ../rts2saf_hands_on_examples.sh 


Overview
--------
This is the description of the rts2saf auto focuser package.
The latest version is available at the RTS2 svn repo:
http://sourceforge.net/p/rts-2/code/HEAD/tree/trunk/rts-2/scripts/rts2saf/
including this description.


rts2saf is a complete rewrite of rts2af.  The goals were

0) a comprehensive command line user interface including sensible log messages,
1) simpler installation and configuration, 
2) a general solution for all RTS2 driven observatories,
3) the support of multiple filter wheels with an arbitrary number of slots,  
4) a modular software design which eases testing of its components.

rts2saf's main tasks are to determine the focus and set ``FOC_DEF``
during autonomous operations whenever the FWHM of an image exceeds 
a threshold.
Depending on the actual configuration it measures filter focus offsets 
relative to an empty or a predefined slot and writes these values
to the CCD driver.
In addition it provides a tool to analyze previous focus runs offline 
e.g. in order to create a temperature model.

The method to find the focus is straight forward. Acquire a set of images and 
determine FWHM using ``SExtractor``. The position of the minimum FWHM, derived
from the fitted function, is the focus.
The output of the fit program is stored as a PNG file and optionally displayed on screen. 
In addition various weighted means are calculated which are currently only logged.

Already available during offline analysis is an independent fit to the sum of the flux 
of sextracted objects. Comparing fluxes among the images makes only sense in
case the sextracted objects are identified on all images. This association is
optionally carried out by ``SExtractor`` itself.

To increase the chance that the fits converge errors for FWHM and flux are introduced.
In case of FWHM it is what ``SExtractor`` thinks the error is, while for flux it is
calculated as the average of the square roots of the flux values.

rts2saf makes use of RTS2's HTTP/JSON interface and hence using the scripts  
on the command line is encouraged before setting up autonomous operations. The JSON interface 
eases and speeds up the test phase considerably specially in the early stage
of debugging the configuration. The execution with 
``rts2-scriptexec -s ' exe script '`` is not needed any more. 

Test runs can be carried out during day time either with RTS2
dummy or real devices. If no real images can be taken, either 
because a dummy CCD or a real CCD is used during daytime, 
"dry fits files" are injected while optionally all involved 
devices operate as if it were night. These files can be images from 
a former focus run or if not available samples are provided by the 
author (see below).

Parameters, like e.g. ``FOC_DEF`` stored in focuser device, are retrieved 
from the running RTS2 instance as far as they are needed. All additional 
device or analysis properties are kept in a single configuration file. 
The number of
additional parameters stored in the configuration is intentionally
kept small.

During analysis ``DS9`` region of interest  data structures are created for each image. 
Optionally the images and the region files are displayed on screen using ``DS9``.
The circle is centered to ``SExtractor``'s x,y positions. Red circles indicate objects
which were rejected green ones which were accepted.

If rts2saf is executed remotely the X-Window DISPLAY variable has to be set otherwise 
neither the fit nor images are displayed. 

Modes of operations
+++++++++++++++++++
1) **autonomous operations**:
   ``rts2saf_imgp.py``, ``rts2saf_fwhm.py``, ``rts2saf_focus.py``
2) **command line execution**:
   ``rts2saf_focus.py``
3) **offline analysis**:
   ``rts2saf_analysis.py``

Focus runs come in two flavors:

1) 'regular'
2) 'blind'

Regular runs can be carried either in autonomous mode or on the
command line while blind runs are typically executed only on the
command line.

Regular runs in autonomous mode are optimized for minimum elapsed time
and typically are only carried out for the wheel's empty slot. That
does imply the knowledge of the real focus position within narrow limits.


Autonomous operations
+++++++++++++++++++++
Once an image has been stored on disk RTS2 calls ``rts2saf_imgp.py``
which carries out two tasks:

1) measurement of FWHM using ``SExtractor``
2) astrometric calibration using ``astrometry.net``

If the measured FWHM is above a configurable threshold ``rts2saf_fwhm.py``
triggers an on target focus run using selector's focus queue. This 
target is soon executed and ``rts2saf_focus.py`` acquires a configurable set  
of images at different focuser positions. To reduce elapsed time 
``SExtractor`` is executed in a thread  while images are
acquired. rts2saf then fits these points and the minimum is derived 
from the fitted function. If successful it sets focuser's ``FOC_DEF`` if
variable ``SET_FOC_DEF`` is set to ``True`` in the configuration file.

Command line execution
++++++++++++++++++++++
In order to simplify the debugging of one's own configuration 
all scripts can be used directly on the command line either
with or without previously acquired images.


Introduction
============

Status and open issues (2013-10-27)
-----------------------------------
This description is not yet meant to be complete. Comments and corrections are very welcome.

The documentation describes 

1) the rts2saf installation and its integration into ``RTS2``
2) usage and testing with dummy devices
3) offline analysis of previously acquired data

Items which need further attention:

1) target selection for focus run: the focus run is carried out at the current telescope position
2) finding the appropriate exposure 
3) further, e.g. faster methods to determine the FWHM minimum: currently about 6...8 images are taken see e.g. Petr's script ``focusing.py``
4) many ToDos in the code
5) more documentation (this file)
6) more unittests


Overview
--------
This is the description of the rts2saf auto focuser package.
The latest version is available at the RTS2 svn repo:
http://sourceforge.net/p/rts-2/code/HEAD/tree/trunk/rts-2/scripts/rts2saf/
including this description.

In case of questions, or if you need support, contact the author.

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

The output of the fit program is stored as a PNG file and optionally displayed on screen. 

During analysis ``DS9`` region of interest  data structures are created for each image. 
Optionally the images and the region files are displayed on screen using ``DS9``.
The circle is centered to ``SExtractor``'s x,y positions. Red circles indicate objects
which were rejected green ones which were accepted.

If rts2saf is executed remotely the X-Window DISPLAY variable has to be set otherwise 
neither the fit nor images are displayed. 

Modes of operations, involved scripts
+++++++++++++++++++++++++++++++++++++
1) **autonomous operations**:
   ``rts2saf_imgp.py``, ``rts2saf_fwhm.py``, ``rts2saf_focus.py``
2) **command line execution**:
   ``rts2saf_focus.py``
3) **offline analysis**:
   ``rts2saf_analysis.py``

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
from the fitted function. If successful it sets focuser's ``FOC_DEF``.

Command line execution
++++++++++++++++++++++
In order to simplify the debugging of one's own configuration 
all scripts can be used directly on the command line either
with or without previously acquired images.

All scripts have on line help and all arguments have a decent
default value which enables them to run in autonomous mode
where appropriate.

The configuration file contains all observatory
specific values which are not available from the running
RTS2 instance. An example:

.. code-block:: bash

 [filter properties]
 flt1 = [ R, -10, 10, 2, 1.1]
 
This line specifies a filter named 'R'. The numbers -10,10 define
the range the focuser scans in steps of 2, that means ca. 10 images
are taken. The last number is the factor by which the base exposure
time is multiplied.

Focus runs come in two flavors:

1) 'regular'
2) 'blind'

Regular runs can be carried either in autonomous mode or on the
command line while blind runs are typically executed only on the
command line.

Regular runs in autonomous mode are optimized for minimum elapsed time
and typically are only carried out for the wheel's empty slot. That
does imply the knowledge of the real focus position within narrow limits.

The measurement of the filter offsets (see your CCD driver) is done on
the command line and the results are manually written to file ``/etc/rts2/devices``:

.. code-block:: bash

 camd     fli    CCD_FLI     --focdev FOC_FLI --wheeldev FTW_FLI --filter-offsets 1644:1472:1346:1349:1267:0:701
 filterd  fli    FTW_FLI     -F "U:B:V:R:I:X:H"

The focus travel range is defined by the values given in section ``[filter properties]``
as explained above.
The range that the focuser should travel is highly dependent on the 
optics. As rule of thumb: if the FWHM minimum is 6 pixel wide then choose
the limits of the range so that the FWHM does not exceed 18 pixel intra- and
extra focal.

Blind focus runs are used in case minimum FWHM position is unknown. 
The values given in ``[filter properties]`` might be still meaningless hence the
focus travel range is defined by the values

.. code-block:: bash

 FOCUSER_LOWER_LIMIT = -12
 FOCUSER_UPPER_LIMIT = 15

The above values apply to RTS2's dummy focuser. If a focuser can travel within [0,7000] as e.g. the FLI PDF, appropriate values
might be

.. code-block:: bash

 FOCUSER_LOWER_LIMIT = 1000
 FOCUSER_UPPER_LIMIT = 5500
 FOCUSER_STEP_SIZE   = 500


and 10 images are exposed. Set the absolute limits

.. code-block:: bash

 FOCUSER_ABSOLUTE_LOWER_LIMIT = -16
 FOCUSER_ABSOLUTE_UPPER_LIMIT = 19

so that the sum of ``FOC_DEF`` and eventual filter offsets does not exceed either lower or upper limits of the real focuser. If unsure set them to the hardware limits. Execute 

.. code-block:: bash

  rts2saf_focus.py  --toconsole --blind

Normally the fit convergences but it does often not represent the minimum in ``--blind`` mode. Therefore
an estimator based on the weighted mean is the best guess. These
values appear as 

.. code-block:: bash

 analyze: FOC_DEF:   258: weighted mean derived from sextracted objects
 analyze: FOC_DEF:   286: weighted mean derived from FWHM
 analyze: FOC_DEF:   305: weighted mean derived from std(FWHM)
 analyze: FOC_DEF:   342: weighted mean derived from Combined

on the console. Under normal circumstances the ``weighted mean derived from Combined``
is the closest approximation of the true value.


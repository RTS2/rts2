Offline analysis
================

The script ``rts2saf_analyze.py`` provides a very basic analysis, namely

 1) the minimum of the fitted function
 2) a linear fit of the minima as a function of ambient temperature if available in FITS header
 3) FITS image display through ``DS9`` together with their region files.

``rts2saf_analyze.py`` is not restricted to ``RST2/rts2saf_focus.py`` created FITS files
as long as the images are grouped properly (see below).

The fit result is stored as ``PNG`` in the directory where the FITS files reside.
Trough option ``--filternames`` the analysis can be restricted to a list of filter names.

Option ``--cataloganalysis`` performs the analysis based on the properties available through
``SExtractor's`` ``parameters``. This feature is still experimental and will be implemented
later. Currently a decission is made based on the distance from the plate center:

.. code-block:: python

    def __criteria(self, ce=None):

        rd= math.sqrt(pow(ce[self.i_x]-self.center[0],2)+ pow(ce[self.i_y]-self.center[1],2))
        if rd < self.radius:
            return True
        else:
            return False

Image directory lay out
-----------------------

``rts2saf_focus.py`` copies the files from RTS2 images directory to BASE_DIRECTORY.
In cas a filter wheel is used to:

.. code-block:: bash

 BASE_DIRECTORY
 |   |-- 2013-10-06T00:45:50.388784
 |   |   `-- COLWFLT
 |   |       |-- R
 |   |       |   |-- 20131006004742-269-RA-000.fits
 |   |       |   |-- 20131006004800-189-RA-001.fits
 |   |       |   |-- 20131006004818-081-RA-002.fits
                 ...
 |   |       |   `-- 20131006004929-777-RA-006.fits
 |   |       `-- open
 |   |           |-- 20131006004553-264-RA-000.fits
 |   |           |-- 20131006004601-425-RA-001.fits
                 ...
 |   |           `-- 20131006004728-017-RA-012.fits
     ...
 ...

First sub level is the date, where the run took place, second level is named after the filter wheel (``COLWFLT``)
and the third level is named after the used filter (``R``, ``open``). In case no filter wheel is used the
last two levels are dropped:

.. code-block:: bash

 BASE_DIRECTORY
 |   |-- 2013-10-06T00:45:50.388784
 |   |       |-- 20131006004742-269-RA-000.fits
 |   |       |-- 20131006004800-189-RA-001.fits
 |   |       |-- 20131006004818-081-RA-002.fits
                 ...
 |   |       `-- 20131006004929-777-RA-006.fits


``rts2saf_focus.py`` copes with directory structures like

.. code-block:: bash

  BASE_DIRECTORY
 |-- 2013-10
 |   |-- 2013-10-01T19:10:48.363237
 |   |   `-- COLWFLT
 |   ...
 ...

and so on.

Focuser temperature model
-------------------------
Interactive usage is carried out through

.. code-block:: bash

 rts2saf_analyze.py --toconsole --fitdisplay --ds9display --basepath BASE

while 

.. code-block:: bash

 rts2saf_analyze.py --basepath BASE

does it all quietly writing only to the log file and to the fit result image files. The parameter
values for the temperature model can be retrievd from the log file.

Monitoring
----------

During acquisition, ``rts2saf_analyze.py`` is being executed by EXEC in the background, 
no plots or images are displayed. To get an idea how an ongoing focus run looks like use

.. code-block:: bash

 rts2saf_analyze.py --toconsole --fitdisplay --ds9display --basepath  BASE_DIRECTORY/DATE 

where ``BASE_DIRECTORY`` refers to the configuration file and ``DATE`` to the start time.
The processes do not interfere at all.

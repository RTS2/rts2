Installation
============

Prerequisites
-------------

Update to Python 2.7.x (mandatory) and various Python packages:

1) DS9 from http://hea-www.harvard.edu/RD/ds9/site/Home.html
2) numpy, numpy-devel
3) pip install astropy
4) pip install sphinxcontrib-programoutput

During RTS2 installation the rts2saf executable are installed to 

  /usr/local/bin 

and the modules to

  /usr/local/lib/python2.7/dist-packages/rts2saf/

In case you modify a rts2saf module issue

  | cd ~/rts-2/scripts
  | sudo make install

For the following description I assume you did

  cd ~
  svn co https://rts-2.svn.sf.net/svnroot/rts-2/trunk/rts-2 rts-2

RTS2 configuration file
-----------------------

Save  /etc/rts2/devices and replace it with the dummy devices
 
  | cd ~/rts-2/conf/
  | sudo mkdir -p /usr/local/etc/rts2/
  | sudo cp -a rts2saf /usr/local/etc/rts2/
  | cd /etc/rts2/
  | mv devices devices.save
  | ln -s ~/rts-2/scripts/rts2saf/configs/one-filter-wheel/devices .

Edit /usr/local/etc/rts2/rts2af/rts2saf.cfg  and check if 
SExtractor binary is found.

rts2saf configuration files
---------------------------
rts2saf needs two configuration files to be present in /usr/local/etc/rts2/rts2saf:

1) rts2saf.cfg
2) rts2saf-sex.cfg

rts2saf.cfg ist used by rts2saf and rts2saf-sex.cfg by SExtractor. A usable example for the latter is stored in ~/rts-2/conf/rts2saf. In directory ~/rts-2/scripts/rts2saf/configs

   | dummy-bootes-2
   |   devices
   |   rts2saf.cfg
   | dummy-bootes-2-autonomous
   |   devices
   |   rts2saf.cfg
   | no-filter-wheel
   |   devices
   |   rts2saf.cfg
   | one-filter-wheel
   |   devices
   |   rts2saf.cfg
   | one-filter-wheel-autonomous
   |   devices
   |   rts2saf.cfg

you'll find four sets of rts2saf configuration files with their
associated device files. The postfix '-autonomous' denotes configurations
which are used while rts2saf is integrated in RTS2.


Postgres DB tables target and scripts entries
---------------------------------------------
As user postgres:

  | psql --user YOUR_USERNAME YOUR_DB # (see /etc/rts2/rts2.ini)
  | insert into targets values ('5', 'o', 'OnTargetFocus', null, null, 'this target does not change the RA/DEC values', 't', '1');
  | insert into scripts values ('5', 'YOUR_CAMERA_NAME', ' exe /YOUR/HOME/rts-2/scripts/rts2saf/rts2saf_focus.py');


Adding the devices to the Postgres DB is usually done by script 
rts-2/scripts/ubuntu-rts2-install, in case not execute as user postgres:

   | cd ~/rts-2/src/sql
   | ./rts2-configdb stars -t T0
   | ./rts2-configdb stars -c C0
   | ./rts2-configdb stars -f W0

The filters are stored in the Postgres DB table filters. These entries are not strictly necessary 
but it is recommended to add them.

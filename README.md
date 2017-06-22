RTS2 - Remote Telescope System, 2nd Version
===========================================

RTS2 is software for a fully autonomous astronomical observatory. It takes care
of details necessary for the observatory operation. It keeps care of closing it
when bad weather arrives, opening when weather permits operation. One of its
driving requirements was to provide a modular and flexible environment, able to
control a lot of (quickly changing) devices.

RTS2 is split into two main components - the RTS2 library, released under LGPL,
and RTS2 drivers, released under GPL. Pleasse see COPYING.LESSER and COPYING and 
header of source files for details.

RTS2 is not an interactive planetarium. RTS2 provides various interfaces which
third parties can use to access system functionalities. Probably the best is
JSON API. [Stellarium](http://stellarium.org)

For further details, please visit:

  [http://www.rts2.org](http://www.rts2.org)

For questions, comments, suggestions and problems, please send emails to petr
(at) rts2 [dot] org or to the RTS2 developer list rts-2-devel (at) lists
[dot] sourceforge [dot] net. Please be aware that support is first provided
to parties sharing development and implementation costs.

How to install RTS2
===================

The easiest way on Ubuntu/debian based distributions is to install RTS2 from packages.
The packages are available from [https://launchpad.net/~rts2/+archive/ubuntu/daily].

If you want to install from the source code, please see the [INSTALL](./INSTALL) file.

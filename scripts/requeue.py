#!/usr/bin/env python

# Requeue script
# (C) 2018 Petr Kubanek
#
# You most probably would like to modify this file to suit your needs.
# Please see comments in flats.py for details of the parameters.

import rts2.scriptcomm

s = rts2.scriptcomm.Rts2Comm()

s.setValue('filter', 2)

s.requeue('requeue', 30*60 )

s.setValue('exposure', 20)
imname = s.exposure()
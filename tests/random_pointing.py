#!/usr/bin/python
# coding=utf8

# SCript to execute random pointing from bash through rts2.json library.
# Please use with care, change username and password.
#
# (C) 2015 Petr Kubanek <petr@kubanek.net>

import rts2.json
import random
import time
import sys

# create Proxy object to work with RTS2 json
# You need to change username and password!
# See man rts2-user for details.

j = rts2.json.JSONProxy("http://localhost:8889", "username", "password", verbose=False)

# format output altitude and azimuth, so those will not be only numbers..
def formatDeg(deg):
	m, s = divmod(deg * 3600, 60)
	h, m = divmod(m, 60)
	return '{0:.0f}°{1:02.0f}\'{2:02.0f}"'.format(h, m, s)

while True:
	al = random.random() * 90
	az = random.random() * 360

	j.refresh()
	j.getDevice("T0", True)
	
	# print variables without newline
	print "moving to {0} {1}, actual {2} {3}, distance {4}                 \r".format(formatDeg(al), formatDeg(az), formatDeg(j.getValue("T0","TEL_")["alt"]), formatDeg(j.getValue("T0","TEL_")["az"]), formatDeg(j.getValue("T0","target_distance"))),
	# flush screen so it gets updated
	sys.stdout.flush()

	j.setValue("T0", "TEL_", "{0} {1}".format(al, az))
	while j.getState("T0") & 0x01:
		# get actual alt-az coordinates
		# force telescope to re-read coordinates
		j.executeCommand("T0", "info")
		# refresh all variables hold by the proxy
		j.refresh()
		# print variables without newline
		print "moving to {0} {1}, actual {2} {3}, distance {4}                 \r".format(formatDeg(al), formatDeg(az), formatDeg(j.getValue("T0","TEL_")["alt"]), formatDeg(j.getValue("T0","TEL_")["az"]), formatDeg(j.getValue("T0","target_distance"))),
		# flush screen so it gets updated
		sys.stdout.flush()
		# sleep a bit..
		time.sleep(0.1)

	print "moving to {0} {1}, actual {2} {3}, distance {4}".format(formatDeg(al), formatDeg(az), formatDeg(j.getValue("T0","TEL_")["alt"]), formatDeg(j.getValue("T0","TEL_")["az"]), formatDeg(j.getValue("T0","target_distance")))

	# sleeps for 5 seconds with a count
	for s in range(5, 0, -1):
		print "Sleep .. {0}\r".format(s),
		sys.stdout.flush()
		time.sleep(1)
	print "next move.."

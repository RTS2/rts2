#!/usr/bin/python
#   Schedule targets to remote observatory.
#
#   This script should be run by rts2-bb. It creates target on remote observatory.
#
#   (C) 2012-2013 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2, or (at your option)
#   any later version.
#
#   Please visit http://www.gnu.org/licenses/gpl.html for license informations.

import sys
import rts2.json
import rts2.target
import re
import time
import sys
from optparse import OptionParser

parser = OptionParser(usage='usage: %prog (--obs-tar-id <id> | --create <id>) <observatory API urls..>')
parser.add_option('--obs-tar-id', help='target ID (on central node)', action='store', dest='obs_tar_id', default=None)
parser.add_option('--create', help='create target with given ID', action='store', dest='create', default=False)
parser.add_option('--verbose', help='report progress', action='store_true', dest='verbose', default=False)

(options, args) = parser.parse_args()

for obs_id in args:
	print "obsapiurl", obs_id
	sys.stdout.flush()
	obs_url = sys.stdin.readline().rstrip('\n')
	obs_user = sys.stdin.readline().rstrip('\n')
	obs_password = sys.stdin.readline().rstrip('\n')

	rts2.json.createProxy(obs_url, obs_user, obs_password, options.verbose)

	if options.create:
		print "targetinfo", options.create
		sys.stdout.flush()
		a = sys.stdin.readline().rstrip('\n')
		(tar_name, tar_ra, tar_dec) = re.match('"([^"]*)" (\S*) (\S*)', a).groups()

		obs_tar_id = rts2.target.create(tar_name, tar_ra, tar_dec)
		print 'mapping', obs_id, options.create, obs_tar_id
		sys.stdout.flush()
	else:
		if options.obs_tar_id is None:
			print >>sys.stderr, 'log E scheduling_target.py: missing --obs-tar-id or --create parameters'
			sys.exit(1)
		obs_tar_id = int(options.obs_tar_id)

	# contact about when the target will be able to schedule..
	sche = rts2.json.getProxy().loadJson('/bbapi/schedule', {'id':obs_tar_id})

	if int(sche['ret']):
		print 'log I cannot schedule {0}', str(sche)
		print 'unscheduled'
	else:
		print 'log I schedule returns {0}'.format(time.ctime(int(sche['from'])))	
		print 'schedule_from',int(sche['from'])

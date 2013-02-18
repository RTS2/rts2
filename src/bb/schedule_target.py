#!/usr/bin/python

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

	rts2.json.createProxy(obs_url, 'petr', 'test', False)

	if options.create:
		print "targetinfo", options.create
		sys.stdout.flush()
		a = sys.stdin.readline().rstrip('\n')
		(tar_name, tar_ra, tar_dec) = re.match('"([^"]*)" (\S*) (\S*)', a).groups()

		ret = rts2.target.create(tar_name, tar_ra, tar_dec)
		obs_tar_id = int(ret['id'])
		print 'mapping', obs_id, options.create, obs_tar_id
		sys.stdout.flush()
	else:
		if options.obs_tar_id is None:
			print >>sys.stderr, 'log E scheduling_target.py: missing --obs-tar-id or --create parameters'
			sys.exit(1)
		obs_tar_id = int(options.obs_tar_id)

	# contact about when the target will be able to schedule..
	ret = rts2.json.getProxy().loadJson('/bbapi/schedule', {'id':obs_tar_id})

	if int(ret) == 0:
		print 'log I cannot schedule'
		print 'unscheduled'
	
	print 'log I schedule returns {0}'.format(time.ctime(int(ret)))	
	print 'schedule_from',int(ret)

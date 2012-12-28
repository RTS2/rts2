#!/usr/bin/python

import sys
import rts2.json
import re
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

		ret = rts2.json.getProxy().loadJson('/api/create_target', {'tn':tar_name, 'ra':tar_ra, 'dec':tar_dec})
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

	print 'log I schedule returns {0}'.format(ret)	

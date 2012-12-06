#!/usr/bin/python

import sys
import rts2.json
from optparse import OptionParser

parser = OptionParser(usage='usage: %prog --obs-tar-id <id> [--create <target_name> <ra> <dec>] <observatory API urls..>')
parser.add_option('--tar-id', help='target ID (on central node)', action='store', dest='tar_id', default=None)
parser.add_option('--create', help='create target', action='store_true', dest='create', default=False)
parser.add_option('--verbose', help='report progress', action='store_true', dest='verbose', default=False)

(options, args) = parser.parse_args()

for obs_urls in sys.argv[1:]:
	rts2.json.createProxy(obs_url, 'petr', 'test', False)

	if options.create:
		ret = rts2.json.getProxy().loadJson('/api/create_target', {'tn':tar_name, 'ra':tar_ra, 'dec':tar_dec})
		print 'mapping', options.tar_id, ret["id"]

	# contact about when the target will be able to schedule..
	ret = rts2.json.getProxy().loadJson('/api/bb_schedule', {'id':options.tar_id})


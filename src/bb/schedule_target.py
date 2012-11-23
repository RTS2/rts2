#!/usr/bin/python

import sys
import rts2.json
from optparse import OptionParser

parser = OptionParser(usage='usage: %prog --obs-tar-id <id> [--create <target_name> <ra> <dec>] <observatory API url>')
parser.add_option('--tar-id', help='target ID (on central node)', action='store', dest='tar_id', default=None)
parser.add_option('--create', help='Create target', action='store_true', dest='create', default=False)
parser.add_option('--verbose', help='Report progress', action='store_true', dest='verbose', default=False)

(options, args) = parser.parse_args()

obs_url = args[1]

rts2.json.createProxy(obs_url, 'petr', 'test', False)

if options.create:
	ret = rts2.json.getProxy().loadJson('/api/create_target', {'tn':tar_name, 'ra':tar_ra, 'dec':tar_dec})
	print 'mapping',options.tar_id,ret["id"]



#!/usr/bin/python

import sys
import rts2.json
from optparse import OptionParser

parser = OptionParser()
parser.add_option('--create', help='Create target', action='store_true', dest='create', default=False)
parser.add_option('--verbose', help='Report progress', action='store_true', dest='verbose', default=False)

obs_url = sys.argv[1]
tar_name = sys.argv[2]
tar_ra = sys.argv[3]
tar_dec = sys.argv[4]

if (parser.

rts2.json.createProxy(obs_url, 'petr', 'test', False)
ret = rts2.json.getProxy().loadJson('/api/create_target', {'tn':tar_name, 'ra':tar_ra, 'dec':tar_dec})

print ret["id"]

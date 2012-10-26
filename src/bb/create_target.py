#!/usr/bin/python

import sys
import rts2.json

obs_url = sys.argv[1]
tar_name = sys.argv[2]
tar_ra = sys.argv[3]
tar_dec = sys.argv[4]

rts2.json.createProxy(obs_url, 'petr', 'test', True)
rts2.json.getProxy().loadJson('/api/create_target', {'tn':tar_name, 'ra':tar_ra, 'dec':tar_dec})

#!/usr/bin/python

import ephem
import time
import json

"""Module for TLE computation.
   Allows load set of satellites and cities/observatiries. On the set, passes can
   be computed."""

class TLESet:
	"""Set of two line elements satellite data.
	Set can be loaded from a file. Satellite visibility can be computed for
	set of cities."""
	def __init__(self):
		self.sats = []

	def parse_file(self, filename):
		f = open(filename)
		lines = [None,None,None]
		l = f.readline()
		while not(l == ''):
			if l[0] == '1' or l[0] == '2':
				lines[int(l[0])] = l
			elif not(l[0] == '#'):
				lines[0] = l

			if lines[1] is not None and lines[2] is not None:
				if lines[0] is None:
					lines[0] = '#{0}'.format(len(self.sats) + 1)
				self.sats.append(ephem.readtle(*lines))
				lines = [None, None, None]

			l = f.readline()
		f.close()

class Observatories:
	"""Set of cities or lat/lon/altitude with name"""
	def __init__(self):
		self.observatories = []

	def parse_file(self, filename):
		f = open(filename)
		l = f.readline()
		while not(l == ''):
			if l[0] == '#':
				pass
			else:
				l = l.rstrip()
				try:
					self.observatories.append(ephem.city(l))
				except KeyError,ke:
					obs = ephem.Observer()
					le = l.split()
					if len(le) == 4:
						obs.name = le[0]
						obs.long = le[1]
						obs.lat = le[2]
						obs.elevation = float(le[3])
						self.observatories.append(obs)
			l = f.readline()
		f.close()

def next_visible_pass(sat,obs,date,future_days=30):
	while future_days > 0:
		obs.date = date
		sunset = obs.next_setting(ephem.Sun())
		obs.date = sunset
		sat.compute(obs)
		t_rise = sat.rise_time
		t_transit = sat.transit_time
		t_set = sat.set_time

		if t_rise is None or t_transit is None or t_set is None:
			future_days -= 1
			date += 1
			continue

		obs.date = t_rise
		sat.compute(obs)
		rise_eclipsed = sat.eclipsed
		rise_alt = sat.alt
		rise_az = sat.az

		obs.date = t_transit
		sat.compute(obs)
		transit_eclipsed = sat.eclipsed
		transit_alt = sat.alt
		transit_az = sat.az

		obs.date = t_set
		sat.compute(obs)
		set_eclipsed = sat.eclipsed
		set_alt = sat.alt
		set_az = sat.az

		return {'rise':t_rise, 'rise_alt':rise_alt, 'rise_az':rise_az,
			'transit':t_transit, 'transit_alt':transit_alt, 'transit_az':transit_az,
			'set':t_set, 'set_alt':set_alt, 'set_az':set_az}
	return None

if __name__ == '__main__':
	obs = Observatories()
	obs.parse_file('c.list')
	
	sat = TLESet()
	sat.parse_file('t.tle')

	date = ephem.now()

	for o in obs.observatories:
		print o.name
		for s in sat.sats:
			p = next_visible_pass(s,o,date)
			if p is not None:
				print 'Sat: {0} Observatory: {1} transit: {2} alt: {3} az: {4}'.format(s.name, o.name, p['transit'], p['transit_alt'], p['transit_az'])


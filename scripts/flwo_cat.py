#!/usr/bin/python

import sys
import subprocess
import re

class FLWOCAT:
	def __init__(self):
		self.tarid = None

	def run_cmd(self,cmd,read_callback=None):
		print cmd
		proc = subprocess.Popen(cmd,stdout=subprocess.PIPE)
		if read_callback:
			while (True):
				a=proc.stdout.readline()
				if a:
					read_callback(a)
				else:
					break

		return proc.wait()
	
	def parse_script(self,scr):
		ret = ''
		s = scr.split(',')
		for se in s:
		  	fil = se.split('-')
			ret += 'filter=' + fil[0] + ' ';
			if (fil[2] > 1):
				ret += 'for ' + fil[2] + ' { E ' + fil[1] + ' }'
			else:
			  	ret += 'E ' + fil[1]
			ret += ' '
		return ret
	
	def process_newtarget(self,line):
		m = re.search ('#(\d+)',line)
		if m:
			self.tarid = m.group(1)
			print 'Target ID is %s' % (self.tarid)
	

	def run(self):	
		for arg in sys.argv[1:]:
			sched = open(arg)
			for l in sched.readlines():
			  	l=l.rstrip()
				if (len(l) == 0):
				  	continue
				a = l.split()
				if a[0][0] == '#':
				  	continue
				if a[0][0] == '!':
					continue
				scripts = a[3].split(',')
				if (a[2][0] != '-' and a[2][0] != '+'):
				  	a[2] = '+' + a[2]
				self.run_cmd(["rts2-newtarget", '-m', a[0], a[1] + ' ' + a[2]],self.process_newtarget)

				if self.tarid is None:
					print >> sys.stderr, 'rts2-newtarget does not produces target ID, exiting'
					exit(0)
				
				# parse script
				script = 'tempdisable 1800 ' + self.parse_script(a[6])

				prior = int(a[9]) * 100
				if (prior <= 0):
				  	prior = 1
		
				cmd = ["rts2-target", "-b", "0", "-e", "-p", str(prior), "-c", "KCAM", "-s", script]
		
				if float(a[11]) > 0:
					cmd.append ("--airmass")
					cmd.append (":" + a[11])

				if float(a[12]) > 0:
					if float(a[12]) > 90:
						cmd.append ("--lunarAltitude")
						cmd.append (":0")
					else:
						cmd.append ("--lunarDistance")
						cmd.append (a[12] + ":")

				cmd.append(self.tarid)

				self.run_cmd(cmd)

a = FLWOCAT()
a.run()

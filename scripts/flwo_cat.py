#!/usr/bin/python

import sys
import subprocess

def run_cmd(cmd):
	print cmd
	proc = subprocess.Popen(cmd)
	proc.wait()

def parse_script(scr):
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
		cmd = ["rts2-newtarget", a[0], "Object " + a[0], a[1] + ' ' + a[2]]
		run_cmd(cmd)

		# parse script
		script = parse_script(a[4])

		cmd = ["rts2-target", "-c", "KCAM", "-s", script, a[0]]
		run_cmd(cmd)

		cmd = ["rts2-target", "-e", "-p", str(int(a[7]) * 100), a[0]]
		run_cmd(cmd)

#!/usr/bin/python

import smtplib
import time
import traceback
import sys

# try pre 4.0 email version
try:
        from email.mime.text import MIMEText
except:
        from email.MIMEText import MIMEText

emails = ['Petr Kubanek <petr@kubanek.net>']
logfile = '/tmp/last_system_mail'
report = '/pool/weather/robot.error'

msg_timeout = 3600
image_timeout = 1800

def update_last_file():
	f = open(logfile, 'w')
	f.write(str(time.time()))
	f.close()

def get_last_time():
	try:
		f = open(logfile, 'r')
		ret = float(f.readline())
		f.close()
		return ret
	except IOError, io:
		return time.time() - 86400


def sendEmail(subject, body):
	if get_last_time() < time.time() - msg_timeout:
		s = smtplib.SMTP('localhost')
		mimsg = MIMEText(subject + '\n' + body)
		mimsg['Subject'] = subject
		mimsg['To'] = ','.join(emails)
		s.sendmail('robot@flwo48.sao.arizona.edu', emails, mimsg.as_string())
		s.quit()

		update_last_file()

		if report is not None:
			f = open(report, 'w')
			f.write(subject + '\n' + body)
			f.close()
try:
	import rts2.json
	import rts2.scriptcomm

	time.sleep(30)

	sc = rts2.scriptcomm.Rts2Comm()

	while True:
		next_state = sc.getValueInteger('next_state', 'centrald')

		ms = sc.getState('centrald')

		sc.timeValue('last_test', 'last system test', time.time())

		if next_state == 0 or next_state == 1 or next_state == 2 or next_state == 5 or (ms & 0x10):
			# no tests, as system is in day or is in standby
			time.sleep(60)
			continue

		if next_state == 3 and ((ms & 0x0f) == 11 or (ms & 0x0f) == 12) and not(ms & 0x80000000):
			sendEmail('1.2m Robot: SYSTEM IS IN OFF STATE DURING DUSK!', 'RTS2-F is in OFF state. Please switch it at least to stanby to allow robotic operations')

		last_night_on = sc.getValueFloat('last_night_on', 'centrald')
		last_astrom = sc.getValueFloat('last_astrom', 'IMGP')
		last_noastrom = sc.getValueFloat('last_noastrom', 'IMGP')
		last_failed = sc.getValueFloat('last_failed', 'IMGP')

		if last_night_on is not None and last_night_on < (time.time() - image_timeout):
			min_time = image_timeout
			min_good = image_timeout
			if last_astrom is not None and last_astrom > last_night_on:
				min_time = min(min_time, time.time() - last_astrom)
				min_good = min_time
			if last_noastrom is not None and last_noastrom > last_night_on:
				min_time = min(min_time, time.time() - last_noastrom)
			if last_failed is not None and last_failed > last_night_on:
				min_time = min(min_time, time.time() - last_failed)

			#if min_time >= image_timeout:
			#	sendEmail('1.2m Robot: No images produced in expected time. ({0} seconds)'.format(image_timeout), 'Please check that the 1.2m RT system and robot are running and not hung')
			#elif min_good >= image_timeout:
			#	sendEmail('1.2m Robot: There wasn\'t a good image in expected time. ({0} seconds)'.format(image_timeout), 'Please check the images 1.2m is producing, probably there will be some problem with TCS')
			#else:
				#sc.log('D', 'all ok {0} {1} {2} {3} {4}'.format(min_time, last_night_on, last_astrom, last_noastrom, last_failed))
				#sendEmail('ROBOT: all ok', str(min_time) + ' ' + str(last_night_on) + ' ' + str(last_astrom) + ' ' + str(last_noastrom) + ' ' + str(last_failed))
		time.sleep(60)

except Exception, ex:
	traceback.print_exc()
	sendEmail('1.2m Robot: SOMETHING IS WRONG!', 'Something is wrong with the robot - {0} {1}'.format(ex, traceback.format_exc()))
	sys.exit(1)

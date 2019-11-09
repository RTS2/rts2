#!/usr/bin/env python

# script which demonstartes move functions
#
# (C) 2010 Petr Kubanek <kubanek@fzu.cz>

import rts2.scriptcomm

class Test(rts2.scriptcomm.Rts2Comm):
	def __init__(self):
		rts2.scriptcomm.Rts2Comm.__init__(self)
	
	def run(self):
		self.radec("20:30:40",20)
		self.setValue('exposure',10)
		self.exposure()
		self.setValue('exposure',20)
		self.exposure()
		self.newObs(30,40)
		self.altaz(10,20)
		self.setValue('exposure',1)
		self.exposure()
		self.setValue('exposure',2)
		self.exposure()
		self.log('I','offseting by 0.1 degree in RA and DEC')
		self.setValueByType(DEVICE_TELESCOPE,'OFFS','0.1 0.1')
		self.altaz(40,50)
		self.setValue('exposure',3)
		self.altaz(60,70)
		self.exposure()
		self.newObsAltAz(60,60)
		self.exposure()

if __name__ == "__main__":
	a = Test()
	a.run()

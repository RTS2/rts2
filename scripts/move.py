#!/usr/bin/python

# script which demonstartes move functions
#
# (C) 2010 Petr Kubanek <kubanek@fzu.cz>

import rts2comm

class Test(rts2comm.Rts2Comm):
	def __init__(self):
		rts2comm.Rts2Comm.__init__(self)
	
	def run(self):
		self.radec(10,20)
		self.setValue('exposure',10)
		self.exposure()
		self.setValue('exposure',20)
		self.exposure()
		self.newObs(30,40)
		self.setValue('exposure',1)
		self.exposure()
		self.setValue('exposure',2)
		self.newObs(40,50)
		self.setValue('exposure',3)
		self.exposure()

if __name__ == "__main__":
	a = Test()
	a.run()

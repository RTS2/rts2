#!/usr/bin/env python
from rts2 import  scriptcomm

class Focusing (scriptcomm.Rts2Comm):	
    
    def __init__(self,exptime = 2,step=1,attempts=20 ):
        scriptcomm.Rts2Comm.__init__(self)
        self.exptime = 2 # 60 # 10
        self.focuser = self.getValue('focuser')
        self.step = 10 # 0.2
        self.attempts = 1 #30 # 20

        # if |offset| is above this value, try linear fit
        self.linear_fit = self.step * self.attempts / 2.0
        # target FWHM for linear fit
        self.linear_fit_fwhm = 3.5

    def beforeReadout(self):
        self.current_focus = self.getValueFloat('FOC_POS',self.focuser)
        if (self.num == self.attempts):
            self.setValue('FOC_TOFF',0,self.focuser)
        else:
            self.off += self.step
            self.setValue('FOC_TOFF',self.off,self.focuser)

    def takeImages(self):
        self.setValue('exposure',self.exptime)
        self.setValue('SHUTTER','LIGHT')
        self.off = -1 * self.step * (self.attempts / 2)
        self.setValue('FOC_TOFF',self.off,self.focuser)
        tries = {}
        # must be overwritten in beforeReadout
        self.current_focus = None
        self.log("I", "attempting exposures")
        for self.num in range(1,self.attempts+1):
                self.log('I','starting {0}s exposure on offset {1}'.format(self.exptime,self.off))
                img = self.exposure(self.beforeReadout,'%b/focusing/%N/%o/%f')
                tries[self.current_focus] = img

        self.log('I','all focusing exposures finished, processing data')

    

f=Focusing()
f.takeImages()

""" 
Python INDI client for FLI Filter Wheels

Copyright (C) 2019 Michael Mommert, Lowell Observatory
based on https://sourceforge.net/p/pyindi-client/code/HEAD/tree/trunk/pip/pyindi-client/examples/testindiclient.py

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

---

This client has to run together with an indiserver (127.0.0.1, PORT 7624)
to make rts2-filterd-fliindi connect to the filter wheel.
"""
import PyIndi
import time

class IndiClient(PyIndi.BaseClient):

    device = None

    def __init__(self):
        super(IndiClient, self).__init__()

    def newDevice(self, d):
        global dmonitor
        # We catch the monitored device
        dmonitor = d
        print("New device ", d.getDeviceName())
        if d.getDeviceName() == "FLI CFW":
            print("Set new device FLI CFW!")
            # save reference to the device in member variable
            self.device = d
        
    def newProperty(self, p):
        global monitored
        global cmonitor
        # we catch the "CONNECTION" property of the monitored device
        if (p.getDeviceName() == monitored and p.getName() == "CONNECTION"):
            cmonitor = p.getSwitch()
        print("New property ", p.getName(), " for device ",
              p.getDeviceName())
        if self.device is not None and p.getName() == "CONNECTION" and
        p.getDeviceName() == self.device.getDeviceName():
            print("Got property CONNECTION for FLI CFW!")
            # connect to device
            self.connectDevice(self.device.getDeviceName())
            self.getAll()

    def removeProperty(self, p):
        pass

    def newBLOB(self, bp):
        pass

    def newSwitch(self, svp):
        pass

    def newNumber(self, nvp):
        global newval
        global prop
        # We only monitor Number properties of the monitored device
        prop = nvp
        newval = True

        self.getStuff()

    def newText(self, tvp):
        pass

    def newLight(self, lvp):
        pass

    def newMessage(self, d, m):
        pass

    def serverConnected(self):
        pass

    def serverDisconnected(self, code):
        pass

    def getStuff(self):
        lp = self.device.getProperties()
        for p in lp:
            if p.getName() == 'FILTER_SLOT':
                tpy = p.getNumber()
                for t in tpy:
                    print('--', t.name, t.value)

    def getAll(self):
        lp=self.device.getProperties()
        for p in lp:
            print("   > "+p.getName())
            if p.getType()==PyIndi.INDI_TEXT:
                tpy=p.getText()
                for t in tpy:
                    print("       "+t.name+"("+t.label+")= "+t.text)
            elif p.getType()==PyIndi.INDI_NUMBER:
                tpy=p.getNumber()
                for t in tpy:
                    print("       "+t.name+"("+t.label+")= "+str(t.value))
            elif p.getType()==PyIndi.INDI_SWITCH:
                tpy=p.getSwitch()
                for t in tpy:
                    print("       "+t.name+"("+t.label+")= ")
            elif p.getType()==PyIndi.INDI_LIGHT:
                tpy=p.getLight()
                for t in tpy:
                    print("       "+t.name+"("+t.label+")= ")
            elif p.getType()==PyIndi.INDI_BLOB:
                tpy=p.getBLOB()
                for t in tpy:
                    print("       "+t.name+"("+t.label+")= <blob "+
                          str(t.size)+" bytes>")
        
monitored = "FLI CFW"
dmonitor = None
cmonitor = None

indiclient = IndiClient()
indiclient.setServer("localhost", 7624)

# we are only interested in the telescope device properties
indiclient.watchDevice(monitored)
indiclient.connectServer()

# wait CONNECTION property be defined
while not(cmonitor):
    time.sleep(0.05)

while True:
    time.sleep(1)


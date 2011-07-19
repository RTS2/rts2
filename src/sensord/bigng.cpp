/* 
 * T-Balancer bigNG smart fan controller 
 * Copyright (C) 2011 Lee Hicks <lee.lorenzo@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "sensord.h"

extern "C"
{
#include "tban.h"
#include "big_ng.h"
}


namespace rts2sensord
{

class bigNG: public Sensor
{

	public:
		bigNG (int argc, char **argv);
		virtual ~bigNG(void);

	protected:
		virtual int processOption (int in_opt);
		virtual int init ();
        virtual void beforeRun();
		virtual int info ();
        virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

        int setFanSpeed(int index, int speed);
        int setMasterFanSpeed(int speed);
        int calcSpeed(int speed);
        
	private:
		char *devFile;

		rts2core::ValueInteger *ds1;
		rts2core::ValueInteger *ds2;
		rts2core::ValueInteger *as1;
		rts2core::ValueInteger *as2;
		rts2core::ValueInteger *as3;
		rts2core::ValueInteger *as4;

        rts2core::ValueSelection *masterFanCon;
        rts2core::ValueSelection *fan1;
        rts2core::ValueSelection *fan2;
        rts2core::ValueSelection *fan3;
        rts2core::ValueSelection *fan4;

		//struct TBan * tban;
		struct TBan tban;
		unsigned char temp, rawTemp, cal;
		int ret;
};

};

using namespace rts2sensord;
bigNG::bigNG(int argc, char **argv):Sensor (argc, argv)
{
    createValue(ds1, "DS1", "Digital Sensor 1 (C)", true);
    createValue(ds2, "DS2", "Digital Sensor 2 (C)", true);
    createValue(as1, "AS1", "Analog Sensor 1 (C)", true);
    createValue(as2, "AS2", "Analog Sensor 2 (C)", true);
    createValue(as3, "AS3", "Analog Sensor 3 (C)", true);
    createValue(as4, "AS4", "Analog Sensor 4 (C)", true);

    createValue(masterFanCon, "MFanCon", "Master Fan Controll", true, RTS2_VALUE_WRITABLE);
    masterFanCon->addSelVal("OFF");
    masterFanCon->addSelVal("LOW");
    masterFanCon->addSelVal("MED");
    masterFanCon->addSelVal("HIGH");

    createValue(fan1, "FAN1", "Control for fan 1", true, RTS2_VALUE_WRITABLE);
    fan1->addSelVal("OFF");
    fan1->addSelVal("LOW");
    fan1->addSelVal("MED");
    fan1->addSelVal("HIGH");

    createValue(fan2, "FAN2", "Control for fan 2", true, RTS2_VALUE_WRITABLE);
    fan2->addSelVal("OFF");
    fan2->addSelVal("LOW");
    fan2->addSelVal("MED");
    fan2->addSelVal("HIGH");

    createValue(fan3, "FAN3", "Control for fan 3", true, RTS2_VALUE_WRITABLE);
    fan3->addSelVal("OFF");
    fan3->addSelVal("LOW");
    fan3->addSelVal("MED");
    fan3->addSelVal("HIGH");
    
    createValue(fan4, "FAN4", "Control for fan 4", true, RTS2_VALUE_WRITABLE);
    fan4->addSelVal("OFF");
    fan4->addSelVal("LOW");
    fan4->addSelVal("MED");
    fan4->addSelVal("HIGH");

    addOption ('f', NULL, 1, "Device path for bigNG");

    setIdleInfoInterval (2);
}

bigNG::~bigNG(void)
{
}

int bigNG::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			devFile = optarg;
			break;
		default:
			return Sensor::processOption (opt);
	}
	return 0;
}

int bigNG::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{

    if(old_value == masterFanCon)
    {
        setMasterFanSpeed(new_value->getValueInteger());
        return 0;
    }

    if(old_value == fan1)
    {
        setFanSpeed(0, new_value->getValueInteger());
        return 0;
    }

    if(old_value == fan2)
    {
        setFanSpeed(1, new_value->getValueInteger());
        return 0;
    }
    if(old_value == fan3)
    {
        setFanSpeed(2, new_value->getValueInteger());
        return 0;
    }

    if(old_value == fan4)
    {
        setFanSpeed(3, new_value->getValueInteger());
        return 0;
    }

    return Sensor::setValue(old_value, new_value);
}

int bigNG::init ()
{
    ret = Sensor::init ();
    if (ret)
    {
       return ret;
    }

    tban_init(&tban, devFile);
    bigNG_init(&tban);
    
    return 0;
}

int bigNG::setFanSpeed(int index, int speed)
{   //index is 0 based for fans channels and seleciton speeds

    ret = tban_setChPwm(&tban,index, calcSpeed(speed));
    logStream(MESSAGE_DEBUG) << "changing speed for fan " << index << " at speed " << speed << sendLog;
    if(ret != TBAN_OK)
    {
        logStream(MESSAGE_ERROR) << "bigNG cannot get speed for FAN " << index+1 << sendLog;
    }
    return ret;
}

int bigNG::setMasterFanSpeed(int speed)
{
    //setting the values so they reflect in the gui
    fan1->setValueInteger(speed);
    fan2->setValueInteger(speed);
    fan3->setValueInteger(speed);
    fan4->setValueInteger(speed);

    int tmpRet;
    for(int i=0;i<4;i++)
    {
        tmpRet = setFanSpeed(i, calcSpeed(speed));
        if(tmpRet != TBAN_OK)
        {
            ret = TBAN_ERROR;
        }
    }

    return ret;
}

int bigNG::calcSpeed(int speed)
{
    switch(speed)
    { // 0=OFF, 1=LOW, 2=MED, 3=HIGH
        case 0:
            speed = 0;
            break;
        case 1:
            speed = 33;
            break;
        case 2:
            speed = 66;
            break;
        case 3:
            speed = 100;
            break;
    }

    return speed;
}

void bigNG::beforeRun()
{
    Sensor::beforeRun();

	if(tban.opened == 0)
	{
	   logStream(MESSAGE_ERROR) << "bigNG device not opened, opening" << sendLog;

	   ret = tban_open(&tban);
	   if(ret != TBAN_OK)
	   {
		   logStream(MESSAGE_ERROR) << "Cannot open/init device device" << sendLog;
	   }
	}

    tban.progressCbPtr = NULL; //have to do because lame ass tban lib has random junk for pointer = segfault
    tban.progressCb = NULL;
    setMasterFanSpeed(0); //turn off all the fans on startup
}


int bigNG::info ()
{

	ret = tban_queryStatus(&tban);
	if(ret != TBAN_OK)
	{
		logStream(MESSAGE_ERROR) << "Cannot poll bigNG device " << tban_strerror(ret) << tban_strerrordesc(ret) << sendLog;
	}

	tban_getdSensorTemp(&tban, 0, &temp, &rawTemp, &cal);
	ds1->setValueInteger(temp/2);

	tban_getdSensorTemp(&tban, 1, &temp, &rawTemp, &cal);
	ds2->setValueInteger(temp/2);

    bigNG_getaSensorTemp(&tban, 0, &temp, &rawTemp, &cal,0);
    as1->setValueInteger(temp/2);

    bigNG_getaSensorTemp(&tban, 1, &temp, &rawTemp, &cal,0);
    as2->setValueInteger(temp/2);

    bigNG_getaSensorTemp(&tban, 2, &temp, &rawTemp, &cal,0);
    as3->setValueInteger(temp/2);

    bigNG_getaSensorTemp(&tban, 3, &temp, &rawTemp, &cal,0);
    as4->setValueInteger(temp/2);

	return Sensor::info ();
}

int main (int argc, char **argv)
{
	bigNG device (argc, argv);
	return device.run ();
}

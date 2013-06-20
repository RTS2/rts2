/* 
 * Class for Zolix OmniLambda 150 monochromator.
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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
#include "connection/serial.h"
#include <errno.h>

namespace rts2sensord
{

class L150:public Sensor
{
	public:
		L150 (int argc, char **argv);
		virtual ~ L150 (void);

		virtual int info ();

	protected:
		virtual int initHardware ();
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);
		virtual int processOption (int in_opt);

	private:
		rts2core::ValueString *model;
        rts2core::ValueDouble *position;
        rts2core::ValueDouble *speed;
        rts2core::ValueInteger *filter;
		rts2core::ConnSerial *L150Dev;

		void resetDevice ();
		int writePort (const char *str);
		int readPort (char *buf, int blen);

		// specialized readPort functions..
		int readPort (int &ret);
		int readPort (double &ret);

		template < typename T > int writeValue (const char *valueName, T val, rts2core::Value *value = NULL);
		template < typename T > int readValue (const char *valueName, T & val);

		int readRts2Value (const char *valueName, rts2core::Value * val);

		const char *dev;
};

}

using namespace rts2sensord;

void L150::resetDevice ()
{
	sleep (5);
	L150Dev->flushPortIO ();
	logStream (MESSAGE_DEBUG) << "Device " << dev << " reseted." << sendLog;
}

int L150::writePort (const char *str)
{
	int ret;
	ret = L150Dev->writePort (str, strlen (str));
	if (ret)
	{
		resetDevice ();
		return -1;
	}
	ret = L150Dev->writePort ("\r", 1);
	if (ret)
	{
		resetDevice ();
		return -1;
	}
	return 0;
}

int L150::readPort (char *buf, int blen)
{
	int ret;

	ret = L150Dev->readPort (buf, blen, "\r");
	if (ret == -1)
		return ret;
	return 0;
}

int L150::readPort (int &ret)
{
	int r;
	char buf[20];
	r = readPort (buf, 20);
	if (r)
		return r;
	ret = atoi (buf);
	return 0;
}

int L150::readPort (double &ret)
{
	int r;
	char buf[20];
	r = readPort (buf, 20);
	if (r)
		return r;
    logStream (MESSAGE_ERROR) << "value buffer :" << buf << sendLog;
	ret = atof (buf);
	return 0;
}

template < typename T > int L150::writeValue (const char *valueName, T val, rts2core::Value *value)
{
	int ret;
	char buf[20];
	blockExposure ();
	if (value)
		logStream (MESSAGE_INFO) << "changing " << value->getName () << " from " << value->getDisplayValue () << " to " << val << "." << sendLog;
	std::ostringstream _os;
	_os << valueName << ' ' << val;
	ret = writePort (_os.str ().c_str ());
	if (ret)
	{
		clearExposure ();
		return ret;
	}
	ret = readPort (buf, 20);
    ret = readPort (buf, 20);
	if (value)
		logStream (MESSAGE_INFO) << value->getName () << " changed from " << value->getDisplayValue () << " to " << val << "." << sendLog;
	clearExposure ();
	return ret;
}

template < typename T > int L150::readValue (const char *valueName, T & val)
{
	int ret;
	char buf[strlen (valueName + 2)];
	strcpy (buf, valueName);
	buf[strlen (valueName)] = '?';
	buf[strlen (valueName) + 1] = '\0';
	ret = writePort (buf);
	if (ret)
		return ret;
	char rbuf[strlen(valueName)+1];
	ret = readPort (rbuf, strlen(valueName)+1);
    logStream (MESSAGE_ERROR) << "rbuffer :" << rbuf << sendLog;
	ret = readPort (val);

    ret = readPort (rbuf, strlen(valueName)+1);
	return ret;
}

int L150::readRts2Value (const char *valueName, rts2core::Value * val)
{
	int ret;
	int iret;
	double dret;
	switch (val->getValueType ())
	{
		case RTS2_VALUE_INTEGER:
			ret = readValue (valueName, iret);
			((rts2core::ValueInteger *) val)->setValueInteger (iret);
			return ret;
		case RTS2_VALUE_DOUBLE:
			ret = readValue (valueName, dret);
			((rts2core::ValueDouble *) val)->setValueDouble (dret);
			return ret;
	}
	logStream (MESSAGE_ERROR) << "unsupported value type" << val->getValueType () << sendLog;
	return -1;
}

L150::L150 (int argc, char **argv):Sensor (argc, argv)
{
	L150Dev = NULL;
	dev = "/dev/ttyUSB1";

	createValue (model, "model", "monochromator model", false);
    createValue (position, "POSITION", "actual wavelenght position", true, RTS2_VALUE_WRITABLE);
    createValue (speed, "SPEED", "speed of wavelenght change [nm/s]", true, RTS2_VALUE_WRITABLE);
    createValue (filter, "FILTER", "number of filter used in monochromator", true, RTS2_VALUE_WRITABLE);
    
	addOption ('f', NULL, 1, "/dev/ttyUSBx entry (defaults to /dev/ttyUSB1");
}

L150::~L150 ()
{
	delete L150Dev;
}

int L150::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
    int ret;
    char buf[80];
    logStream (MESSAGE_ERROR) << "VALUE SETTING" << "\n" << sendLog;
    
	if (old_value == position)
	{
		ret=writeValue ("MOVETO", new_value->getValueDouble (), position);
        
        if ((new_value->getValueDouble ()>350)&&(new_value->getValueDouble () <=440)) {
            writePort ("FILTER 2");
            ret = readPort (buf, 80);
        }
        if ((new_value->getValueDouble ()>440)&&(new_value->getValueDouble () <=650)) {
            writePort ("FILTER 3");
            ret = readPort (buf, 80);
        }
        if ((new_value->getValueDouble ()>650)&&(new_value->getValueDouble () <=800)) {
            writePort ("FILTER 4");
            ret = readPort (buf, 80);
        }
        if ((new_value->getValueDouble ()>800)&&(new_value->getValueDouble () <=1100)) {
            writePort ("FILTER 5");
            ret = readPort (buf, 80);
        }
        if ((new_value->getValueDouble ()>1100)) {
            writePort ("FILTER 6");
            ret = readPort (buf, 80);
            logStream (MESSAGE_ERROR) << "Filter for wavelenght greater than 1100 [nm]--->could not be convenient for wavelenght much more greater check manual" << "\n" << sendLog;
        }
        if ((new_value->getValueDouble ()<=350)) {
            writePort ("FILTER 1");
            ret = readPort (buf, 80);
            logStream (MESSAGE_ERROR) << "No filter in filter wheel for wavelenght less than 350 [nm]--->filter wheel adjusted to empty position" << "\n" << sendLog;
        }
    }
    
    if (old_value == speed)
	{
		return writeValue ("SPEED", new_value->getValueDouble (), speed);
	}
    
    if (old_value == filter)
	{
		return writeValue ("FILTER", new_value->getValueInteger (), filter);
    }

	
			return Sensor::setValue (old_value, new_value);
}

int L150::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			dev = optarg;
			break;
		default:
			return Sensor::processOption (in_opt);
	}
	return 0;
}

int L150::initHardware ()
{
	logStream (MESSAGE_ERROR) << "INITIALIZATION" << "\n" << sendLog;
    int ret;

	L150Dev = new rts2core::ConnSerial (dev, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 200);
	L150Dev->setDebug (getDebug ());
	ret = L150Dev->init ();
	char buf[80];
		
	writePort ("Hello");
        ret = readPort (buf, 80);
      	
	writePort ("SYSTEMINFO?");
	ret = readPort (buf, 80);
	model->setValueCharArr (buf);
	ret = readPort (buf, 80);
    
    ret = readRts2Value ("POSITION", position);
    logStream (MESSAGE_ERROR) << "position: " << position->getValue() << sendLog;
    
    ret = readRts2Value ("SPEED", speed);
    logStream (MESSAGE_ERROR) << "speed: " << speed->getValue() << sendLog;
    return 0;
}

int L150::info ()
{
    logStream (MESSAGE_ERROR) << "INFO" << "\n" << sendLog;
	int ret;
    ret = readRts2Value ("POSITION", position);
	if (ret)
		return ret;
    logStream (MESSAGE_ERROR) << "position: " << position->getValue() << sendLog;
    ret = readRts2Value ("SPEED", speed);
	if (ret)
		return ret;
    logStream (MESSAGE_ERROR) << "speed: " << speed->getValue() << sendLog;
    ret = readRts2Value ("FILTER", filter);
	if (ret)
		return ret;
    logStream (MESSAGE_ERROR) << "filter: " << filter->getValue() << sendLog;
    
return Sensor::info ();
}

int main (int argc, char **argv)
{
	L150 device = L150 (argc, argv);
	return device.run ();
}

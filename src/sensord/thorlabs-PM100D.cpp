/* 
 * Driver for ThorLaser laser source.
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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
#include <errno.h>
#include "connection/connscpi.h"

namespace rts2sensord
{

/**
 * Main class for ThorLabs PM 100D power measurement unit.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class ThorPM100D:public Sensor
{
	public:
		ThorPM100D (int argc, char **argv);
		virtual ~ThorPM100D (void);
    
		virtual int info ();
    
	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);
    
	private:
		rts2core::ValueString *model;
		rts2core::ValueDouble *wavelength;
		rts2core::ValueDouble *power;
		rts2core::ConnSCPI *usbtmcConn;

		template < typename T > int writeValue (const char *valueName, T val, rts2core::Value *value = NULL);
		int readRts2Value (const char *command, rts2core::Value * val);

		const char *device_file;
};

}

using namespace rts2sensord;

template < typename T > int ThorPM100D::writeValue (const char *valueName, T val, rts2core::Value *value)
{
	blockExposure ();
	if (value)
		logStream (MESSAGE_INFO) << "changing " << value->getName () << " from " << value->getDisplayValue () << " to " << val << "." << sendLog;
	std::ostringstream _os;
	_os << valueName << ' ' << val <<"\n";
	usbtmcConn->gpibWrite (_os.str ().c_str ());
	if (value)
		logStream (MESSAGE_INFO) << value->getName () << " changed from " << value->getDisplayValue () << " to " << val << "." << sendLog;
	clearExposure ();
	return 0;
}


int ThorPM100D::readRts2Value (const char *command, rts2core::Value * val)
{
	char buf[100];
	int iret;
	double dret;
	switch (val->getValueType ())
	{
		case RTS2_VALUE_INTEGER:
			usbtmcConn->gpibWriteRead (command, buf, 100);
			iret = atoi(buf);
			((rts2core::ValueInteger *) val)->setValueInteger (iret);
			return 0;
		case RTS2_VALUE_DOUBLE:
			usbtmcConn->gpibWriteRead (command, buf, 100);
			dret = atof(buf);
			((rts2core::ValueDouble *) val)->setValueDouble (dret);
			return 0;
	}
	logStream (MESSAGE_ERROR) << "unsupported value type" << val->getValueType () << sendLog;
	return -1;
}

ThorPM100D::ThorPM100D (int argc, char **argv): Sensor (argc, argv)
{
	device_file = "/dev/fotodioda";
	usbtmcConn = NULL;

	createValue (model, "model", "photodiode sensor model", false);
	createValue (wavelength, "WAVELENGTH", "actual wavelenght position", true, RTS2_VALUE_WRITABLE);
	createValue (power, "POWER", "actual power on photodiode sensor", true, RTS2_VALUE_WRITABLE);
    
	addOption ('f', NULL, 1, "serial port with the module (ussually /dev/ttyUSB for ThorLaser USB serial connection");
}

ThorPM100D::~ThorPM100D ()
{
	delete usbtmcConn;
}

int ThorPM100D::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == wavelength)
	{
		return writeValue ("SENS:CORR:WAV", new_value->getValueDouble (), wavelength);
	}
	return Sensor::setValue (old_value, new_value);
}


int ThorPM100D::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Sensor::processOption (opt);
	}
	return 0;
}

int ThorPM100D::initHardware ()
{
	if (device_file == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify device file (TTY port)" << sendLog;
		return -1;
	}

	usbtmcConn = new rts2core::ConnSCPI (device_file);

	usbtmcConn->setDebug (getDebug ());

	usbtmcConn->initGpib ();

	int ret;
	char buf[100];
	usbtmcConn->gpibWriteRead ("*IDN?\n", buf, 100);
	logStream (MESSAGE_ERROR) <<"IDN : "<<buf << sendLog;
	model->setValueCharArr (buf);
    
	usbtmcConn->gpibWriteRead ("SYSTem:DATE?\n", buf, 100);
	logStream (MESSAGE_ERROR) <<"DATE : "<< buf << sendLog;
    
	usbtmcConn->gpibWriteRead ("SYSTEM:TIME?\n", buf, 100);
	logStream (MESSAGE_ERROR) <<"TIME : "<<buf << sendLog;
    
	ret = readRts2Value ("SENS:CORR:WAV?\n", wavelength);
	if (ret)
		return ret;
	logStream (MESSAGE_DEBUG) << "wavelength: " << wavelength->getValue() << sendLog;
    
	ret = readRts2Value ("SENS:CORR:POW?\n", power);
	if (ret)
		return ret;
	logStream (MESSAGE_DEBUG) << "power: " << power->getValue() << sendLog;

	return 0;
}

int ThorPM100D::info ()
{
	int ret;
	ret = readRts2Value ("SENS:CORR:WAV?\n", wavelength);
	if (ret)
		return ret;

	ret = readRts2Value ("SENS:CORR:POW?\n", power);
	if (ret)
		return ret;

	return Sensor::info ();
}


int main (int argc, char **argv)
{
	ThorPM100D device (argc, argv);
	return device.run ();
}

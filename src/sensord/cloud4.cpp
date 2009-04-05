/* 
 * Sensor daemon for cloudsensor (mrakomer) by Martin Kakona
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2009 Martin Jelinek
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

#include "../utils/rts2connserial.h"

/**
 * Class for cloudsensor.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2Cloud4: public Rts2DevSensor
{
	private:
		char *device_file;
		Rts2ConnSerial *mrakConn;

		Rts2ValueDoubleStat *tempDiff;
		Rts2ValueDoubleStat *tempIn;
		Rts2ValueDoubleStat *tempOut;

		Rts2ValueBool *heater;

		Rts2ValueInteger *numberMes;
		Rts2ValueInteger *mrakStatus;

		/**
		 * Read sensor values.
		 */
		int readSensor ();
	protected:
		virtual int processOption (int in_opt);
		virtual int init ();

		virtual int info ();

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

	public:
		Rts2Cloud4 (int in_argc, char **in_argv);
		virtual ~Rts2Cloud4 (void);
};

int
Rts2Cloud4::readSensor ()
{
	int ret;
	char buf[128];
	ret = mrakConn->writeRead (heater->getValueBool () ? "h" : "s", 1, buf, 50, '\r');
	if (ret < 0)
		return ret;
	buf[ret] = '\0';

	// parse response
	float temp0, temp1;
	int tno, tstat=1;
	int x = sscanf (buf+6, "%d %f %f %*d %*d %*3s", &tno, &temp0, &temp1);
	temp0/=100.0;
	temp1/=100.0;

	logStream (MESSAGE_INFO) << "MM4 reply was: " << tno
		<< " " << temp0 << " " << temp1 << " " << x << sendLog;
	if (x != 3) 
	{
		logStream (MESSAGE_ERROR) << "cannot parse reply from cloud senso, reply was: '"
			<< buf << "', return " << x << sendLog;
		return -1;
	}
	tempDiff->addValue (temp0 - temp1, 20);
	tempIn->addValue (temp0, 20);
	tempOut->addValue (temp1, 20);

	tempDiff->calculate ();
	tempIn->calculate ();
	tempOut->calculate ();

	numberMes->setValueInteger (tno);
	mrakStatus->setValueInteger (tstat);

	return 0;
}


Rts2Cloud4::Rts2Cloud4 (int in_argc, char **in_argv)
:Rts2DevSensor (in_argc, in_argv)
{
	mrakConn = NULL;

	createValue (tempDiff, "TEMP_DIFF", "temperature difference");
	createValue (tempIn, "TEMP_IN", "temperature inside", true);
	createValue (tempOut, "TEMP_OUT", "tempreature outside", true);

	createValue (heater, "HEATER", "heater state", true);

	createValue (numberMes, "number_mes", "number of measurements", false);
	createValue (mrakStatus, "status", "device status", true, RTS2_DT_HEX);

	addOption ('f', NULL, 1, "serial port with cloud sensor");
}


Rts2Cloud4::~Rts2Cloud4 (void)
{
	delete mrakConn;
}


int
Rts2Cloud4::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Rts2DevSensor::processOption (in_opt);
	}
	return 0;
}


int
Rts2Cloud4::init ()
{
	int ret;
	char buf[128];
	ret = Rts2DevSensor::init ();
	if (ret)
		return ret;

	mrakConn = new Rts2ConnSerial (device_file, this, BS2400, C8, NONE, 10);
	ret = mrakConn->init ();
	if (ret)
		return ret;
	
	mrakConn->flushPortIO ();

	ret = mrakConn->writeRead ("s", 1, buf, 50, '\r');

	return 0;
}


int
Rts2Cloud4::info ()
{
	int ret;
	ret = readSensor ();
	if (ret)
		return -1;
	return Rts2DevSensor::info ();
}


int
Rts2Cloud4::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	return Rts2DevSensor::setValue (old_value, new_value);
}


int
main (int argc, char **argv)
{
	Rts2Cloud4 device = Rts2Cloud4 (argc, argv);
	return device.run ();
}

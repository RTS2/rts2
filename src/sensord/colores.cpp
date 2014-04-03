/* 
 * Driver for B2 colores control board.
 * Copyright (C) 2011 Petr Kubanek <petr@kubanek.net>
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

namespace rts2sensord
{

/**
 * Colores board control various I/O.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Colores:public Sensor
{
	public:
		Colores (int argc, char **argv);
		virtual ~Colores ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual int info ();

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

	private:
		char *device_file;
		rts2core::ConnSerial *coloresConn;

		rts2core::ValueBool *filtA;
		rts2core::ValueBool *filtB;
		rts2core::ValueBool *filtC;

		rts2core::ValueBool *mirror;

		rts2core::ValueFloat *temp1;
		rts2core::ValueFloat *temp2;

		rts2core::ValueInteger *light1;
		rts2core::ValueInteger *light2;

		void coloresCommand (char c);
};

}

using namespace rts2sensord;

Colores::Colores (int argc, char **argv): Sensor (argc, argv)
{
	device_file = NULL;
	coloresConn = NULL;

	createValue (mirror, "mirror", "mirror position", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);

	createValue (filtA, "A", "A filter", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
	createValue (filtB, "B", "B filter", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
	createValue (filtC, "C", "C filter", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);

	createValue (temp1, "T-lamp", "Lamp temp. (C)", false);
	createValue (temp2, "T-chassis", "Chassis temp. (C)", false);

	createValue (light1, "L1", "Barrel light level", false);
	createValue (light2, "L2", "Lamp light level", false);

	addOption ('f', NULL, 1, "serial port with the module (ussually /dev/ttyUSB)");

	setIdleInfoInterval (10);
}

Colores::~Colores ()
{
	delete coloresConn;
}

int Colores::processOption (int opt)
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

int Colores::initHardware ()
{
	if (device_file == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify device file (TTY port)" << sendLog;
		return -1;
	}

	coloresConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 50);
	int ret = coloresConn->init ();
	if (ret)
		return ret;
	
	coloresConn->flushPortIO ();
	coloresConn->setDebug (true);

	// init connection
	ret = coloresConn->writePort ("\n", 1);
	if (ret < 0)
		return -1;
	sleep (10);
	coloresConn->flushPortIO ();

	filtA->setValueBool (true);
	filtB->setValueBool (true);
	filtC->setValueBool (true);
	mirror->setValueBool (true);

	return 0;
}

int Colores::info ()
{
	coloresCommand ('\n');
	return Sensor::info ();
}

int Colores::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == mirror)
	{
		coloresConn->setVTime (150);
		coloresCommand (((rts2core::ValueBool *) new_value)->getValueBool () ? 'M':'m');
		coloresConn->setVTime (50);
		return 0;
	}
	else if (old_value == filtA)
	{
		coloresCommand (((rts2core::ValueBool *) new_value)->getValueBool () ? 'A':'a');
		return 0;
	}
	else if (old_value == filtB)
	{
		coloresCommand (((rts2core::ValueBool *) new_value)->getValueBool () ? 'B':'b');
		return 0;
	}
	else if (old_value == filtC)
	{
		coloresCommand (((rts2core::ValueBool *) new_value)->getValueBool () ? 'C':'c');
		return 0;
	}
	return Sensor::setValue (old_value, new_value);
}

void Colores::coloresCommand (char c)
{
	char buf[200];
	coloresConn->writeRead (&c, 1, buf, 199, '\n');

	char fA,fB,fC;
	float t1,t2;
	int l1,l2;

	int ret = sscanf (buf, "FW1=%c,FW2=%c,FW3=%c,T1=%f,T2=%f,L1=%d,L2=%d", &fA, &fB, &fC, &t1, &t2, &l1, &l2);
	if (ret != 7)
	{
		logStream (MESSAGE_ERROR) << "cannot parse colores reply '" << buf << "', ret:" << ret << sendLog;
		return;
	}

	filtA->setValueBool (fA == '1');
	filtB->setValueBool (fB == '1');
	filtC->setValueBool (fC == '1');

	temp1->setValueFloat (t1);
	temp2->setValueFloat (t2);

	light1->setValueInteger (l1);
	light2->setValueInteger (l2);
}

int main (int argc, char **argv)
{
	Colores device (argc, argv);
	return device.run ();
}


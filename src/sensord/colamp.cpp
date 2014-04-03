/* 
 * Driver for B2 colamp control board.
 *
 * Copyright (C) 2011 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2014 Martin Jelinek <petr@kubanek.net>
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
 * Colamp board control various I/O.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Colamp:public Sensor
{
	public:
		Colamp (int argc, char **argv);
		virtual ~Colamp ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual int info ();

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

	private:
		char *device_file;
		rts2core::ConnSerial *colampConn;

		rts2core::ValueBool *lampHg;
		rts2core::ValueBool *lampKr;

		void colampCommand (char c);
};

}

using namespace rts2sensord;

Colamp::Colamp (int argc, char **argv): Sensor (argc, argv)
{
	device_file = NULL;
	colampConn = NULL;

	createValue (lampHg, "H", "Mercury-Argon Lamp", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
	createValue (lampKr, "K", "Krypton Lamp", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);

	addOption ('f', NULL, 1, "serial port with the module (usually /dev/ttyUSBn)");

	setIdleInfoInterval (10);
}

Colamp::~Colamp ()
{
	delete colampConn;
}

int Colamp::processOption (int opt)
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

int Colamp::initHardware ()
{
	if (device_file == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify device file (TTY port)" << sendLog;
		return -1;
	}

	colampConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 50);
	int ret = colampConn->init ();
	if (ret)
		return ret;
	
	colampConn->flushPortIO ();
	colampConn->setDebug (true);

	// init connection
	ret = colampConn->writePort ("p", 1);
	if (ret < 0)
		return -1;
	colampConn->flushPortIO ();

	return 0;
}

int Colamp::info ()
{
//	colampCommand ('\n');
	return Sensor::info ();
}

int Colamp::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == lampHg)
	{
		colampCommand (((rts2core::ValueBool *) new_value)->getValueBool () ? 'a':'p');
		return 0;
	}
	else if (old_value == lampKr)
	{
		colampCommand (((rts2core::ValueBool *) new_value)->getValueBool () ? 'x':'p');
		return 0;
	}

	return Sensor::setValue (old_value, new_value);
}

void Colamp::colampCommand (char c)
{
	char buf[200];
	colampConn->writeRead (&c, 1, buf, 1);

// arduino here returns what we sent him. if it is x or a, the respective lamp is on, otherwise, none is on. 

	lampHg->setValueBool (buf[0] == 'a');
	lampKr->setValueBool (buf[0] == 'x');
}

int main (int argc, char **argv)
{
	Colamp device (argc, argv);
	return device.run ();
}


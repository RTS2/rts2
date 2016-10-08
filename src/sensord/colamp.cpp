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

/* i
Lamps Controller $Revision: 3654 $
Commands: abcdefghABCDEFGHitRS
t1=a t2=b ... t8=h
a = OFF t1 / A = ON t1
i = info
t = telemetry
R = reset
S = store to EEPROM
*/

#include "sensord.h"

#include "connection/serial.h"

namespace rts2sensord
{

/**
 * Colamp board control various I/O.
 *
 * @author Petr Kubanek <petr@kubanek.net>, Martin Jelinek <mates@iaa.es>
 */
class Colamp:public Sensor
{
	public:
		Colamp (int argc, char **argv);
		virtual ~Colamp ();
		virtual int scriptEnds ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual int info ();

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

	private:
		const char *device_file;
		rts2core::ConnSerial *colampConn;

		rts2core::ValueBool *lampHg;
		rts2core::ValueBool *lampKr;
		rts2core::ValueBool *halogenPower;
		rts2core::ValueBool *focuserPower;
		rts2core::ValueBool *coloresPower;
		rts2core::ValueBool *switch6;
		rts2core::ValueBool *switch7;
		rts2core::ValueBool *switch8;

		rts2core::ValueFloat *domeTemp;

		void colampCommand (char c);
};

}

using namespace rts2sensord;

Colamp::Colamp (int argc, char **argv): Sensor (argc, argv)
{
	device_file = "/dev/collamp"; // default value 
	colampConn = NULL;

	createValue (domeTemp, "dometemp", "Dome temperature", false);

	createValue (lampHg, "H", "Mercury-Argon Lamp", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
	createValue (lampKr, "K", "Krypton Lamp", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
	createValue (halogenPower, "halogen", "halogen lamp power switch", false, RTS2_VALUE_WRITABLE |  RTS2_DT_ONOFF);
	createValue (focuserPower, "focuser", "Optec 12V power switch", false, RTS2_VALUE_WRITABLE |  RTS2_DT_ONOFF);
	createValue (coloresPower, "colores", "Colores 12V power switch", false, RTS2_VALUE_WRITABLE |  RTS2_DT_ONOFF);
	createValue (switch6, "switch6", "Unassigned switch 6", false, RTS2_VALUE_WRITABLE |  RTS2_DT_ONOFF);
	createValue (switch7, "switch7", "Unassigned switch 7", false, RTS2_VALUE_WRITABLE |  RTS2_DT_ONOFF);
	createValue (switch8, "switch8", "Unassigned switch 8", false, RTS2_VALUE_WRITABLE |  RTS2_DT_ONOFF);

	addOption ('f', NULL, 1, "serial port with the module (may be /dev/ttyUSBn, defaults to /dev/collamp)");

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

	colampCommand ('t');

	return 0;
}

int Colamp::info ()
{
	colampCommand ('t');
	return Sensor::info ();
}

int Colamp::scriptEnds ()
{
changeValue(lampHg, false);
changeValue(lampKr, false);
return 0;
}

int Colamp::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == lampHg)
	{
		colampCommand (((rts2core::ValueBool *) new_value)->getValueBool () ? 'A':'a');
		return 0;
	}
	else if (old_value == lampKr)
	{
		colampCommand (((rts2core::ValueBool *) new_value)->getValueBool () ? 'B':'b');
		return 0;
	}
	else if (old_value == halogenPower)
	{
		colampCommand (((rts2core::ValueBool *) new_value)->getValueBool () ? 'C':'c');
		return 0;
	}
	else if (old_value == focuserPower)
	{
		colampCommand (((rts2core::ValueBool *) new_value)->getValueBool () ? 'D':'d');
		return 0;
	}

	else if (old_value == coloresPower)
	{
		colampCommand (((rts2core::ValueBool *) new_value)->getValueBool () ? 'E':'e');
		return 0;
	}
	else if (old_value == switch6)
	{
		colampCommand (((rts2core::ValueBool *) new_value)->getValueBool () ? 'F':'f');
		return 0;
	}
	else if (old_value == switch7)
	{
		colampCommand (((rts2core::ValueBool *) new_value)->getValueBool () ? 'G':'g');
		return 0;
	}
	else if (old_value == switch8)
	{
		colampCommand (((rts2core::ValueBool *) new_value)->getValueBool () ? 'H':'h');
		return 0;
	}

	return Sensor::setValue (old_value, new_value);
}

void Colamp::colampCommand (char c)
{
	double dt;
	char buf[200];
	char buf2[32],buf3[32];

	colampConn->flushPortIO();
	colampConn->writeRead (&c, 1, buf, 199, '\n');

	// typical reply is: 25.6 abcDEfgh abcDEfgh\n
	// which means: temperature on the sensor, lowercase=off, uppercase=on,
	//  fisrt set of letters is current, second the default state	

	int ret = sscanf (buf, "%lf %s %s", &dt, buf2, buf3);
	if (ret != 3)
	{
		logStream (MESSAGE_WARNING) << "cannot parse collamp reply '" << buf << "', ret:" << ret << sendLog;
		return;
	}

	domeTemp->setValueFloat (dt);

	// modified logic, now obsolete, but still valid
	if(buf2[0] == 'A')
		{
		lampHg->setValueBool (buf2[1] == 'B');
		lampKr->setValueBool (buf2[1] == 'b');
		}
	else
		{
		lampHg->setValueBool (false);
		lampKr->setValueBool (false);
		}

	halogenPower->setValueBool (buf2[2] == 'C');
	focuserPower->setValueBool (buf2[3] == 'D');
	coloresPower->setValueBool (buf2[4] == 'E');
	switch6->setValueBool (buf2[5] == 'F');
	switch7->setValueBool (buf2[6] == 'G');
	switch8->setValueBool (buf2[7] == 'H');
}

int main (int argc, char **argv)
{
	Colamp device (argc, argv);
	return device.run ();
}


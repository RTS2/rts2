/**
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2005-2006 John French
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define BAUDRATE B9600
#define FOCUSER_PORT "/dev/ttyS1"

#define CMD_FOCUS_MOVE_IN     "FI"
#define CMD_FOCUS_MOVE_OUT    "FO"
#define CMD_FOCUS_GET         "FG"
#define CMD_TEMP_GET          "FT"
#define CMD_FOCUS_GOTO        "FG"

#include "focusd.h"
#include "../../lib/rts2/connserial.h"

namespace rts2focusd
{

class Robofocus:public Focusd
{
	private:
		const char *device_file;
		rts2core::ConnSerial *robofocConn;
		char checksum;
		int step_num;

		rts2core::ValueBool *switches[4];

		// high-level I/O functions
		int focus_move (const char *cmd, int steps);
		void compute_checksum (char *cmd);

		int getPos ();
		int getTemp ();
		int getSwitchState ();
		int setSwitch (int switch_num, bool new_state);

	protected:
		virtual int processOption (int in_opt);
		virtual int isFocusing ();

		virtual bool isAtStartPosition ()
		{
			return false;
		}

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);
	public:
		Robofocus (int argc, char **argv);
		~Robofocus (void);

		virtual int init ();
		virtual int initValues ();
		virtual int info ();
		virtual int setTo (float num);
		virtual float tcOffset () {return 0.;};
};

}

using namespace rts2focusd;

Robofocus::Robofocus (int argc, char **argv):Focusd (argc, argv)
{
	device_file = FOCUSER_PORT;

	for (int i = 0; i < 4; i++)
	{
		std::ostringstream _name;
		std::ostringstream _desc;
		_name << "switch_" << i+1;
		_desc << "plug number " << i+1;
		createValue (switches[i], _name.str().c_str(), _desc.str().c_str(), false, RTS2_VALUE_WRITABLE);
	}

	createTemperature ();

	addOption ('f', NULL, 1, "device file (ussualy /dev/ttySx");

}

Robofocus::~Robofocus ()
{
  	delete robofocConn;
}

int Robofocus::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Focusd::processOption (in_opt);
	}
	return 0;
}

/*!
 * Init focuser, connect on given port port, set manual regime
 *
 * @return 0 on succes, -1 & set errno otherwise
 */
int Robofocus::init ()
{
	int ret;

	ret = Focusd::init ();

	if (ret)
		return ret;

	robofocConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 40);
	ret = robofocConn->init ();
	if (ret)
		return ret;

	robofocConn->flushPortIO ();

	// turn paramount on
	ret = setSwitch (1, true);
	if (ret)
		return -1;

	ret = setSwitch (2, false);
	if (ret)
	  	return -1;

	return 0;
}

int Robofocus::initValues ()
{
	focType = std::string ("ROBOFOCUS");
	return Focusd::initValues ();
}

int Robofocus::info ()
{
	getPos ();
	getTemp ();
	// querry for switch state
	int swstate = getSwitchState ();
	if (swstate >= 0)
	{
		for (int i = 0; i < 4; i++)
		{
			switches[i]->setValueBool (swstate & (1 << i));
		}
	}
	return Focusd::info ();
}

int Robofocus::getPos ()
{
	char command[10], rbuf[10];
	char command_buffer[9];

	// Put together command with neccessary checksum
	sprintf (command_buffer, "FG000000");
	compute_checksum (command_buffer);

	sprintf (command, "%s%c", command_buffer, checksum);

	if (robofocConn->writeRead (command, 9, rbuf, 9) != 9)
		return -1;
	position->setValueInteger (atoi (rbuf + 2));
	return 0;
}

int Robofocus::getTemp ()
{
	char command[10], rbuf[10];
	char command_buffer[9];

	// Put together command with neccessary checksum
	sprintf (command_buffer, "FT000000");
	compute_checksum (command_buffer);

	sprintf (command, "%s%c", command_buffer, checksum);

	if (robofocConn->writeRead (command, 9, rbuf, 9) != 9)
		return -1;
								 // return temp in Celsius
	temperature->setValueFloat ((atof (rbuf + 2) / 2) - 273.15);
	return 0;
}

int Robofocus::getSwitchState ()
{
	char command[10], rbuf[10];
	char command_buffer[9];
	int ret;

	sprintf (command_buffer, "FP000000");
	compute_checksum (command_buffer);

	sprintf (command, "%s%c", command_buffer, checksum);
	if (robofocConn->writeRead (command, 9, rbuf, 9) != 9)
		return -1;
	ret = 0;
	for (int i = 0; i < 4; i++)
	{
		if (rbuf[i + 4] == '2')
			ret |= (1 << i);
	}
	return ret;
}

int Robofocus::setTo (float num)
{
	char command[9], command_buf[10];
	sprintf (command, "FG%06i", (int) num);
	compute_checksum (command);
	sprintf (command_buf, "%s%c", command, checksum);
	if (robofocConn->writePort (command_buf, 9))
		return -1;
	return 0;
}

int Robofocus::setSwitch (int switch_num, bool new_state)
{
	char command[10], rbuf[10];
	char command_buffer[9] = "FP001111";
	if (switch_num >= 4)
	{
		return -1;
	}
	int swstate = getSwitchState ();
	if (swstate < 0)
		return -1;
	for (int i = 0; i < 4; i++)
	{
		if (swstate & (1 << i))
			command_buffer[i + 4] = '2';
	}
	command_buffer[switch_num + 4] = (new_state ? '2' : '1');
	compute_checksum (command_buffer);
	sprintf (command, "%s%c", command_buffer, checksum);

	if (robofocConn->writeRead (command, 9, rbuf, 9) != 9)
		return -1;

	infoAll ();
	return 0;
}

int Robofocus::focus_move (const char *cmd, int steps)
{
	char command[10];
	char command_buffer[9];

	// Number of steps moved must account for backlash compensation
	//  if (strcmp (cmd, CMD_FOCUS_MOVE_OUT) == 0)
	//    num_steps = steps + 40;
	//  else
	//    num_steps = steps;

	// Pad out command with leading zeros
	sprintf (command_buffer, "%s%06i", cmd, steps);

	//Get checksum character
	compute_checksum (command_buffer);

	sprintf (command, "%s%c", command_buffer, checksum);

	// Send command to focuser

	if (robofocConn->writePort (command, 9))
		return -1;

	step_num = steps;

	return 0;
}

int Robofocus::isFocusing ()
{
	char rbuf[10];
	int ret;
	ret = robofocConn->readPort (rbuf, 1);
	if (ret == -1)
		return ret;
	// if we get F, read out end command
	if (*rbuf == 'F')
	{
		ret = robofocConn->readPort (rbuf + 1, 8);
		usleep (USEC_SEC/10);
		if (ret != 8)
			return -1;
		return -2;
	}
	return 0;
}

int Robofocus::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	for (int i = 0; i < 4; i++)
	{
		if (switches[i] == oldValue)
		{
			return setSwitch (i, ((rts2core::ValueBool *) newValue)->getValueBool ()) == 0 ? 0 : -2;
		}
	}
	return Focusd::setValue (oldValue, newValue);
}

// Calculate checksum (according to RoboFocus spec.)
void Robofocus::compute_checksum (char *cmd)
{
	int bytesum = 0;
	unsigned int size, i;

	size = strlen (cmd);

	for (i = 0; i < size; i++)
		bytesum = bytesum + (int) (cmd[i]);

	checksum = toascii ((bytesum % 340));
}

int main (int argc, char **argv)
{
	Robofocus device (argc, argv);
	return device.run ();
}

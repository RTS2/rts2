/* 
 * Driver for Ford boards.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#include "domeford.h"

#define CAS_NA_OTEVRENI              20

typedef enum
{
	ZASUVKA_1, ZASUVKA_2, ZASUVKA_3, ZASUVKA_4, ZASUVKA_5, ZASUVKA_6, MOTOR,
	SMER, SVETLO, KONCAK_ZAVRENI_JIH, KONCAK_OTEVRENI_JIH,
	KONCAK_OTEVRENI_SEVER, KONCAK_ZAVRENI_SEVER
} vystupy;

#define NUM_ZAS   6

#define OFF   0
#define STANDBY   1
#define OBSERVING 2

// zasuvka c.1 kamera (NF), c.2 kamera (WF), c.5 montaz. c.6 topeni /Torman 18.2.2013
int zasuvky_index[NUM_ZAS] =
{ ZASUVKA_1, ZASUVKA_2, ZASUVKA_3, ZASUVKA_4, ZASUVKA_5, ZASUVKA_6 };

enum stavy
{ ZAS_VYP, ZAS_ZAP };

// prvni index - cislo stavu (0 - off, 1 - standby, 2 - observing), druhy je stav zasuvky cislo j)
enum stavy zasuvky_stavy[3][NUM_ZAS] =
{
	// off
	{ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_VYP},
	// standby
	{ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_VYP},
	// observnig
	{ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP}
};

using namespace rts2dome;


namespace rts2dome
{

/**
 * Class for BART dome control.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Bart:public Ford
{
	private:
		unsigned char spinac[2];

		int handle_zasuvky (int zas);

		time_t domeTimeout;

		rts2core::ValueInteger *sw_state;
		rts2core::ValueBool *socket1, *socket2, *socket3, *socket4, *socket5, *socket6;
		rts2core::ValueBool *domeMotorPower;
		rts2core::ValueSelection *domeMotorDirection, *domeSouthSwitchOpened, *domeSouthSwitchClosed;

	protected:
		virtual int init ();


		virtual int info ();

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();
		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();

		virtual int observing ();
		virtual int standby ();
		virtual int off ();

	public:
		Bart (int argc, char **argv);
		virtual ~Bart ();
};

}

Bart::Bart (int argc, char **argv):Ford (argc, argv)
{
	domeTimeout = 0;

	createValue (sw_state, "sw_state", "state of dome switches", false, RTS2_DT_HEX);
	createValue (domeMotorPower, "motor_power", "power to dome-motor", false, RTS2_DT_ONOFF);
	createValue (domeMotorDirection, "motor_direction", "dome-motor direction", false, RTS2_DT_ONOFF);
	domeMotorDirection->addSelVal ("opening (off)");
	domeMotorDirection->addSelVal ("closing (on)");
	createValue (domeSouthSwitchOpened, "south_roof_opened", "switch on south roof, opened state", false, RTS2_DT_ONOFF);
	domeSouthSwitchOpened->addSelVal ("---");
	domeSouthSwitchOpened->addSelVal ("OPENED");
	createValue (domeSouthSwitchClosed, "south_roof_closed", "switch on south roof, closed state", false, RTS2_DT_ONOFF);
	domeSouthSwitchClosed->addSelVal ("---");
	domeSouthSwitchClosed->addSelVal ("CLOSED");
	createValue (socket1, "socket1", "wall socket #1 (camera C1)", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	createValue (socket2, "socket2", "wall socket #2 (camera C2)", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	createValue (socket3, "socket3", "wall socket #3 (camera C3)", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	createValue (socket4, "socket4", "wall socket #4", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	createValue (socket5, "socket5", "wall socket #5 (mount T0)", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	createValue (socket6, "socket6", "wall socket #6 (anti-dew heating of NF & WF)", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
}

Bart::~Bart ()
{
}

int Bart::startOpen ()
{
	if (!isOn (KONCAK_OTEVRENI_JIH))
		return endOpen ();
	VYP (MOTOR);
	sleep (1);
	VYP (SMER);
	sleep (1);
	ZAP (MOTOR);
	logStream (MESSAGE_DEBUG) << "oteviram strechu" << sendLog;

	time (&domeTimeout);
	domeTimeout += CAS_NA_OTEVRENI;

	return 0;
}

long Bart::isOpened ()
{
	int ret;
	ret = zjisti_stav_portu ();
	if (domeTimeout != 0 && domeTimeout < time (NULL))
	{
		logStream (MESSAGE_ERROR) << "Timeout reached during dome opening, dome is regarded as opened" << sendLog;
		return -2;
	}
	if (ret)
		return ret;
	if (isOn (KONCAK_OTEVRENI_JIH))
		return USEC_SEC;
	return -2;
}

int Bart::endOpen ()
{
	VYP (MOTOR);
	zjisti_stav_portu ();		 //kdyz se to vynecha, neposle to posledni prikaz nebo znak
	setTimeout (USEC_SEC);
	domeTimeout = 0;
	return 0;
}

int Bart::startClose ()
{
	int motor;
	int smer;
	if (!isOn (KONCAK_ZAVRENI_JIH))
		return endClose ();
	motor = isOn (MOTOR);
	smer = isOn (SMER);
	if (motor == -1 || smer == -1)
	{
		// errror
		return -1;
	}
	if (!motor)
	{
		// closing in progress
		if (!smer)
			return 0;
		// let it go to open state and then close it..
		return -1;
	}
	ZAP (SMER);
	sleep (1);
	ZAP (MOTOR);
	logStream (MESSAGE_DEBUG) << "zaviram strechu" << sendLog;

	time (&domeTimeout);
	domeTimeout += CAS_NA_OTEVRENI;

	return 0;
}

long Bart::isClosed ()
{
	int ret;

	if (domeTimeout != 0 && domeTimeout < time (NULL))
	{
		logStream (MESSAGE_ERROR) << "Timeout reached during dome closing. Aborting dome closing." << sendLog;
		return -2;
	}

	ret = zjisti_stav_portu ();
	if (ret)
		return ret;
	if (isOn (KONCAK_ZAVRENI_JIH))
		return USEC_SEC;
	return -2;
}

int Bart::endClose ()
{
	int motor;
	motor = isOn (MOTOR);
	domeTimeout = 0;
	if (motor == -1)
		return -1;
	if (motor)
		return 0;
	VYP (MOTOR);
	sleep (1);
	VYP (SMER);
	zjisti_stav_portu ();
	return 0;
}

int Bart::init ()
{
	int ret = Ford::init ();
	if (ret)
		return ret;


	// get state
	ret = zjisti_stav_portu ();
	if (ret)
		return -1;

	if (isOn (KONCAK_OTEVRENI_JIH) && !isOn (KONCAK_ZAVRENI_JIH))
		maskState (DOME_DOME_MASK, DOME_CLOSED, "dome is closed");
	else if (!isOn (KONCAK_OTEVRENI_JIH) && isOn (KONCAK_ZAVRENI_JIH))
		maskState (DOME_DOME_MASK, DOME_OPENED, "dome is opened");

	return 0;
}

int Bart::handle_zasuvky (int zas)
{
	int i;
	for (i = 0; i < NUM_ZAS; i++)
	{
		int zasuvka_num = zasuvky_index[i];
		if (zasuvky_stavy[zas][i] == ZAS_VYP)
		{
			ZAP (zasuvka_num);
		}
		else
		{
			VYP (zasuvka_num);
		}
		sleep (1);				 // doplnil Ford
	}
	return 0;
}

int Bart::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == socket1)
	{
		if (((rts2core::ValueBool* )new_value)->getValueBool () == true)
		{
			return VYP(ZASUVKA_1) == 0 ? 0 : -2;
		}
		else
		{
			return ZAP(ZASUVKA_1) == 0 ? 0 : -2;
		}
	}
	if (old_value == socket2)
	{
		if (((rts2core::ValueBool* )new_value)->getValueBool () == true)
		{
			return VYP(ZASUVKA_2) == 0 ? 0 : -2;
		}
		else
		{
			return ZAP(ZASUVKA_2) == 0 ? 0 : -2;
		}
	}
	if (old_value == socket3)
	{
		if (((rts2core::ValueBool* )new_value)->getValueBool () == true)
		{
			return VYP(ZASUVKA_3) == 0 ? 0 : -2;
		}
		else
		{
			return ZAP(ZASUVKA_3) == 0 ? 0 : -2;
		}
	}
	if (old_value == socket4)
	{
		if (((rts2core::ValueBool* )new_value)->getValueBool () == true)
		{
			return VYP(ZASUVKA_4) == 0 ? 0 : -2;
		}
		else
		{
			return ZAP(ZASUVKA_4) == 0 ? 0 : -2;
		}
	}
	if (old_value == socket5)
	{
		if (((rts2core::ValueBool* )new_value)->getValueBool () == true)
		{
			return VYP(ZASUVKA_5) == 0 ? 0 : -2;
		}
		else
		{
			return ZAP(ZASUVKA_5) == 0 ? 0 : -2;
		}
	}
	if (old_value == socket6)
	{
		if (((rts2core::ValueBool* )new_value)->getValueBool () == true)
		{
			return VYP(ZASUVKA_6) == 0 ? 0 : -2;
		}
		else
		{
			return ZAP(ZASUVKA_6) == 0 ? 0 : -2;
		}
	}
	return Ford::setValue (old_value, new_value);
}

int Bart::info ()
{
	int ret;
	ret = zjisti_stav_portu ();
	if (ret)
		return -1;
	sw_state->setValueInteger (!getPortState (KONCAK_OTEVRENI_JIH));
	sw_state->setValueInteger (sw_state->getValueInteger () | (!getPortState (SMER) << 1));
	sw_state->setValueInteger (sw_state->getValueInteger () | (!getPortState (KONCAK_ZAVRENI_JIH) << 2));
	sw_state->setValueInteger (sw_state->getValueInteger () | (!getPortState (MOTOR) << 3));
	domeMotorPower->setValueBool (getPortState (MOTOR));
	domeMotorDirection->setValueInteger (getPortState (SMER));
	domeSouthSwitchOpened->setValueInteger (getPortState (KONCAK_OTEVRENI_JIH));
	domeSouthSwitchClosed->setValueInteger (getPortState (KONCAK_ZAVRENI_JIH));
	socket1->setValueBool (!getPortState (ZASUVKA_1));
	socket2->setValueBool (!getPortState (ZASUVKA_2));
	socket3->setValueBool (!getPortState (ZASUVKA_3));
	socket4->setValueBool (!getPortState (ZASUVKA_4));
	socket5->setValueBool (!getPortState (ZASUVKA_5));
	socket6->setValueBool (!getPortState (ZASUVKA_6));
	return Ford::info ();
}

int Bart::off ()
{
	Ford::off ();
	return handle_zasuvky (OFF);
}

int Bart::standby ()
{
	Ford::standby ();
	return handle_zasuvky (STANDBY);
}

int Bart::observing ()
{
	handle_zasuvky (OBSERVING);
	return Ford::observing ();
}

int main (int argc, char **argv)
{
	Bart device (argc, argv);
	return device.run ();
}

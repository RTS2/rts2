/* 
 * Driver for board for FRAM telescope, Pierre-Auger observatory, Argentina
 * Copyright (C) 2005-2010 Petr Kubanek <petr@kubanek.net>
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

#include "connection/ford.h"
#include "sensord.h"


#define OPT_WDC_DEV		OPT_LOCAL + 130
#define OPT_WDC_TIMEOUT		OPT_LOCAL + 131
#define OPT_EXTRA_SWITCH	OPT_LOCAL + 132

typedef enum
{
	SWITCH_BATBACK,
	SPINAC_2,
	SPINAC_3,
	SPINAC_4,
	PLUG_4,
	PLUG_PARA,
	PLUG_PHOTOMETER,
	PLUG_1
} extraOuts;

namespace rts2sensord
{

class Fram:public Sensor
{
	private:
		rts2core::ConnSerial *wdcConn;
		char *wdc_file;

		rts2core::FordConn *extraSwitch;
		
		char *extraSwitchFile;

		rts2core::ValueDouble *wdcTimeOut;
		rts2core::ValueDouble *wdcTemperature;

		rts2core::ValueBool *switchBatBack;
		rts2core::ValueBool *plug_para;
		rts2core::ValueBool *plug_photometer;
		rts2core::ValueBool *plug1;
		rts2core::ValueBool *plug4;

		int openWDC ();
		void closeWDC ();

		int resetWDC ();
		int getWDCTimeOut ();
		int setWDCTimeOut (int on, double timeout);
		int getWDCTemp (int id);

		int setValueSwitch (int sw, bool new_state);

	protected:
		virtual int processOption (int in_opt);

		virtual int init ();
		virtual int idle ();

		virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

	public:
		Fram (int argc, char **argv);
		virtual ~Fram (void);
		virtual int info ();
};

}

using namespace rts2sensord;

int Fram::openWDC ()
{
	int ret;
	wdcConn = new rts2core::ConnSerial (wdc_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 100);
	ret = wdcConn->init ();
	if (ret)
		return ret;

	return setWDCTimeOut (1, wdcTimeOut->getValueDouble ());
}

void Fram::closeWDC ()
{
	setWDCTimeOut (1, 120.0);
	delete wdcConn;
}

int Fram::resetWDC ()
{
	wdcConn->writePort ("~**\r", 4);
	wdcConn->flushPortO ();
	return 0;
}

int Fram::getWDCTimeOut ()
{
	int i, r, t;
	char q;
	char reply[128];

	wdcConn->writePort ("~012\r", 5); /*,i,q */;
	wdcConn->flushPortO ();

	q = i = t = 0;
	do
	{
		r = wdcConn->readPort (q);
		if (r == 1)
			reply[i++] = q;
		else
		{
			t++;
			if (t > 10)
				break;
		}
	}
	while (q > 20);

	reply[i] = 0;

	if (reply[0] == '!')
		logStream (MESSAGE_ERROR) << "Fram::getWDCTimeOut reply " << reply + 1 << sendLog;

	return 0;

}

int Fram::setWDCTimeOut (int on, double timeout)
{
	int i, r, t, timeo;
	char q;
	char reply[128];

	timeo = (int) (timeout / 0.03);
	if (timeo > 0xffff)
		timeo = 0xffff;
	if (timeo < 0)
		timeo = 0;
	if (on)
		i = '1';
	else
		i = '0';

	t = sprintf (reply, "~013%c%04X\r", i, timeo);
	wdcConn->writePort (reply, t);
	wdcConn->flushPortO ();

	q = i = t = 0;
	do
	{
		r = wdcConn->readPort (q);
		if (r == 1)
			reply[i++] = q;
		else
		{
			t++;
			if (t > 10)
				break;
		}
	}
	while (q > 20);

	logStream (MESSAGE_DEBUG) << "Fram::setWDCTimeOut on: " << on << " timeout: " << timeout <<
		" q: " << q << sendLog;

	reply[i] = 0;

	if (reply[0] == '!')
		logStream (MESSAGE_DEBUG) << "Fram::setWDCTimeOut reply: " <<
			reply + 1 << sendLog;

	return 0;
}

int Fram::getWDCTemp (int id)
{
	int i, r, t;
	char q;
	char reply[128];

	if ((id < 0) || (id > 2))
		return -1;

	t = sprintf (reply, "~017%X\r", id + 5);
	wdcConn->writePort (reply, t);
	wdcConn->flushPortO ();

	q = i = t = 0;
	do
	{
		r = wdcConn->readPort (q);
		if (r == 1)
			reply[i++] = q;
		else
		{
			t++;
			if (t > 10)
				return -1;
		}
	}
	while (q > 20);

	reply[i] = 0;

	if (reply[0] != '!')
		return -1;

	i = atoi (reply + 1);

	return i;
}

void Fram::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
{

	if (!(new_state & SERVERD_ONOFF_MASK))
	{
		switch (new_state & SERVERD_STATUS_MASK)
		{
			case SERVERD_DUSK:
			case SERVERD_NIGHT:
			case SERVERD_DAWN:
				if (extraSwitch)
					extraSwitch->VYP (SWITCH_BATBACK);
				Sensor::changeMasterState (old_state, new_state);
				return;
		}
	}
	if (extraSwitch)
		extraSwitch->ZAP (SWITCH_BATBACK);
	Sensor::changeMasterState (old_state, new_state);
}

int Fram::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_WDC_DEV:
			wdc_file = optarg;
			break;
		case OPT_WDC_TIMEOUT:
			wdcTimeOut->setValueDouble (atof (optarg));
			break;
		case OPT_EXTRA_SWITCH:
			extraSwitchFile = optarg;
			break;
		default:
			return Sensor::processOption (in_opt);
	}
	return 0;
}

int Fram::init ()
{
	int ret = Sensor::init ();
	if (ret)
		return ret;

	if (extraSwitchFile)
	{
		extraSwitch = new rts2core::FordConn (extraSwitchFile, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 40);
		ret = extraSwitch->init ();
		if (ret)
			return ret;

		extraSwitch->VYP (SWITCH_BATBACK);

		createValue (switchBatBack, "bat_backup", "state of batery backup switch", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
		switchBatBack->setValueBool (false);

		createValue (plug1, "plug_1", "1st plug", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);

		createValue (plug_photometer, "plug_photometer", "photometer plug", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
		extraSwitch->ZAP (PLUG_PHOTOMETER);
		plug_photometer->setValueBool (true);
		createValue (plug_para, "plug_para", "paramount plug", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
		extraSwitch->ZAP (PLUG_PARA);
		plug_para->setValueBool (true);

		createValue (plug4, "plug_4", "4th plug", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
	}
	if (wdc_file)
	{
		ret = openWDC ();
		if (ret)
			return ret;
	}

	return 0;
}

int Fram::idle ()
{
	// resetWDC
	if (wdc_file)
		resetWDC ();
	return Sensor::idle ();
}

int Fram::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == switchBatBack)
	{
		return setValueSwitch (SWITCH_BATBACK, ((rts2core::ValueBool *) newValue)->getValueBool ());
	}
	if (oldValue == plug_photometer)
	{
		return setValueSwitch (PLUG_PHOTOMETER, ((rts2core::ValueBool *) newValue)->getValueBool ());
	}
	if (oldValue == plug_para)
	{
		return setValueSwitch (PLUG_PARA, ((rts2core::ValueBool *) newValue)->getValueBool ());
	}
	if (oldValue == plug1)
	{
		return setValueSwitch (PLUG_1, ((rts2core::ValueBool *) newValue)->getValueBool ());
	}
	if (oldValue == plug4)
	{
		return setValueSwitch (PLUG_4, ((rts2core::ValueBool *) newValue)->getValueBool ());
	}
	return Sensor::setValue (oldValue, newValue);
}

int Fram::setValueSwitch (int sw, bool new_state)
{
	if (new_state == true)
	{
		return extraSwitch->ZAP (sw) == 0 ? 0 : -2;
	}
	return extraSwitch->VYP (sw) == 0 ? 0 : -2;
}

Fram::Fram (int argc, char **argv):Sensor (argc, argv)
{
	createValue (wdcTimeOut, "watchdog_timeout", "timeout of the watchdog card (in seconds)", false);
	wdcTimeOut->setValueDouble (30.0);

	createValue (wdcTemperature, "watchdog_temp1", "first temperature of the watchedog card", false);

	wdc_file = NULL;
	wdcConn = NULL;

	extraSwitchFile = NULL;
	extraSwitch = NULL;

	plug_para = NULL;
	plug_photometer = NULL;
	plug1 = NULL;
	plug4 = NULL;

	addOption (OPT_WDC_DEV, "wdc-dev", 1, "/dev file with watch-dog card");
	addOption (OPT_WDC_TIMEOUT, "wdc-timeout", 1, "WDC timeout (default to 30 seconds)");
	addOption (OPT_EXTRA_SWITCH, "extra-switch", 1, "/dev entery for extra switches, handling baterry etc..");
}

Fram::~Fram (void)
{
	if (extraSwitch)
		extraSwitch->ZAP (SWITCH_BATBACK);
	if (wdc_file)
		closeWDC ();
}

int Fram::info ()
{
	if (wdcConn > 0)
	{
	  	wdcTemperature->setValueDouble (getWDCTemp (2));
	}
	return Sensor::info ();
}

int main (int argc, char **argv)
{
	Fram device = Fram (argc, argv);
	return device.run ();
}

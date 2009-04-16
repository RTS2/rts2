/* 
 * OpenTpl focuser control.
 * Copyright (C) 2005-2007 Stanislav Vitek
 * Copyright (C) 2005-2009 Petr Kubanek <petr@kubanek.net>
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

#define DEBUG_EXTRA

#include "../utils/connopentpl.h"
#include "../utils/rts2config.h"

#include "focuser.h"

class Rts2DevFocuserOpenTpl:public Rts2DevFocuser
{
	private:
		HostString *openTPLServer;

		rts2core::OpenTpl *opentplConn;

		Rts2ValueInteger *realPos;

		int initOpenTplDevice ();

	protected:
		virtual int endFocusing ();
		virtual bool isAtStartPosition ();
	public:
		Rts2DevFocuserOpenTpl (int argc, char **argv);
		virtual ~ Rts2DevFocuserOpenTpl (void);
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int initValues ();
		virtual int info ();
		virtual int setTo (int num);
		virtual int isFocusing ();
};


Rts2DevFocuserOpenTpl::Rts2DevFocuserOpenTpl (int in_argc, char **in_argv):Rts2DevFocuser (in_argc,
in_argv)
{
	openTPLServer = NULL;
	opentplConn = NULL;

	createValue (realPos, "FOC_REAL", "real position of the focuser", true, 0, 0, false);

	addOption (OPT_OPENTPL_SERVER, "opentpl", 1, "OpenTPL server TCP/IP address and port (separated by :)");
}


Rts2DevFocuserOpenTpl::~Rts2DevFocuserOpenTpl ()
{
	delete opentplConn;
}


int
Rts2DevFocuserOpenTpl::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_OPENTPL_SERVER:
			openTPLServer = new HostString (optarg, "65432");
			break;
		default:
			return Rts2DevFocuser::processOption (in_opt);
	}
	return 0;
}


int
Rts2DevFocuserOpenTpl::initOpenTplDevice ()
{
	std::string ir_ip;
	int ir_port = 0;
	if (openTPLServer)
	{
		ir_ip = openTPLServer->getHostname ();
		ir_port = openTPLServer->getPort ();
	}
	else
	{
		Rts2Config *config = Rts2Config::instance ();
		config->loadFile (NULL);
		// try to get default from config file
		config->getString ("ir", "ip", ir_ip);
		config->getInteger ("ir", "port", ir_port);
	}

	if (ir_ip.length () == 0 || !ir_port)
	{
		std::cerr << "Invalid port or IP address of mount controller PC"
			<< std::endl;
		return -1;
	}

	try
	{	
		opentplConn = new rts2core::OpenTpl (this, ir_ip, ir_port);
		opentplConn->init ();
	}
	catch (rts2core::ConnError er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}

	return 0;
}


int
Rts2DevFocuserOpenTpl::init ()
{
	int ret;
	ret = Rts2DevFocuser::init ();
	if (ret)
		return ret;

	ret = initOpenTplDevice ();
	if (ret)
		return ret;

	return checkStartPosition ();
}


int
Rts2DevFocuserOpenTpl::initValues ()
{
	focType = std::string ("FIR");
	return Rts2DevFocuser::initValues ();
}


int
Rts2DevFocuserOpenTpl::info ()
{
	int status = 0;

	double f_realPos;
	status = opentplConn->get ("FOCUS.REALPOS", f_realPos, &status);
	if (status)
		return -1;

	realPos->setValueInteger ((int) (f_realPos * 1000.0));

	return Rts2DevFocuser::info ();
}


int
Rts2DevFocuserOpenTpl::setTo (int num)
{
	int status = 0;

	int power = 1;
	int referenced = 0;
	double offset;

	status = opentplConn->get ("FOCUS.REFERENCED", referenced, &status);
	if (referenced != 1)
	{
		logStream (MESSAGE_ERROR) << "setTo referenced is: " <<
			referenced << sendLog;
		return -1;
	}
	status = opentplConn->setww ("FOCUS.POWER", power, &status);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "setTo cannot set POWER to 1"
			<< sendLog;
	}
	offset = (double) num / 1000.0;
	status = opentplConn->setww ("FOCUS.TARGETPOS", offset, &status);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "setTo cannot set offset!" <<
			sendLog;
		return -1;
	}
	setFocusTimeout (100);
	focPos->setValueInteger (num);
	return 0;
}


int
Rts2DevFocuserOpenTpl::isFocusing ()
{
	double targetdistance;
	int status = 0;
	status = opentplConn->get ("FOCUS.TARGETDISTANCE", targetdistance, &status);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "focuser IR isFocusing status: " << status
			<< sendLog;
		return -1;
	}
	return (fabs (targetdistance) < 0.001) ? -2 : USEC_SEC / 50;
}


int
Rts2DevFocuserOpenTpl::endFocusing ()
{
	int status = 0;
	int power = 0;
	status = opentplConn->setww ("FOCUS.POWER", power, &status);
	if (status)
	{
		logStream (MESSAGE_ERROR) <<
			"focuser IR endFocusing cannot set POWER to 0" << sendLog;
		return -1;
	}
	return 0;
}


bool
Rts2DevFocuserOpenTpl::isAtStartPosition ()
{
	int ret;
	ret = info ();
	if (ret)
		return false;
	return (fabs ((float) getFocPos () - 15200 ) < 100);
}


int
main (int argc, char **argv)
{
	Rts2DevFocuserOpenTpl device = Rts2DevFocuserOpenTpl (argc, argv);
	return device.run ();
}

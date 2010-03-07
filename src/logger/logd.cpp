/* 
 * Log daemon.
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

#include "rts2loggerbase.h"
#include "../utils/rts2device.h"

class Rts2Logd:public Rts2Device, public Rts2LoggerBase
{
	public:
		Rts2Logd (int in_argc, char **in_argv);
		virtual rts2core::Rts2DevClient *createOtherType (Rts2Conn * conn, int other_device_type);
	protected:
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
		virtual int processArgs (const char *arg);
		virtual int willConnect (Rts2Address * in_addr);
	private:
		Rts2ValueString * logConfig;
		Rts2ValueString *logFile;
		int setLogConfig (const char *new_config);
		int setLogFile (const char *new_file);
};

Rts2Logd::Rts2Logd (int in_argc, char **in_argv):Rts2Device (in_argc, in_argv, DEVICE_TYPE_LOGD, "LOGD")
{
	setTimeout (USEC_SEC);

	addOption ('c', NULL, 1,
		"specify config file with logged device, timeouts and values");
	addOption ('o', NULL, 1, "output log file expression");

	createValue (logConfig, "config", "logging configuration file", false, RTS2_VALUE_WRITABLE);
	createValue (logFile, "output", "logging file", false, RTS2_VALUE_WRITABLE);
}

int Rts2Logd::setLogConfig (const char *new_config)
{
	std::ifstream * istream = new std::ifstream (new_config);
	int ret = readDevices (*istream);
	delete istream;
	if (ret)
		return ret;
	setLogFile (logFile->getValue ());
	return ret;
}


int
Rts2Logd::setLogFile (const char *new_file)
{
	postEvent (new Rts2Event (EVENT_SET_LOGFILE, (void *) new_file));
	return 0;
}


int
Rts2Logd::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'c':
			logConfig->setValueCharArr (optarg);
			return 0;
		case 'o':
			logFile->setValueCharArr (optarg);
			return 0;
	}
	return Rts2Device::processOption (in_opt);
}


int
Rts2Logd::init ()
{
	int ret;
	ret = Rts2Device::init ();
	if (ret)
		return ret;
	if (logConfig->getValue () && *logConfig->getValue () != '\n')
		return setLogConfig (logConfig->getValue ());
	return 0;
}

int Rts2Logd::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == logConfig)
	{
		return (setLogConfig (new_value->getValue ()) == 0) ? 0 : -2;
	}
	if (old_value == logFile)
	{
		return (setLogFile (new_value->getValue ()) == 0) ? 0 : -2;
	}
	return Rts2Device::setValue (old_value, new_value);
}

int Rts2Logd::processArgs (const char *arg)
{
	return setLogConfig (arg);
}

int Rts2Logd::willConnect (Rts2Address * in_addr)
{
	return Rts2LoggerBase::willConnect (in_addr);
}

rts2core::Rts2DevClient *Rts2Logd::createOtherType (Rts2Conn * conn, int other_device_type)
{
	rts2core::Rts2DevClient *cli = Rts2LoggerBase::createOtherType (conn, other_device_type);
	if (cli)
	{
		cli->postEvent (new Rts2Event (EVENT_SET_LOGFILE, (void*) logFile->getValue ()));
		return cli;
	}
	return Rts2Device::createOtherType (conn, other_device_type);
}

int main (int argc, char **argv)
{
	Rts2Logd logd (argc, argv);
	return logd.run ();
}

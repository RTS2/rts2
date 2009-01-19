/* 
 * Simple script executor.
 * Copyright (C) 2007 Petr Kubanek <petr@kubanek.net>
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

#include "scriptexec.h"
#include "rts2execcli.h"
#include "rts2devcliphot.h"

#include "../utils/rts2config.cpp"

#include <iostream>
#include <fstream>
#include <vector>

// Rts2ScriptExec class

int
Rts2ScriptExec::findScript (std::string in_deviceName, std::string & buf)
{
	// take script from stdin
	if (deviceName)
	{
		scripts.push_back (new Rts2ScriptForDeviceStream (std::string (deviceName), &std::cin));
		deviceName = NULL;
	}
	for (std::vector < Rts2ScriptForDevice * >::iterator iter = scripts.begin ();
		iter != scripts.end (); iter++)
	{
		Rts2ScriptForDevice *ds = *iter;
		if (ds->isDevice (in_deviceName))
		{
			if (!nextRunningQ)
			{
				time (&nextRunningQ);
				nextRunningQ += 5;
			}
			return ds->getScript (buf);
		}
	}
	return -1;
}


bool
Rts2ScriptExec::isScriptRunning ()
{
	int runningScripts = 0;
	postEvent (new Rts2Event (EVENT_SCRIPT_RUNNING_QUESTION, (void *) &runningScripts));
	if (runningScripts > 0)
		return true;
	// if there are some images which need to be written
	postEvent (new Rts2Event (EVENT_NUMBER_OF_IMAGES, (void *)&runningScripts));
	if (runningScripts > 0)
		return true;
	// if we still have some commands in que, wait till they finish
	return (!commandQueEmpty ());
}


int
Rts2ScriptExec::processOption (int in_opt)
{
	std::ifstream * is;
	switch (in_opt)
	{
		case OPT_CONFIG:
			configFile = optarg;
			break;
		case 'd':
			deviceName = optarg;
			break;
		case 's':
			if (!deviceName)
			{
				std::cerr << "unknow device name" << std::endl;
				return -1;
			}
			scripts.push_back (new Rts2ScriptForDevice (std::string (deviceName), std::string (optarg)));
			deviceName = NULL;
			break;
		case 'f':
			if (!deviceName)
			{
				std::cerr << "unknow device name" << std::endl;
				return -1;
			}
			is = new std::ifstream (optarg);
			scripts.push_back (new Rts2ScriptForDeviceStream (std::string (deviceName), is));
			deviceName = NULL;
			break;
		case 'e':
			expandPath = new Rts2ValueString ("expand_path");
			expandPath->setValueString (optarg);
			break;
		default:
			return Rts2Client::processOption (in_opt);
	}
	return 0;
}


Rts2ScriptExec::Rts2ScriptExec (int in_argc, char **in_argv)
:Rts2Client (in_argc, in_argv), Rts2ScriptInterface ()
{
	waitState = 0;
	currentTarget = NULL;
	nextRunningQ = 0;
	configFile = NULL;
	expandPath = NULL;

	addOption (OPT_CONFIG, "config", 1, "configuration file");

	addOption ('d', NULL, 1, "name of next script device");
	addOption ('s', NULL, 1, "device script (for device specified with d)");
	addOption ('f', NULL, 1, "script filename");

	addOption ('e', NULL, 1, "filename expand string");
}


Rts2ScriptExec::~Rts2ScriptExec (void)
{
	for (std::vector < Rts2ScriptForDevice * >::iterator iter = scripts.begin ();
		iter != scripts.end (); iter++)
	{
		delete *iter;
	}
	scripts.clear ();
}


int
Rts2ScriptExec::init ()
{
	int ret;
	ret = Rts2Client::init ();
	if (ret)
		return ret;

	// load config..

	Rts2Config *config = Rts2Config::instance ();
	ret = config->loadFile (configFile);
	if (ret)
		return ret;

	// create current target
	currentTarget = new Rts2TargetScr (this);

	return 0;
}


int
Rts2ScriptExec::doProcessing ()
{
	return 0;
}


Rts2DevClient *
Rts2ScriptExec::createOtherType (Rts2Conn * conn, int other_device_type)
{
	Rts2DevClient *cli;
	switch (other_device_type)
	{
		case DEVICE_TYPE_MOUNT:
			cli = new Rts2DevClientTelescopeExec (conn);
			break;
		case DEVICE_TYPE_CCD:
			cli = new Rts2DevClientCameraExec (conn, expandPath);
			break;
		case DEVICE_TYPE_FOCUS:
			cli = new Rts2DevClientFocusImage (conn);
			break;
			/*    case DEVICE_TYPE_PHOT:
				  cli = new Rts2DevClientPhotExec (conn);
				  break; */
		case DEVICE_TYPE_MIRROR:
			cli = new Rts2DevClientMirrorExec (conn);
			break;
		case DEVICE_TYPE_DOME:
		case DEVICE_TYPE_SENSOR:
			cli = new Rts2DevClientWriteImage (conn);
			break;
		default:
			cli = Rts2Client::createOtherType (conn, other_device_type);
	}
	return cli;
}


void
Rts2ScriptExec::postEvent (Rts2Event * event)
{
	switch (event->getType ())
	{
		case EVENT_SCRIPT_ENDED:
			break;
		case EVENT_MOVE_OK:
			if (waitState)
			{
				postEvent (new Rts2Event (EVENT_CLEAR_WAIT));
				break;
			}
			//      postEvent (new Rts2Event (EVENT_OBSERVE));
			break;
		case EVENT_MOVE_FAILED:
			//endRunLoop ();
			break;
		case EVENT_ENTER_WAIT:
			waitState = 1;
			break;
		case EVENT_CLEAR_WAIT:
			waitState = 0;
			break;
		case EVENT_GET_ACQUIRE_STATE:
			*((int *) event->getArg ()) = 1;
			//      (currentTarget) ? currentTarget->getAcquired () : -2;
			break;
	}
	Rts2Client::postEvent (event);
}


void
Rts2ScriptExec::deviceReady (Rts2Conn * conn)
{
	conn->postEvent (new Rts2Event (EVENT_SET_TARGET, (void *) currentTarget));
	conn->postEvent (new Rts2Event (EVENT_OBSERVE));
}


int
Rts2ScriptExec::idle ()
{
	if (nextRunningQ != 0)
	{
		time_t now;
		time (&now);
		if (nextRunningQ < now)
		{
			if (!isScriptRunning ())
				endRunLoop ();
			nextRunningQ = now + 5;
		}
	}
	return Rts2Client::idle ();
}


void
Rts2ScriptExec::deviceIdle (Rts2Conn * conn)
{
	if (!isScriptRunning ())
		endRunLoop ();
}


void
Rts2ScriptExec::getPosition (struct ln_equ_posn *pos, double JD)
{
	pos->ra = 20;
	pos->dec = 20;
}


int
main (int argc, char **argv)
{
	Rts2ScriptExec app = Rts2ScriptExec (argc, argv);
	return app.run ();
}

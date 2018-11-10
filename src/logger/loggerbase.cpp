/* 
 * Logger base.
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

#include "loggerbase.h"

using namespace rts2logd;

DevClientLogger::DevClientLogger (rts2core::Connection * in_conn, double in_numberSec, time_t in_fileCreationInterval, std::list < std::string > &in_logNames):rts2core::DevClient (in_conn)
{
	exp = NULL;

	gettimeofday (&nextInfoCall, NULL);
	numberSec.tv_sec = (int) (floor (in_numberSec));
	numberSec.tv_usec = (int) (USEC_SEC * (in_numberSec - floor (in_numberSec)));

	time(&nextFileCreationCheck);
	fileCreationInterval = in_fileCreationInterval;

	logNames = in_logNames;

	outputStream = &std::cout;
}

DevClientLogger::~DevClientLogger (void)
{
	if (outputStream != &std::cout)
		delete outputStream;
	delete exp;
}

void DevClientLogger::fillLogValues ()
{
	for (std::list < std::string >::iterator iter = logNames.begin (); iter != logNames.end (); iter++)
	{
		rts2core::Value *val = getConnection ()->getValue ((*iter).c_str ());
		if (val)
		{
			logValues.push_back (val);
		}
		else
		{
			logStream (MESSAGE_ERROR) << "Cannot find value '" << *iter << "' of device '" << getName () << "', exiting." << sendLog;
			getMaster ()->endRunLoop ();
		}
	}
}

void DevClientLogger::setOutputFile (const char *pattern)
{
	if (exp == NULL)
	{
		exp = new rts2core::Expander ();
	}
	expandPattern = std::string (pattern);
	changeOutputStream ();
}

void DevClientLogger::changeOutputStream ()
{
	struct timeval tv;
	getConnection ()->getInfoTime (tv);
	if (tv.tv_sec < nextFileCreationCheck)
		return;
	exp->setExpandDate (&tv, false);
	std::string expanded = exp->expand (expandPattern);
	// if filename was not changed
	if (expanded == expandedFilename)
		return;
	expandedFilename = expanded;
	std::ofstream * nstream = new std::ofstream (expandedFilename.c_str(), std::ios_base::app);
	if (nstream->fail ())
	{
		delete nstream;
		return;
	}
	if (outputStream != &std::cout)
		delete outputStream;
	
	outputStream = nstream;
}

void DevClientLogger::infoOK ()
{
	if (logValues.empty ())
		fillLogValues ();
	// check if we have to change log file..
	changeOutputStream ();
	*outputStream << getName ();
	for (std::list < rts2core::Value * >::iterator iter = logValues.begin (); iter != logValues.end (); iter++)
	{
		*outputStream << " " << rts2core::getDisplayValue (*iter);
	}
	*outputStream << std::endl;
}

void DevClientLogger::infoFailed ()
{
 	changeOutputStream ();
	*outputStream << "info failed" << std::endl;
}

void DevClientLogger::idle ()
{
	struct timeval now;
	gettimeofday (&now, NULL);
	if (timercmp (&nextInfoCall, &now, <))
	{
		queCommand (new rts2core::CommandInfo (getMaster ()));
		timeradd (&now, &numberSec, &nextInfoCall);
	}
}

void DevClientLogger::postEvent (rts2core::Event * event)
{
	switch (event->getType ())
	{
		case EVENT_SET_LOGFILE:
			setOutputFile ((const char *) event->getArg ());
			break;
	}
	rts2core::DevClient::postEvent (event);
}

LoggerBase::LoggerBase ()
{
}

int LoggerBase::readDevices (std::istream & is)
{
	while (!is.eof () && !is.fail ())
	{
		std::string devName;
		double timeout;
		std::string values;
		std::list < std::string > valueList;
		valueList.clear ();

		is >> devName;
		if (is.eof ())
			return 0;

		is >> timeout;

		if (is.eof () || is.fail ())
		{
			logStream (MESSAGE_ERROR) << "Cannot read device name or timeout, please correct line " <<
				sendLog;
			return -1;
		}
		getline (is, values);
		// split values..
		std::istringstream is2 (values);
		while (true)
		{
			std::string value;
			is2 >> value;
			if (is2.fail ())
				break;
			valueList.push_back (std::string (value));
		}
		if (valueList.empty ())
		{
			logStream (MESSAGE_ERROR) << "Value list for device " << devName << " is empty, please correct that." << sendLog;
			return -1;
		}
		devicesNames.push_back (LogValName (devName, timeout, valueList));
	}
	return 0;
}

LogValName * LoggerBase::getLogVal (const char *name)
{
	for (std::list < LogValName >::iterator iter = devicesNames.begin ();
		iter != devicesNames.end (); iter++)
	{
		if ((*iter).devName == name)
			return &(*iter);
	}
	return NULL;
}

int LoggerBase::willConnect (rts2core::NetworkAddress * in_addr)
{
	if (getLogVal (in_addr->getName ()))
		return 1;
	return 0;
}

rts2core::DevClient * LoggerBase::createOtherType (rts2core::Connection * conn, __attribute__ ((unused)) int other_device_type)
{
	LogValName *val = getLogVal (conn->getName ());
	if (val)
		return new DevClientLogger (conn, val->timeout, 60, val->valueList);
	return NULL;
}

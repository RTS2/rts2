/* 
 * System sensor, displaying free disk space and more.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#include <sys/types.h>
#include <sys/statvfs.h>
#include <stdlib.h>
#include <iostream>
#include "configuration.h"

#define OPT_STORAGE         OPT_LOCAL + 601

#define EVENT_STORE_PATHS   RTS2_LOCAL_EVENT + 1320
#define EVENT_REFRESH_LOAD  RTS2_LOCAL_EVENT + 1321

namespace rts2sensord
{

/**
 * This class is for sensor which displays informations about
 * system states.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class System:public Sensor
{
	public:
		System (int in_argc, char **in_argv);

		virtual void postEvent (rts2core::Event *event);
	protected:
		virtual int processOption (int opt);
		virtual int init ();
		virtual int info ();
	private:
		rts2core::ValueTime *lastWrite;
		rts2core::DoubleArray *loadavg;
		rts2core::ValueString *path;
		rts2core::ValueDouble *freeSize;
		rts2core::ValueDoubleStat *history;
		rts2core::ValueFloat *nfree;
		rts2core::ValueLong *bytesNight;

		void addHistoryValue (double val);

		const char *storageFile;
		void storePaths ();
		void loadPaths ();
		void scheduleStore ();
		void refreshLoad ();
};

};

using namespace rts2sensord;

int System::processOption (int opt)
{
	switch (opt)
	{
		case 'p':
			path->setValueCharArr (optarg);
			return 0;
		case OPT_STORAGE:
			storageFile = optarg;
			return 0;
		default:
			return Sensor::processOption (opt);
	}
	return 0;
}

int System::init ()
{
	int ret = Sensor::init ();
	if (ret)
		return ret;
	if (path->getValueString ().length () == 0)
	{
		logStream (MESSAGE_ERROR) << "you must specify -p parameter for path which will be checked" << sendLog;
		return -1;
	}
	if (storageFile)
	{
		loadPaths ();
		// calculate next local midday
		scheduleStore ();
	}
	refreshLoad ();
	return 0;
}

int System::info ()
{
	struct statvfs sf;
	if (statvfs (path->getValue (), &sf))
	{
		logStream (MESSAGE_ERROR) << "cannot call stat on " << path->getValue () << ". Error " << strerror (errno) << sendLog;
	}
	else
	{
		freeSize->setValueDouble ((long double) sf.f_bavail * sf.f_bsize);	
	}
	return Sensor::info ();
}

void System::addHistoryValue (double val)
{
	history->addValue (val);
	history->calculate ();
	// recalculate _expected
	bytesNight->setValueLong (-1);
	std::deque <double>::iterator iter = history->valueBegin ();
	if (iter != history->valueEnd ())
	{
		double hist = *iter;
		iter++;
		for (; iter != history->valueEnd (); iter++)
		{
			if (hist - *iter > bytesNight->getValueLong ())
				bytesNight->setValueLong (hist - *iter);
			hist = *iter;
		}
	}
	if (bytesNight->getValueLong () > 0)
	{
		nfree->setValueFloat (val / bytesNight->getValueLong ());
		sendValueAll (nfree);
	}
	sendValueAll (bytesNight);
}

void System::storePaths ()
{
	lastWrite->setValueDouble (getNow ());
	// open stream..
	std::ofstream os (storageFile);
	if (os.fail ())
	{
		logStream (MESSAGE_ERROR) << "failed to write to storage file " << storageFile << sendLog;
		return;
	}
	os.setf (std::ios_base::fixed, std::ios_base::floatfield);
	os << lastWrite->getValueDouble () << std::endl;
	os << path->getValue ();
	addHistoryValue (freeSize->getValueDouble ());
	for (std::deque <double>::iterator miter = history->valueBegin (); miter != history->valueEnd (); miter++)
	{
		os << " " << *miter;
	}
	os << std::endl;
	os.close ();
	sendValueAll (lastWrite);
}

void System::loadPaths ()
{
	std::ifstream is (storageFile);
	double dv;
	is >> dv;
	if (is.fail ())
	{
		logStream (MESSAGE_ERROR) << "Cannot read last storage date from " << storageFile << sendLog;
		return;
	}
	lastWrite->setValueDouble (dv);
	is.ignore (2000, '\n');
	while (!is.fail ())
	{
		std::string ipath;
		is >> ipath;
		if (is.fail ())
			return;
		if (ipath != path->getValue ())
		{
			logStream (MESSAGE_ERROR) << "Invalid path value specified in history file: " << ipath << " expected " << path->getValue () << sendLog;
		}
		std::string line;
		getline (is, line);
		std::istringstream iss (line);
		while (true)
		{
			iss >> dv;
			if (iss.fail ())
				break;
			addHistoryValue (dv);
		}
	}
	is.close ();	
}

void System::scheduleStore ()
{
	rts2core::Configuration *config = rts2core::Configuration::instance ();
	time_t t = config->getNight () - time (NULL);
	if (t < 20)
		t += 86400;
	addTimer (t, new rts2core::Event (EVENT_STORE_PATHS));
}

void System::refreshLoad ()
{
	double loads[3];
	int count = getloadavg(loads, 3);
	loadavg->setValueArray (std::vector<double>(loads, loads + count));
	addTimer(20, new rts2core::Event (EVENT_REFRESH_LOAD));
}

System::System (int argc, char **argv):Sensor (argc, argv)
{
	storageFile = NULL;

	createValue (path, "path", "path being monitored", false);
	createValue (freeSize, "free", "free disk space on ", false, RTS2_DT_BYTESIZE);
	createValue (history, "history", "history of free bytes (every 24h)", false); // , RTS2_DT_BYTESIZE);
	createValue (nfree, "night", "number of expected nights till disk will get full", false);
	createValue (bytesNight, "expected", "expected number of bytes per night (maximum from night diferences from last 10 nights)", false, RTS2_DT_BYTESIZE);

	createValue (lastWrite, "last_write", "time of last write of system usage statistics", false);
	createValue (loadavg, "load_avg", "system load average for 1, 5 and 15 minutes", false);
	addOption ('p', NULL, 1, "specifyed path being monitored");
	addOption (OPT_STORAGE, "save", 1, "save data to given file");

	setIdleInfoInterval(20);
}

void System::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
		case EVENT_STORE_PATHS:
			storePaths ();
			scheduleStore ();
			break;
		case EVENT_REFRESH_LOAD:
			refreshLoad ();
			break;
	}
	Sensor::postEvent (event);
}

int main (int argc, char **argv)
{
	System device (argc, argv);
	return device.run ();
}

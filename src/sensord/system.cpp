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
#include <iostream>
#include "../utils/rts2config.h"

#define OPT_STORAGE         OPT_LOCAL + 601

#define EVENT_STORE_PATHS   RTS2_LOCAL_EVENT + 1320

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

		virtual void postEvent (Rts2Event *event);
	protected:
		virtual int processOption (int opt);
		virtual int init ();
		virtual int info ();
	private:
		std::vector <std::string> paths;
		Rts2ValueTime *lastWrite;

		int addPath (const char *path);

		const char *storageFile;
		void storePaths ();
		void loadPaths ();
		void scheduleStore ();
};

};

using namespace rts2sensord;

int System::processOption (int opt)
{
	switch (opt)
	{
		case 'p':
			return addPath (optarg);
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
	if (storageFile)
	{
		loadPaths ();
		// calculate next local midday
		scheduleStore ();
	}
	return 0;
}

int System::info ()
{
	for (std::vector <std::string>::iterator iter = paths.begin (); iter != paths.end (); iter++)
	{
		struct statvfs sf;
		if (statvfs ((*iter).c_str (), &sf))
		{
			logStream (MESSAGE_ERROR) << "Cannot get status for " << (*iter) << ". Error " << strerror (errno) << sendLog;
		}
		else
		{
			Rts2Value *val = getValue ((*iter).c_str ());
			if (val)
				((Rts2ValueDouble *) val)->setValueDouble ((long double) sf.f_bavail * sf.f_bsize);	
		}
	}
	return Sensor::info ();
}

int System::addPath (const char *path)
{
	Rts2ValueDouble *val;
	Rts2ValueDoubleStat *da;
	Rts2ValueFloat *nfree;
	createValue (val, path, (std::string ("free disk space on ") + std::string (path)).c_str (), false, RTS2_DT_BYTESIZE);
	createValue (da, (std::string (path) + "_history").c_str (), "history of free bytes (every 24h)", false); // , RTS2_DT_BYTESIZE);
	createValue (nfree, (std::string (path) + "_expected").c_str (), "number of expected nights till disk will get full", false);
	paths.push_back (path);

	return 0;
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
	for (std::vector <std::string>::iterator iter = paths.begin (); iter != paths.end (); iter++)
	{
		os << *iter;
		Rts2ValueDouble *dv = (Rts2ValueDouble*) getValue (iter->c_str ());
		Rts2ValueDoubleStat *ds = (Rts2ValueDoubleStat*) getValue ((*iter + "_history").c_str ());
		ds->addValue (dv->getValueDouble ());
		ds->calculate ();
		for (std::deque <double>::iterator miter = ds->valueBegin (); miter != ds->valueEnd (); miter++)
		{
			os << " " << *miter;
		}
		os << std::endl;
	}
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
		Rts2ValueDoubleStat *ds = (Rts2ValueDoubleStat*) getValue ((ipath + "_history").c_str ());
		if (ds == NULL || !(ds->getValueType () & (RTS2_VALUE_STAT | RTS2_VALUE_DOUBLE)))
		{
			logStream (MESSAGE_ERROR) << "Cannot get variable for path " << ipath << sendLog;
			is.ignore (2000, '\n');
			continue;
		}
		std::string line;
		getline (is, line);
		std::istringstream iss (line);
		while (!iss.fail ())
		{
			iss >> dv;
			std::cout << dv << std::endl;
			ds->addValue (dv);
		}
	}
	is.close ();	
}

void System::scheduleStore ()
{
	Rts2Config *config = Rts2Config::instance ();
	time_t t = config->getNight () - time (NULL);
	if (t < 20)
		t += 86400;
	addTimer (t, new Rts2Event (EVENT_STORE_PATHS));
}

System::System (int argc, char **argv):Sensor (argc, argv)
{
	storageFile = NULL;

	createValue (lastWrite, "last_write", "time of last write of system usage statistics", false);

	addOption ('p', NULL, 1, "add this path to paths being monitored");
	addOption (OPT_STORAGE, "save", 1, "save data to given file");

	setIdleInfoInterval (300);
}

void System::postEvent (Rts2Event *event)
{
	switch (event->getType ())
	{
		case EVENT_STORE_PATHS:
			storePaths ();
			scheduleStore ();
			break;
	}
}

int main (int argc, char **argv)
{
	System device (argc, argv);
	return device.run ();
}

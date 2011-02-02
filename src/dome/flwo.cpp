/* 
 * Driver for FLWO copula, using commands to open and close
 * Copyright (C) 2010 Petr Kubanek <kubanek@fzu.cz> Institute of Physics, Czech Republic
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

#include "dome.h"
#include "../utils/connfork.h"

#define OPT_OPEN     OPT_LOCAL + 620
#define OPT_CLOSE    OPT_LOCAL + 621
#define OPT_SLITST   OPT_LOCAL + 622
#define OPT_COVERST  OPT_LOCAL + 623

using namespace rts2dome;

class FLWO:public Dome
{
	public:
		FLWO (int argc, char **argv);

		virtual int changeMasterState (int new_state);

	protected:
		virtual int processOption (int opt);
		virtual int init ();
		virtual int info ();

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();
		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();

		virtual int deleteConnection (Rts2Conn * conn);
	private:
		rts2core::ConnFork *domeExe;

		rts2core::ValueBool *openInOn;
		rts2core::ValueBool *coverOpened;

		bool shouldClose;

		const char *opendome;
		const char *closedome;
		const char *slitfile;
		const char *coverfile;

		int getFileStatus (const char *fn);

		void getSlitStatus ();
		void getCoverStatus ();
};

FLWO::FLWO (int argc, char **argv):Dome (argc, argv)
{
	domeExe = NULL;
	shouldClose = false;

	opendome = "/usr/local/bin/openslit";
	closedome = "/usr/local/bin/closeslit";
	slitfile = "/Realtime/lib/slit_status";
	coverfile = "/Realtime/lib/cover_status";

	createValue (coverOpened, "cover_opened", "telescope cover status", false);
	createValue (openInOn, "open_when_on", "open dome if state is on", false, RTS2_VALUE_WRITABLE);
	openInOn->setValueBool (false);

	addOption (OPT_OPEN, "bin-open", 1, "path to program for opening the dome");
	addOption (OPT_CLOSE, "bin-close", 1, "path to program to close the dome");
	addOption (OPT_SLITST, "slit-file", 1, "path to slit status file");
	addOption (OPT_COVERST, "cover-file", 1, "path to cover state file");
}

int FLWO::changeMasterState (int new_state)
{
	// do not open dome if open
	// close dome if not switching to night
	if (openInOn->getValueBool () == false && (new_state & SERVERD_STANDBY_MASK) != SERVERD_STANDBY && ((new_state & SERVERD_STATUS_MASK) == SERVERD_DUSK || (new_state & SERVERD_STATUS_MASK) == SERVERD_NIGHT || (new_state & SERVERD_STATUS_MASK) == SERVERD_DAWN))
		return rts2core::Device::changeMasterState (new_state);
	return Dome::changeMasterState (new_state);
}

int FLWO::processOption (int opt)
{
	switch (opt)
	{
		case OPT_OPEN:
			opendome = optarg;
			break;
		case OPT_CLOSE:
			closedome = optarg;
			break;
		case OPT_SLITST:
			slitfile = optarg;
			break;
		case OPT_COVERST:
			coverfile = optarg;
			break;	
		default:
			return Dome::processOption (opt);
	}
	return 0;
}

int FLWO::init ()
{
	int ret = Dome::init ();
	if (ret)
		return ret;
	if (closedome == NULL)
	{
		logStream (MESSAGE_ERROR) << "missing required --bin-close option, please specify it" << sendLog;
		return -1;
	}
	if (opendome == NULL)
	{
		logStream (MESSAGE_ERROR) << "missing required --bin-open option, please specify it" << sendLog;
		return -1;
	}
	if (slitfile == NULL)
	{
		logStream (MESSAGE_ERROR) << "missing required --slit-file option, please specify it" << sendLog;
		return -1;
	}
	if (coverfile == NULL)
	{
		logStream (MESSAGE_ERROR) << "missing required --cover-file option, please specify it" << sendLog;
		return -1;
	}
	getSlitStatus ();
	getCoverStatus ();
	setIdleInfoInterval (20);
	return 0;
}

int FLWO::info ()
{
	try
	{
	  	if ((getState () & DOME_DOME_MASK) == DOME_CLOSED || (getState () & DOME_DOME_MASK) == DOME_OPENED)
			getSlitStatus ();
		getCoverStatus ();	
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return Dome::info ();
}

int FLWO::startOpen ()
{
	if (domeExe)
		return -1;
	domeExe = new rts2core::ConnFork (this, opendome, false, false);
	int ret = domeExe->init ();
	if (ret)
		return ret;
	addConnection (domeExe);
	return 0;
}

long FLWO::isOpened ()
{
	if (domeExe)
		return USEC_SEC;
	if ((getState () & DOME_DOME_MASK) == DOME_OPENING)
		return -2;
	return -1;
}

int FLWO::endOpen ()
{
	if (shouldClose)
	{
		startClose ();
		shouldClose = false;
	}
	return 0;
}

int FLWO::startClose ()
{
	if (domeExe)
	{
		if ((getState () & DOME_DOME_MASK) == DOME_CLOSING)
			return 0;
		shouldClose = true;
		return -1;
	}
	domeExe = new rts2core::ConnFork (this, closedome, false, false);
	int ret = domeExe->init ();
	if (ret)
		return ret;
	addConnection (domeExe);
	return 0;
}

long FLWO::isClosed ()
{
	if (domeExe)
		return USEC_SEC;
	if ((getState () & DOME_DOME_MASK) == DOME_CLOSING || (getState () & DOME_DOME_MASK) == DOME_CLOSED)
		return -2;
	return -1;
}

int FLWO::endClose ()
{
	return 0;
}

int FLWO::deleteConnection (Rts2Conn * conn)
{
	if (conn == domeExe)
		domeExe = NULL;
	return Dome::deleteConnection (conn);
}

int FLWO::getFileStatus (const char *fn)
{
	if (fn == NULL)
		throw rts2core::Error ("status file not specified");
	std::ifstream sf (fn);
	if (sf.fail ())
	{
		throw rts2core::Error (std::string ("cannot open status file ") + strerror (errno));
	}
	std::string slst;
	sf >> slst;
	if (sf.fail ())
	  	// assume file is empty = closed
		return 0;
	else if (slst == "Open")
		return 1;
	else if (slst == "Close")
	  	return -1;
	else
		throw rts2core::Error (std::string ("unknow string in status file ") + fn + " " + slst);
}

void FLWO::getSlitStatus ()
{
  	int ret = getFileStatus (slitfile);
	if (ret <= 0)
	  	// assume file is empty = closed
		maskState (DOME_DOME_MASK, DOME_CLOSED, "dome detected closed");
	else
		maskState (DOME_DOME_MASK, DOME_OPENED, "dome detected opened");
}


void FLWO::getCoverStatus ()
{
	int ret = getFileStatus (coverfile);  
	if (ret <= 0)
		coverOpened->setValueBool (false);
	else
		coverOpened->setValueBool (true);  	  
}

int main (int argc, char **argv)
{
	FLWO device (argc, argv);
	return device.run ();
}

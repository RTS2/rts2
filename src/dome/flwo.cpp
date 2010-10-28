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

using namespace rts2dome;

class FLWO:public Dome
{
	public:
		FLWO (int argc, char **argv);

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

		bool shouldClose;

		const char *opendome;
		const char *closedome;
		const char *slitfile;

		void getSlitStatus ();
};

FLWO::FLWO (int argc, char **argv):Dome (argc, argv)
{
	domeExe = NULL;
	shouldClose = false;

	opendome = NULL;
	closedome = NULL;
	slitfile = NULL;

	addOption (OPT_OPEN, "bin-open", 1, "path to program for opening the dome");
	addOption (OPT_CLOSE, "bin-close", 1, "path to program to close the dome");
	addOption (OPT_SLITST, "slit-file", 1, "path to slit file (/Rea...)");
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
	if (opendome == NULL || closedome == NULL)
	{
		logStream (MESSAGE_ERROR) << "binaries to open / close dome not specified, exiting" << sendLog;
		return -1;
	}
	return 0;
}

int FLWO::info ()
{
	try
	{
		getSlitStatus ();
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
	if ((getState () & DOME_DOME_MASK) == DOME_CLOSING)
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

void FLWO::getSlitStatus ()
{
	if (slitfile == NULL)
		throw rts2core::Error ("slitfile not specified");
	std::ifstream sf (slitfile);
	std::string slst;
	sf >> slst;
	if (sf.fail ())
	{
		throw rts2core::Error (std::string ("cannot read from slitfile") + strerror (errno));
	}
	if (slst == "Open")
		maskState (DOME_DOME_MASK, DOME_OPENED, "dome detected opened");
	else if (slst == "Close")
		maskState (DOME_DOME_MASK, DOME_CLOSED, "dome detected closed");
	else
		throw rts2core::Error (std::string ("unknow string in slitfile") + slst);
}

int main (int argc, char **argv)
{
	FLWO device (argc, argv);
	return device.run ();
}

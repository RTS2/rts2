/* 
 * Store images from RTS2.
 * Copyright (C) 2004-2007 Petr Kubanek <petr@kubanek.net>
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

/*!
 * Command-line fits writer.
 * $Id$
 *
 * Client to test camera deamon.
 *
 * @author petr
 */

#include "genfoc.h"

#include <vector>
#include <iostream>

class Rts2focusc:public Rts2GenFocClient
{
	private:
		exposureType exposureT;
	protected:
		virtual Rts2GenFocCamera * createFocCamera (Rts2Conn * conn);
		virtual void help ();
	public:
		Rts2focusc (int argc, char **argv);

		virtual int processOption (int in_opt);

		exposureType getExposureType ()
		{
			return exposureT;
		}
};

class Rts2focuscCamera:public Rts2GenFocCamera
{
	private:
		Rts2focusc * master;
	protected:
		virtual void queExposure ();
	public:
		Rts2focuscCamera (Rts2Conn * in_connection, Rts2focusc * in_master);

		virtual void postEvent (Rts2Event * event);
};

Rts2GenFocCamera *
Rts2focusc::createFocCamera (Rts2Conn * conn)
{
	return new Rts2focuscCamera (conn, this);
}


void
Rts2focusc::help ()
{
	Rts2GenFocClient::help ();
	std::cout << "Examples:" << std::endl
		<<
		"\trts2-focusc -d C0 -d C1 -e 20 .. takes 20 sec exposures on devices C0 and C1"
		<< std::endl;
}


Rts2focusc::Rts2focusc (int in_argc, char **in_argv):
Rts2GenFocClient (in_argc, in_argv)
{
	exposureT = EXP_LIGHT;
	autoSave = 1;

	addOption ('s', "secmod", 1, "exposure every UT sec");
	addOption ('a', "dark", 0, "create dark images");
}


int
Rts2focusc::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'a':
			exposureT = EXP_DARK;
			break;
		default:
			return Rts2GenFocClient::processOption (in_opt);
	}
	return 0;
}


Rts2focuscCamera::Rts2focuscCamera (Rts2Conn * in_connection, Rts2focusc * in_master):Rts2GenFocCamera
(in_connection,
in_master)
{
	master = in_master;
}


void
Rts2focuscCamera::queExposure ()
{
	exposureT = master->getExposureType ();
	Rts2GenFocCamera::queExposure ();
}


void
Rts2focuscCamera::postEvent (Rts2Event * event)
{
	switch (event->getType ())
	{
		case EVENT_START_EXPOSURE:
			Rts2GenFocCamera::postEvent (event);
			if (connection->havePriority ())
				queExposure ();
			return;
	}
	Rts2GenFocCamera::postEvent (event);
}


int
main (int argc, char **argv)
{
	Rts2focusc client = Rts2focusc (argc, argv);
	return client.run ();
}

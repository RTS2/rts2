/* 
 * Talker to log RTS2 message to console
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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

#include "client.h"

#include <algorithm>
#include <list>

namespace rts2mon
{
/**
 * Class to deliver messages on console.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Talker:public rts2core::Client
{
	public:
		Talker (int argc, char **argv);
		
		virtual void message (rts2core::Message & msg);
	protected:
		virtual int processOption (int opt);
		virtual int processArgs (const char *arg);
		virtual int init ();
		virtual void usage ();
	private:
		int messageMask;
		std::list <std::string> devices;
};
}

using namespace rts2mon;

Talker::Talker (int argc, char **argv):rts2core::Client (argc, argv, "talker")
{
	messageMask = MESSAGE_INFO;

	addOption ('a', NULL, 0, "print all messages");
	addOption ('e', NULL, 0, "print only error message");
}

void Talker::message (rts2core::Message & msg)
{
	if (devices.size () == 0 || std::find (devices.begin (), devices.end (),  std::string (msg.getMessageOName ())) != devices.end ())
		std::cout << msg << std::endl;
}

int Talker::processOption (int opt)
{
	switch (opt)
	{
		case 'a':
			messageMask = MESSAGE_MASK_ALL;
			break;
		case 'e':
			messageMask = MESSAGE_ERROR | MESSAGE_CRITICAL;
			break;
		default:
			return rts2core::Client::processOption (opt);
	}
	return 0;
}

int Talker::processArgs (const char *arg)
{
	devices.push_back (std::string (arg));
	return 0;
}

int Talker::init ()
{
	int ret = rts2core::Client::init ();
	if (ret)
		return ret;

	setMessageMask (messageMask);

	return 0;
}

void Talker::usage ()
{
	std::cout << "  " << getAppName () << " -a C0                .. list all messages for device C0" << std::endl;
}

int main (int argc, char **argv)
{
	Talker app (argc, argv);
	return app.run ();
}

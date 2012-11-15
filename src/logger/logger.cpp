/* 
 * Logger client.
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
#include "client.h"

namespace rts2logd
{

class Logger:public rts2core::Client, public LoggerBase
{
	public:
		Logger (int in_argc, char **in_argv);

		virtual rts2core::DevClient *createOtherType (rts2core::Connection * conn, int other_device_type);
	protected:
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int willConnect (rts2core::NetworkAddress * in_addr);
	private:
		std::istream * inputStream;
};

}

using namespace rts2logd;

Logger::Logger (int in_argc, char **in_argv):rts2core::Client (in_argc, in_argv)
{
	setTimeout (USEC_SEC);
	inputStream = NULL;

	addOption ('c', NULL, 1, "specify config file with logged device, timeouts and values");
}

int Logger::processOption (int in_opt)
{
	int ret;
	switch (in_opt)
	{
		case 'c':
			inputStream = new std::ifstream (optarg);
			ret = readDevices (*inputStream);
			delete inputStream;
			return ret;
		default:
			return rts2core::Client::processOption (in_opt);
	}
	return 0;
}

int Logger::init ()
{
	int ret;
	ret = rts2core::Client::init ();
	if (ret)
		return ret;
	if (!inputStream)
		ret = readDevices (std::cin);
	return ret;
}

int Logger::willConnect (rts2core::NetworkAddress * in_addr)
{
	return LoggerBase::willConnect (in_addr);
}

rts2core::DevClient * Logger::createOtherType (rts2core::Connection * conn, int other_device_type)
{
	rts2core::DevClient *cli = LoggerBase::createOtherType (conn, other_device_type);
	if (cli)
		return cli;
	return rts2core::Client::createOtherType (conn, other_device_type);
}

int main (int argc, char **argv)
{
	Logger app = Logger (argc, argv);
	return app.run ();
}

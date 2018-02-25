/* 
 * UCAC5 indexing application
 * Copyright (C) 2018 Petr Kubanek <petr@kubanek.net>
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

#include "app.h"
#include "UCAC5Record.hpp"

#include <errno.h>
#include <string>
#include <vector>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

class UCAC5Search:public rts2core::App
{
	public:
		UCAC5Search (int argc, char **argv);

		virtual int run();
	
	protected:
		virtual int processOption (int opt);
		virtual int processArgs (const char *arg);
	
	private:
		
};

UCAC5Search::UCAC5Search (int argc, char **argv):App (argc, argv)
{
}

int UCAC5Search::run()
{
	int ret = init();
	if (ret)
		return ret;

	return 0;
}

int UCAC5Search::processOption (int opt)
{
	switch (opt)
	{
		default:
			return App::processOption(opt);
	}
	return 0;
}

int UCAC5Search::processArgs (const char *arg)
{
	return 0;
}

int main (int argc, char **argv)
{
	UCAC5Search app(argc, argv);
	return app.run();
}

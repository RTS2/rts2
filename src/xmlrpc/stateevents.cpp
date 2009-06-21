/* 
 * State changes triggering infrastructure. 
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "stateevents.h"
#include "message.h"

#include "../utils/rts2block.h"
#include "../utils/rts2logstream.h"

#include <fstream>

using namespace rts2xmlrpc;

void
StateCommands::load (const char *file)
{
	clear ();
	std::ifstream fs;
	fs.open (file);
	if (fs.fail ())
	{
		logStream (MESSAGE_ERROR) << "cannot open XML-RPC state config file " << file << sendLog;
		return;
	}
	// parse the file..
	while (!fs.fail ())
	{
		std::string line;
		getline (fs, line);
		if (fs.fail ())
			break;
		// eat commen strings
		size_type ci = line.find ('#');
		if (ci != std::string::npos)
		{
			line = line.substr (0, ci);
		}
		if (line.length () == 0)
			continue;
		// we have the string, try to get out what we need
		std::string deviceName;
		int changeMask;
		int newStateValue;
		std::string commandName;
		std::istringstream is (line);
		is >> deviceName >> changeMask >> newStateValue >> commandName;
		if (is.fail ())
		{
			logStream (MESSAGE_ERROR) << "Cannot parse XML-RPC state config line " << line << sendLog;
			continue;
		}
		push_back (StateChangeCommand (deviceName, changeMask, newStateValue, commandName));
	}
}

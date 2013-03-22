/* 
 * Scheduling web interface.
 * Copyright (C) 2013 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "bbdb.h"
#include "schedreq.h"

using namespace rts2bb;

void SchedReq::authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	response_type = "text/html";
	std::vector <std::string> vals = SplitStr (path.substr (1), std::string ("/"));
	std::ostringstream _os;

	if (vals.size () == 1)
	{
		// assume it is schedule id
		int sched_id = atoi (vals[0].c_str ());

		BBSchedules sched (sched_id);
		sched.load ();

		std::ostringstream title;

		title << "Scheduling status of " << sched_id << " for target #" << sched.getTargetId ();

		printHeader (_os, title.str ().c_str ());

		_os << "<p>Status of schedule " << sched_id << "</p>" << std::endl;

		_os << "</body></html>";
	}
	else
	{
		throw rts2core::Error ("Invalid path for schedules!");
	}

	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

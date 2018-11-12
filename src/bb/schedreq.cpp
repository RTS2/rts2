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

void SchedReq::authorizedExecute (__attribute__ ((unused)) XmlRpc::XmlRpcSource *source, std::string path, __attribute__ ((unused)) XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	response_type = "text/html";
	std::vector <std::string> vals = SplitStr (path.substr (1), std::string ("/"));
	std::ostringstream _os;

	if (vals.size () == 0)
	{
		std::ostringstream title;

		printHeader (_os, "All schedules", NULL, "/css/table.css", "schedules.refresh ()");

		includeJavaScriptWithPrefix (_os, "table.js");

		_os << "<p>All schedules</p>" << std::endl
			<< "<script type='text/javascript'>" << std::endl
			<< "schedules = new Table('../api/get_all_schedules', 'schedules', 'schedules');" << std::endl
			<< "</script>" << std::endl
			<< "<p><div id='schedules'>Loading..</div></p>" << std::endl
			<< "</body></html>";
	}
	else if (vals.size () == 1)
	{
		// assume it is schedule id
		int sched_id = atoi (vals[0].c_str ());

		BBSchedules sched (sched_id);
		sched.load ();

		std::ostringstream title;

		title << "Scheduling status of " << sched_id << " for target #" << sched.getTargetId ();

		printHeader (_os, title.str ().c_str (), NULL, "/css/datatables.css");

		includeJavaScript (_os, "jquery.js");
		includeJavaScript (_os, "datatables.js");

		_os <<
			"<script type='text/javascript' charset='utf-8'>\n"
				"sched_states = new Object ();\n"
				"sched_states[0] = 'created';\n"
				"sched_states[10] = 'failed';\n"
				"sched_states[11] = 'observable';\n"
				"sched_states[12] = 'backup';\n"
				"sched_states[13] = 'confirmed';\n"
				"$(document).ready(function () {\n"
					"$('#schedules').dataTable( {\n"
						"'bProcessing': true,\n"
						"'sAjaxSource': '../../api/get_schedule?id=" << sched_id << "',\n"
						"'bPaginate': false,\n"
						"'aoColumnDefs': [{\n"
							"'aTargets' : [1],\n"
							"'mRender': function(data,type,full) {\n"
								"return sched_states[data];\n"
							"}\n"
						"},{\n"
							"'aTargets' : [2,3,4,5],\n"
							"'mRender': function(data,type,full) {\n"
								"if (!data)\n"
									"return '---';\n"
								"var d = new Date(data * 1000);\n"
								"return d.toLocaleString();\n"
							"}\n"
						"}]\n"
					"} );\n"
				"} );\n"
			"</script>\n"

			"<table cellpadding='0' cellspacing='0' border='0' class='display' id='schedules'>\n"
				"<thead>\n"
					"<tr>\n"
						"<th>Observatory</th>\n"
						"<th>State</th>\n"
						"<th>From</th>\n"
						"<th>To</th>\n"
						"<th>Created</th>\n"
						"<th>Last Update</th>\n"
					"</tr>\n"
				"</thead>\n"
				"<tbody>\n"
					"<tr>\n"
						"<td colspan='5' class='dataTables_empty'>Loading data from server</td>\n"
					"</tr>\n"
				"</tbody>\n"
			"</table>\n";
	}
	else
	{
		throw rts2core::Error ("Invalid path for schedules!");
	}

	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

/* 
 * Classes for generating pages for planning/scheduling.
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

#include "planreq.h"
#include "xmlrpcd.h"

#include "../utilsdb/plan.h"

using namespace rts2xmlrpc;

void Plan::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::vector <std::string> vals = SplitStr (path.substr (1), std::string ("/"));
	response_type = "text/html";
	switch (vals.size ())
	{
		case 0:
			printPlans (params, response, response_length);
			return;
		case 1:
			printPlan (vals[0].c_str (), response, response_length);
			return;
	}
	throw rts2core::Error ("Invalid path");
}

void Plan::printPlans (XmlRpc::HttpParams *params, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	double t_from = params->getDouble ("from", getMasterApp ()->getNow ());
	double t_to = params->getDouble ("to", rts2_nan ("f"));

	std::ostringstream title;
	title << "Plan entrie from " << LibnovaDateDouble (t_from);
	if (!isnan (t_to))
		title << "to " << LibnovaDateDouble (t_to);

	printHeader (_os, title.str ().c_str (), NULL, "/css/table.css", "allPlans.refresh();");

	includeJavaScript (_os, "equ.js");
	includeJavaScriptWithPrefix (_os, "table.js");

	_os << "<script type='text/javascript'>\n"
		"allPlans = new Table('../api/plan?from=" << t_from;
	if (!isnan (t_to))
		_os << "&to=" << t_to;

	_os << "','plan','allPlan');\n"
		"</script>\n"
		"<div id='plan'>Loading..</div>\n";

	printFooter (_os);

	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Plan::printPlan (const char *id, char* &response, size_t &response_length)
{
	std::ostringstream _os;
	printHeader (_os, "Observing plan");

	char *end;
	int pid = strtol (id, &end, 10);
	if (end == id || *end != '\0')
		throw XmlRpc::XmlRpcException ("Invalid plan ID");

	rts2db::Plan p (pid);
	p.load ();

	_os << "<h1>Plan with ID" << pid << " for target <a href='../../targets/" << p.getTargetId () << "/'>" << p.getTarget ()->getTargetName () << "</a></h1>"
		<< "<p>Active from " << LibnovaDateDouble (p.getPlanStart ()) << " to " << LibnovaDateDouble (p.getPlanEnd ()) << "</p>";
	rts2db::Observation *o = p.getObservation ();
	if (o == NULL)
	{
		_os << "<p>Plan was not observed.</p>";
	}
	else
	{
		_os << "<p>Plan was observed as observation <a href='../../observations/" << o->getObsId () << "'>" << o->getObsId () << "</a></p>";
	}

	printFooter (_os);

	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

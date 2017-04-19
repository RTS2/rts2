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

#include "httpd.h"

#include "rts2db/plan.h"

using namespace rts2xmlrpc;

void Plan::authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
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

	double t_from = params->getDouble ("from", getNow ());
	double t_to = params->getDouble ("to", NAN);

	std::ostringstream title;
	title << "Plan entries from " << Timestamp (t_from);
	if (!std::isnan (t_to))
		title << "to " << Timestamp (t_to);

	printHeader (_os, title.str ().c_str (), NULL, "/css/table.css", "allPlans.refresh();");

	includeJavaScript (_os, "equ.js");
	includeJavaScriptWithPrefix (_os, "table.js");

	_os << "<script type='text/javascript'>\n"
		"allPlans = new Table('../api/plan?from=" << t_from;
	if (!std::isnan (t_to))
		_os << "&to=" << t_to;

	_os << "','plan','allPlans');\n"
		"</script>\n"
		"<div id='plan'>Loading..</div>\n";

	printFooter (_os);

	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

std::string printTarget (rts2db::Target *tar, double t)
{
	time_t tt = t; 
	double jd = ln_get_julian_from_timet (&tt);
	std::ostringstream ret;

	struct ln_equ_posn equ;
	tar->getPosition (&equ, jd);

	struct ln_hrz_posn hrz;
	tar->getAltAz (&hrz, jd);

	ret << "<table>"
		"<tr><td>Equatorial</td><td class='deg'>" << LibnovaRaDec (&equ) << "</td></tr>"
		"<tr><td>Horizontal</td><td class='deg'>" << LibnovaHrz (&hrz) << "</td></tr>"
		"<tr><td>Is above horizon</td><td>" << (tar->isAboveHorizon (&hrz) ? "yes" : "no") << "</td></tr>"
		"</table>";

	return ret.str ();
}

void Plan::printPlan (const char *id, char* &response, size_t &response_length)
{
	std::ostringstream _os;
	printHeader (_os, "Observing plan", NULL, "/css/table.css");

	char *end;
	int pid = strtol (id, &end, 10);
	if (end == id || *end != '\0')
		throw XmlRpc::XmlRpcException ("Invalid plan ID");

	rts2db::Plan p (pid);
	p.load ();

	rts2db::Target *tar = p.getTarget ();

	_os << "<h1>Plan with ID" << pid << " for target <a href='../../targets/" << p.getTargetId () << "/'>" << p.getTarget ()->getTargetName () << "</a></h1>"
		"<table><tr><td>From</td><td class='time'>" << Timestamp (p.getPlanStart ()) << "</td></tr>"
		"<tr><td>To</td><td class='time'>" << Timestamp (p.getPlanEnd ()) << "</td></tr>"
		"<tr><td>At the beginning</td><td>" << printTarget (tar, p.getPlanStart ()) << "</td></tr>";
	if (!std::isnan (p.getPlanEnd ()))
		_os << "<tr><td>At the end</td><td>" << printTarget (tar, p.getPlanEnd ()) << "</td></tr>";
	_os << "</table>";

	printFooter (_os);

	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

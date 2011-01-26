/* 
 * API access for RTS2.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "api.h"
#include "xmlrpcd.h"

#include "../utilsdb/planset.h"

using namespace rts2xmlrpc;

void API::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));
  	std::ostringstream os;
	Rts2Conn *conn;

	// widgets - divs with informations
	if (vals.size () >= 2 && vals[0] == "w")
	{
		getWidgets (vals, params, response_type, response, response_length);
		return;
	}

	os << "{";
	XmlRpcd * master = (XmlRpcd *) getMasterApp ();
	if (vals.size () == 1)
	{
		if (vals[0] == "executor")
		{
			connections_t::iterator iter = master->getConnections ()->begin ();
			master->getOpenConnectionType (DEVICE_TYPE_EXECUTOR, iter);
			if (iter == master->getConnections ()->end ())
			 	throw XmlRpcException ("executor is not connected");
			sendConnectionValues (os, *iter);
		}
		else if (vals[0] == "set")
		{
			const char *device = params->getString ("d","");
			const char *variable = params->getString ("n", "");
			const char *value = params->getString ("v", "");
			if (variable[0] == '\0')
				throw XmlRpcException ("variable name not set - missing or empty n parameter");
			if (value[0] == '\0')
				throw XmlRpcException ("value not set - missing or empty v parameter");
			conn = master->getOpenConnection (device);
			if (conn == NULL)
				throw XmlRpcException ("cannot find device with given name");
			Rts2Value * rts2v = master->getValue (device, variable);
			if (rts2v == NULL)
				throw XmlRpcException ("cannot find variable");
			conn->queCommand (new rts2core::Rts2CommandChangeValue (conn->getOtherDevClient (), std::string (variable), '=', std::string (value)));
			sendConnectionValues (os, conn);
		}
		else if (vals[0] == "get")
		{
			const char *device = params->getString ("d","");
			conn = master->getOpenConnection (device);
			if (conn == NULL)
				throw XmlRpcException ("cannot find device");
			sendConnectionValues (os, conn);
		}
		else if (vals[0] == "plan")
		{
			rts2db::PlanSet ps (params->getDouble ("from", master->getNow ()), params->getDouble ("to", rts2_nan ("f")));
			ps.load ();

			os << "\"h\":["
				"{\"n\":\"Plan ID\",\"t\":\"a\",\"prefix\":\"" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/plan/\",\"href\":0,\"c\":0},"
				"{\"n\":\"Target ID\",\"t\":\"a\",\"prefix\":\"" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/targets/\",\"href\":1,\"c\":1},"
				"{\"n\":\"Target Name\",\"t\":\"a\",\"prefix\":\"" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/targets/\",\"href\":1,\"c\":2},"
				"{\"n\":\"Start\",\"t\":\"t\",\"c\":3},"
				"{\"n\":\"End\",\"t\":\"t\",\"c\":4},"
				"{\"n\":\"RA\",\"t\":\"r\",\"c\":5},"
				"{\"n\":\"DEC\",\"t\":\"d\",\"c\":6},"
				"{\"n\":\"Alt start\",\"t\":\"altD\",\"c\":7},"
				"{\"n\":\"Az start\",\"t\":\"azD\",\"c\":8}],"
				"\"d\":[";

			for (rts2db::PlanSet::iterator iter = ps.begin (); iter != ps.end (); iter++)
			{
				if (iter != ps.begin ())
					os << ",";
				rts2db::Target *tar = iter->getTarget ();
				time_t t = iter->getPlanStart ();
				double JDstart = ln_get_julian_from_timet (&t);
				struct ln_equ_posn equ;
				tar->getPosition (&equ, JDstart);
				struct ln_hrz_posn hrz;
				tar->getAltAz (&hrz, JDstart);
				os << "[" << iter->getPlanId () << ","
					<< iter->getTargetId () << ",\""
					<< tar->getTargetName () << "\",\""
					<< LibnovaDateDouble (iter->getPlanStart ()) << "\",\""
					<< LibnovaDateDouble (iter->getPlanEnd ()) << "\","
					<< equ.ra << "," << equ.dec << ","
					<< hrz.alt << "," << hrz.az << "]";
			}

			os << "]";
		}
		else
		{
			throw XmlRpcException ("invalid request " + path);
		}
	}
	os << "}";

	response_type = "application/json";
	response_length = os.str ().length ();
	response = new char[response_length];
	memcpy (response, os.str ().c_str (), response_length);
}

void API::sendConnectionValues (std::ostringstream & os, Rts2Conn * conn)
{
	for (Rts2ValueVector::iterator iter = conn->valueBegin (); iter != conn->valueEnd ();)
	{
		switch ((*iter)->getValueType ())
		{
			case RTS2_VALUE_STRING:
				os << "\"" << (*iter)->getName () << "\":\"" << (*iter)->getValue () << "\"";
				break;
			case RTS2_VALUE_DOUBLE:
			case RTS2_VALUE_FLOAT:
			case RTS2_VALUE_INTEGER:
			case RTS2_VALUE_LONGINT:
			case RTS2_VALUE_SELECTION:
			case RTS2_VALUE_BOOL:
			case RTS2_VALUE_TIME:
				os << "\"" << (*iter)->getName () << "\":" << (*iter)->getValue ();
				break;
			case RTS2_VALUE_RADEC:
				os << "\"" << (*iter)->getName () << "\":{\"ra\":" << ((Rts2ValueRaDec *) (*iter))->getRa () << ",\"dec\":" << ((Rts2ValueRaDec *) (*iter))->getDec () << "}";
				break;
			default:
				iter++;
				continue;
		}
		iter++;
		if (iter != conn->valueEnd ())
			os << ",";
	}
}

void API::getWidgets (const std::vector <std::string> &vals, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream os;

	includeJavaScript (os, "widgets.js");

	if (vals[1] == "executor")
	{
		os << "<div id='executor'>\n<table>"
			"<tr><td>Current</td><td><div id='ex:current'></div></td></tr>"
			"<tr><td>Next</td><td><div id='ex:next'></div></td></tr>"
			"</table>\n"
			"<button type='button' onClick=\"refresh('executor','ex:','executor');\">Refresh</button>\n"
			"</div>\n";
	}
	else
	{
		throw XmlRpcException ("Widget " + vals[1] + " is not defined.");
	}

	response_type = "text/html";
	response_length = os.str ().length ();
	response = new char[response_length];
	memcpy (response, os.str ().c_str (), response_length);
}

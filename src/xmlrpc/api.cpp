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
#include "../plan/script.h"

using namespace rts2xmlrpc;


API::API (const char* prefix, XmlRpc::XmlRpcServer* s):GetRequestAuthorized (prefix, NULL, s)
{
}

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
			sendConnectionValues (os, *iter, params);
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
			if (isCentraldName (device))
				conn = master->getSingleCentralConn ();
			else
				conn = master->getOpenConnection (device);
			if (conn == NULL)
				throw XmlRpcException ("cannot find device with given name");
			rts2core::Value * rts2v = master->getValue (device, variable);
			if (rts2v == NULL)
				throw XmlRpcException ("cannot find variable");
			conn->queCommand (new rts2core::Rts2CommandChangeValue (conn->getOtherDevClient (), std::string (variable), '=', std::string (value)));
			sendConnectionValues (os, conn, params);
		}
		else if (vals[0] == "get")
		{
			const char *device = params->getString ("d","");
			if (isCentraldName (device))
				conn = master->getSingleCentralConn ();
			else
				conn = master->getOpenConnection (device);
			double from = params->getDouble ("from", 0);
			conn = master->getOpenConnection (device);
			if (conn == NULL)
				throw XmlRpcException ("cannot find device");
			sendConnectionValues (os, conn, params, from);
		}
		else if (vals[0] == "cmd")
		{
			const char *device = params->getString ("d", "");
			const char *cmd = params->getString ("c", "");
			if (cmd[0] == '\0')
				throw XmlRpcException ("empty command");
			if (isCentraldName (device))
				conn = master->getSingleCentralConn ();
			else
				conn = master->getOpenConnection (device);
			if (conn == NULL)
				throw XmlRpcException ("cannot find device");
			conn->queCommand (new rts2core::Rts2Command (master, cmd));
			sendConnectionValues (os, conn, params);
		}
#ifdef HAVE_PGSQL
		else if (vals[0] == "tbyname")
		{
			rts2db::TargetSet tar_set;
			const char *name = params->getString ("n", "");
			if (name[0] == '\0')
				throw XmlRpcException ("empty n parameter");
			tar_set.loadByName (name);
			jsonTargets (tar_set, os, params);
		}
		else if (vals[0] == "tbyid")
		{
			rts2db::TargetSet tar_set;
			int id = params->getInteger ("id", -1);
			if (id <= 0)
				throw XmlRpcException ("empty id parameter");
			tar_set.load (id);
			jsonTargets (tar_set, os, params);
		}
		else if (vals[0] == "tbylabel")
		{
			rts2db::TargetSet tar_set;
			int label = params->getInteger ("l", -1);
			if (label == -1)
			  	throw XmlRpcException ("empty l parameter");
			tar_set.loadByLabelId (label);
			jsonTargets (tar_set, os, params);
		}
		else if (vals[0] == "labels")
		{
			const char *label = params->getString ("l", "");
			if (label[0] == '\0')
				throw XmlRpcException ("empty l parameter");
			int t = params->getInteger ("t", -1);
			if (t < 0)
				throw XmlRpcException ("invalid type parametr");
			rts2db::Labels lb;
			os << lb.getLabel (label, t);
		}
		else if (vals[0] == "plan")
		{
			rts2db::PlanSet ps (params->getDouble ("from", master->getNow ()), params->getDouble ("to", rts2_nan ("f")));
			ps.load ();

			os << "\"h\":["
				"{\"n\":\"Plan ID\",\"t\":\"a\",\"prefix\":\"" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/plan/\",\"href\":0,\"c\":0},"
				"{\"n\":\"Target ID\",\"t\":\"a\",\"prefix\":\"" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/targets/\",\"href\":1,\"c\":1},"
				"{\"n\":\"Target Name\",\"t\":\"a\",\"prefix\":\"" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/targets/\",\"href\":1,\"c\":2},"
				"{\"n\":\"Obs ID\",\"t\":\"a\",\"prefix\":\"" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/observations/\",\"href\":3,\"c\":3},"
				"{\"n\":\"Start\",\"t\":\"t\",\"c\":4},"
				"{\"n\":\"End\",\"t\":\"t\",\"c\":5},"
				"{\"n\":\"RA\",\"t\":\"r\",\"c\":6},"
				"{\"n\":\"DEC\",\"t\":\"d\",\"c\":7},"
				"{\"n\":\"Alt start\",\"t\":\"altD\",\"c\":8},"
				"{\"n\":\"Az start\",\"t\":\"azD\",\"c\":9}],"
				"\"d\":[" << std::fixed;

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
					<< tar->getTargetName () << "\","
					<< iter->getObsId () << ",\""
					<< iter->getPlanStart () << "\",\""
					<< iter->getPlanEnd () << "\","
					<< equ.ra << "," << equ.dec << ","
					<< hrz.alt << "," << hrz.az << "]";
			}

			os << "]";
		}
#endif
		else
		{
			throw XmlRpcException ("invalid request " + path);
		}
	}
	os << "}";

	returnJSON (os.str ().c_str (), response_type, response, response_length);
}

void API::sendArrayValue (rts2core::Value *value, std::ostringstream &os)
{
	os << "[";
	switch (value->getValueBaseType ())
	{
		case RTS2_VALUE_INTEGER:
			for (std::vector <int>::iterator iter = ((rts2core::IntegerArray *) value)->valueBegin (); iter != ((rts2core::IntegerArray *) value)->valueEnd (); iter++)
			{
			  	if (iter != ((rts2core::IntegerArray *) value)->valueBegin ())
					os << ",";
				os << (*iter);
			}
			break;
		case RTS2_VALUE_STRING:
			for (std::vector <std::string>::iterator iter = ((rts2core::StringArray *) value)->valueBegin (); iter != ((rts2core::StringArray *) value)->valueEnd (); iter++)
			{
			  	if (iter != ((rts2core::StringArray *) value)->valueBegin ())
					os << ",";
				os << "\"" << (*iter) << "\"";
			}
			break;
		default:
			os << "\"" << value->getDisplayValue () << "\"";
			break;
	}
	os << "]";
}

void API::sendValue (rts2core::Value *value, std::ostringstream &os)
{
	switch (value->getValueBaseType ())
	{
		case RTS2_VALUE_STRING:
			os << "\"" << value->getValue () << "\"";
			break;
		case RTS2_VALUE_DOUBLE:
		case RTS2_VALUE_FLOAT:
		case RTS2_VALUE_TIME:
			if (isnan (value->getValueDouble ()))
				os << "null";
			else	
				os << value->getValue ();
			break;
		case RTS2_VALUE_INTEGER:
		case RTS2_VALUE_LONGINT:
		case RTS2_VALUE_SELECTION:
		case RTS2_VALUE_BOOL:
			os << value->getValue ();
			break;
		case RTS2_VALUE_RADEC:
			if (isnan (((rts2core::ValueRaDec *) value)->getRa ()) || isnan (((rts2core::ValueRaDec *) value)->getRa ()))
			  	os << "{\"ra\":null,\"dec\":null}";
			else
				os << "{\"ra\":" << ((rts2core::ValueRaDec *) value)->getRa () << ",\"dec\":" << ((rts2core::ValueRaDec *) value)->getDec () << "}";
			break;
		default:
			os << "\"" << value->getDisplayValue () << "\"";
			break;
	}
}

void API::sendConnectionValues (std::ostringstream & os, Rts2Conn * conn, HttpParams *params, double from)
{
	bool extended = params->getInteger ("e", false);
	os << "\"d\":{";
	double mfrom = rts2_nan ("f");
	bool first = true;
	for (rts2core::ValueVector::iterator iter = conn->valueBegin (); iter != conn->valueEnd (); iter++)
	{
		if (conn->getOtherDevClient ())
		{
			double ch = ((XmlDevClient *) (conn->getOtherDevClient ()))->getValueChangedTime (*iter);
			if (isnan (mfrom) || ch > mfrom)
				mfrom = ch;
			if (!isnan (from) && !isnan (ch) && ch < from)
				continue;
		}

		if (first)
			first = false;
		else
			os << ",";

		os << "\"" << (*iter)->getName () << "\":";
		if (extended)
			os << "[" << (*iter)->getFlags () << ",";
	  	if ((*iter)->getValueExtType())
		  	sendArrayValue (*iter, os);
		else
		  	sendValue (*iter, os);
		if (extended)
			os << "," << (*iter)->isError () << "," << (*iter)->isWarning () << ",\"" << (*iter)->getDescription () << "\"]";
	}
	os << "},\"f\":";
	if (isnan (mfrom))
		os << "null";
	else
		os << std::fixed << mfrom;  	
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

#ifdef HAVE_PGSQL
void API::jsonTargets (rts2db::TargetSet &tar_set, std::ostream &os, XmlRpc::HttpParams *params)
{
	bool extended = params->getInteger ("e", false);  
	os << "\"h\":["
		"{\"n\":\"Target ID\",\"t\":\"n\",\"prefix\":\"" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/targets/\",\"href\":0,\"c\":0},"
		"{\"n\":\"Target Name\",\"t\":\"a\",\"prefix\":\"" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/targets/\",\"href\":0,\"c\":1},"
		"{\"n\":\"RA\",\"t\":\"r\",\"c\":2},"
		"{\"n\":\"DEC\",\"t\":\"d\",\"c\":3}";
	if (extended)
		os << ",{\"n\":\"Duration\",\"t\":\"dur\",\"c\":4},"
		"{\"n\":\"Scripts\",\"t\":\"scripts\",\"c\":5}";
	os << "],\"d\":[" << std::fixed;

	double JD = ln_get_julian_from_sys ();
	os.precision (8);
	for (rts2db::TargetSet::iterator iter = tar_set.begin (); iter != tar_set.end (); iter++)
	{
		if (iter != tar_set.begin ())
			os << ",";
		struct ln_equ_posn equ;
		rts2db::Target *tar = iter->second;
		tar->getPosition (&equ, JD);
		const char *n = tar->getTargetName ();
		os << "[" << tar->getTargetID () << ",";
		if (n == NULL)
			os << "null,";
		else
			os << "\"" << n << "\",";
		if (isnan (equ.ra) || isnan(equ.dec))
			os << "null,null";
		else
			os << equ.ra << "," << equ.dec;

		if (extended)
		{
			double md = -1;
			std::ostringstream cs;
			for (Rts2CamList::iterator cam = ((XmlRpcd *) getMasterApp ())->cameras.begin (); cam != ((XmlRpcd *) getMasterApp ())->cameras.end (); cam++)
			{
				try
				{

					std::string script_buf;
					rts2script::Script script;
					tar->getScript (cam->c_str(), script_buf);
					if (cam != ((XmlRpcd *) getMasterApp ())->cameras.begin ())
						cs << ",";
					script.setTarget (cam->c_str (), tar);
					double d = script.getExpectedDuration ();
					cs << "{\"" << *cam << "\":[\"" << script_buf << "\"," << d << "]}";
					if (d > md)
						md = d;  
				}
				catch (rts2core::Error &er)
				{
					logStream (MESSAGE_ERROR) << "cannot parsing script for camera " << *cam << ": " << er << sendLog;
				}
			}
			os << "," << md << ",[" << cs.str () << "]";
		}
		os << "]";
	}
	os << "]";
}
#endif // HAVE_PGSQL

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

#include "xmlrpcd.h"

#include "../utilsdb/constraints.h"
#include "../utilsdb/planset.h"
#include "../plan/script.h"

#ifdef HAVE_PGSQL
#include "../utilsdb/labellist.h"
#include "../utilsdb/simbadtarget.h"
#endif

using namespace rts2xmlrpc;

AsyncAPI::AsyncAPI (API *_req, Rts2Conn *_conn, XmlRpcServerConnection *_source):Rts2Object ()
{
	// that's legal - requests are statically allocated and will cease exists with the end of application
	req = _req;
	conn = _conn;
	source = _source;
}

void AsyncAPI::postEvent (Rts2Event *event)
{
	std::ostringstream os;
	switch (event->getType ())
	{
		case EVENT_COMMAND_OK:
			os << "{";
			req->sendConnectionValues (os, conn, NULL);
			os << ",\"ret\":0 }";
			req->sendAsyncJSON (os, source);
			break;
		case EVENT_COMMAND_FAILED:
			os << "{";
			req->sendConnectionValues (os, conn, NULL);
			os << ", \"ret\":-1 }";
			req->sendAsyncJSON (os, source);
			break;
	}
	Rts2Object::postEvent (event);
}

class AsyncAPIExpose:public AsyncAPI
{
	public:
		AsyncAPIExpose (API *_req, Rts2Conn *conn, XmlRpcServerConnection *_source);

		virtual void postEvent (Rts2Event *event);
	private:
		enum {waitForExpReturn, waitForImage} callState;
};

AsyncAPIExpose::AsyncAPIExpose (API *_req, Rts2Conn *_conn, XmlRpcServerConnection *_source):AsyncAPI (_req, _conn, _source)
{
	callState = waitForExpReturn;
}

void AsyncAPIExpose::postEvent (Rts2Event *event)
{
	switch (event->getType ())
	{
		case EVENT_COMMAND_OK:
			if (callState == waitForExpReturn)
				callState = waitForImage;
			break;
		case EVENT_COMMAND_FAILED:
			if (callState == waitForExpReturn)
			{
				std::ostringstream os;
				os << "{\"failed\"}";
				req->sendAsyncJSON (os, source);
			}
			break;
	}
	Rts2Object::postEvent (event);
}

API::API (const char* prefix, XmlRpc::XmlRpcServer* s):GetRequestAuthorized (prefix, NULL, s)
{
}

void API::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	try
	{
		executeJSON (path, params, response_type, response, response_length);
	}
	catch (rts2core::Error &er)
	{
		throw JSONException (er.what ());
	}
}

void API::executeJSON (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
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

	os << std::fixed;
	os.precision (8);

	XmlRpcd * master = (XmlRpcd *) getMasterApp ();

	// calls returning arrays
	if (vals.size () == 1 && vals[0] == "devices")
	{
		os << "[";
		for (connections_t::iterator iter = master->getConnections ()->begin (); iter != master->getConnections ()->end (); iter++)
		{
			if (iter != master->getConnections ()->begin ())
				os << ",";
			os << '"' << (*iter)->getName () << '"';
		}
		os << "]";
	}
	else if (vals.size () == 1 && vals[0] == "devbytype")
	{
		os << "[";
		int t = params->getInteger ("t",0);
		connections_t::iterator iter = master->getConnections ()->begin ();
		bool first = true;
		while (true)
		{
			master->getOpenConnectionType (t, iter);
			if (iter == master->getConnections ()->end ())
				break;
			if (first)
				first = false;
			else
				os << ",";
			os << '"' << (*iter)->getName () << '"';
			iter++;
		}
		os << ']';
	}
	// returns array with selection value strings
	else if (vals.size () == 1 && vals[0] == "selval")
	{
		const char *device = params->getString ("d","");
		const char *variable = params->getString ("n", "");
		if (variable[0] == '\0')
			throw JSONException ("variable name not set - missing or empty n parameter");
		if (isCentraldName (device))
			conn = master->getSingleCentralConn ();
		else
			conn = master->getOpenConnection (device);
		if (conn == NULL)
			throw JSONException ("cannot find device with given name");
		rts2core::Value * rts2v = master->getValue (device, variable);
		if (rts2v == NULL)
			throw JSONException ("cannot find variable");
		if (rts2v->getValueBaseType () != RTS2_VALUE_SELECTION)
			throw JSONException ("variable is not selection");
		sendSelection (os, (rts2core::ValueSelection *) rts2v);
	}
	// return sun altitude
	else if (vals.size () == 1 && vals[0] == "sunalt")
	{
		time_t from = params->getInteger ("from", master->getNow () - 86400);
		time_t to = params->getInteger ("to", from + 86400);
		const int steps = params->getInteger ("step", 1000);

		const double jd_from = ln_get_julian_from_timet (&from);
		const double jd_to = ln_get_julian_from_timet (&to);

		struct ln_equ_posn equ;
		struct ln_hrz_posn hrz;

		os << "[" << std::fixed;
		for (double jd = jd_from; jd < jd_to; jd += fabs (jd_to - jd_from) / steps)
		{
			if (jd != jd_from)
				os << ",";
			ln_get_solar_equ_coords (jd, &equ);
			ln_get_hrz_from_equ (&equ, Rts2Config::instance ()->getObserver (), jd, &hrz);
			os << "[" << hrz.alt << "," << hrz.az << "]";
		}
		os << "]";
	}
#ifdef HAVE_PGSQL
	else if (vals.size () == 1 && vals[0] == "script")
	{
		int id = params->getInteger ("id", -1);
		if (id <= 0)
			throw JSONException ("invalid id parameter");
		const char *cname = params->getString ("cn", "");
		if (cname[0] == '\0')
			throw JSONException ("empty camera name");
		
		rts2db::Target *target = createTarget (id, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
		rts2script::Script script = rts2script::Script ();
		script.setTarget (cname, target);
		script.prettyPrint (os, rts2script::PRINT_JSON);
	}
	// return altitude of target..
	else if (vals.size () == 1 && vals[0] == "taltitudes")
	{
		int id = params->getInteger ("id", -1);
		if (id <= 0)
			throw JSONException ("invalid id parameter");
		time_t from = params->getInteger ("from", master->getNow () - 86400);
		time_t to = params->getInteger ("to", from + 86400);
		const int steps = params->getInteger ("steps", 1000);

		rts2db::Target *target = createTarget (id, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
		const double jd_from = ln_get_julian_from_timet (&from);
		const double jd_to = ln_get_julian_from_timet (&to);

		struct ln_hrz_posn hrz;

		os << "[" << std::fixed;
		for (double jd = jd_from; jd < jd_to; jd += fabs (jd_to - jd_from) / steps)
		{
			if (jd != jd_from)
				os << ",";
			target->getAltAz (&hrz, jd);
			os << hrz.alt;
		}
		os << "]";
	}
	else if (vals.size () == 1 && vals[0] == "consts")
	{
		int tid = params->getInteger ("id", -1);
		if (tid <= 0)
			throw JSONException ("empty target ID");
		rts2db::Target *target = createTarget (tid, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
		target->getConstraints ()->print (os);

		response_type = "application/xml";
		response_length = os.str ().length ();
		response = new char[response_length];
		memcpy (response, os.str().c_str (), response_length);
		return;
	}
#endif // HAVE_PGSQL
	else if (vals.size () == 1)
	{
		os << "{";
		if (vals[0] == "executor")
		{
			connections_t::iterator iter = master->getConnections ()->begin ();
			master->getOpenConnectionType (DEVICE_TYPE_EXECUTOR, iter);
			if (iter == master->getConnections ()->end ())
			 	throw JSONException ("executor is not connected");
			sendConnectionValues (os, *iter, params);
		}
		// device information
		else if (vals[0] == "deviceinfo")
		{
			const char *device = params->getString ("d","");
			if (isCentraldName (device))
				conn = master->getSingleCentralConn ();
			else
				conn = master->getOpenConnection (device);
			if (conn == NULL)
				throw JSONException ("cannot find device with given name");
			os << "\"type\":" << conn->getOtherType ();
		}
		// set or increment variable
		else if (vals[0] == "set" || vals[0] == "inc")
		{
			const char *device = params->getString ("d","");
			const char *variable = params->getString ("n", "");
			const char *value = params->getString ("v", "");
			if (variable[0] == '\0')
				throw JSONException ("variable name not set - missing or empty n parameter");
			if (value[0] == '\0')
				throw JSONException ("value not set - missing or empty v parameter");
			if (isCentraldName (device))
				conn = master->getSingleCentralConn ();
			else
				conn = master->getOpenConnection (device);
			if (conn == NULL)
				throw JSONException ("cannot find device with given name");
			rts2core::Value * rts2v = master->getValue (device, variable);
			if (rts2v == NULL)
				throw JSONException ("cannot find variable");
			char op;
			if (vals[0] == "inc")
				op = '+';
			else
				op = '=';
			conn->queCommand (new rts2core::Rts2CommandChangeValue (conn->getOtherDevClient (), std::string (variable), op, std::string (value), true));
			sendConnectionValues (os, conn, params);
		}
		// get variable
		else if (vals[0] == "get" || vals[0] == "status")
		{
			const char *device = params->getString ("d","");
			if (isCentraldName (device))
				conn = master->getSingleCentralConn ();
			else
				conn = master->getOpenConnection (device);
			if (conn == NULL)
				throw JSONException ("cannot find device");
			double from = params->getDouble ("from", 0);
			sendConnectionValues (os, conn, params, from);
		}
		// execute command on server
		else if (vals[0] == "cmd")
		{
			const char *device = params->getString ("d", "");
			const char *cmd = params->getString ("c", "");
			int async = params->getInteger ("async", 0);
			if (cmd[0] == '\0')
				throw JSONException ("empty command");
			if (isCentraldName (device))
				conn = master->getSingleCentralConn ();
			else
				conn = master->getOpenConnection (device);
			if (conn == NULL)
				throw JSONException ("cannot find device");
			if (async)
			{
				conn->queCommand (new rts2core::Rts2Command (master, cmd));
				sendConnectionValues (os, conn, params);
			}
			else
			{
				// TODO must deallocated..
				AsyncAPI *aa = new AsyncAPI (this, conn, connection);
				((XmlRpcd *) getMasterApp ())->registerAPI (aa);

				conn->queCommand (new rts2core::Rts2Command (master, cmd), 0, aa);
				throw XmlRpc::XmlRpcAsynchronous ();
			}
		}
		// start exposure, return from server image
		else if (vals[0] == "expose")
		{
			const char *camera = params->getString ("ccd","");
			conn = master->getOpenConnection (camera);
			if (conn == NULL || conn->getOtherType () != DEVICE_TYPE_CCD)
				throw JSONException ("cannot find camera with given name");
			AsyncAPIExpose *aa = new AsyncAPIExpose (this, conn, connection);
			((XmlRpcd *) getMasterApp ())->registerAPI (aa);

			conn->queCommand (new rts2core::Rts2CommandExposure (master, (Rts2DevClientCamera *) (conn->getOtherDevClient ()), 0), 0, aa);
			throw XmlRpc::XmlRpcAsynchronous ();
		}
#ifdef HAVE_PGSQL
		// returns target information specified by target name
		else if (vals[0] == "tbyname")
		{
			rts2db::TargetSet tar_set;
			const char *name = params->getString ("n", "");
			bool ic = params->getInteger ("ic",1);
			bool pm = params->getInteger ("pm",1);
			if (name[0] == '\0')
				throw JSONException ("empty n parameter");
			tar_set.loadByName (name, pm, ic);
			jsonTargets (tar_set, os, params);
		}
		// returns target specified by target ID
		else if (vals[0] == "tbyid")
		{
			rts2db::TargetSet tar_set;
			int id = params->getInteger ("id", -1);
			if (id <= 0)
				throw JSONException ("empty id parameter");
			tar_set.load (id);
			jsonTargets (tar_set, os, params);
		}
		// returns target with given label
		else if (vals[0] == "tbylabel")
		{
			rts2db::TargetSet tar_set;
			int label = params->getInteger ("l", -1);
			if (label == -1)
			  	throw JSONException ("empty l parameter");
			tar_set.loadByLabelId (label);
			jsonTargets (tar_set, os, params);
		}
		// returns targets within certain radius from given ra dec
		else if (vals[0] == "tbydistance")
		{
			struct ln_equ_posn pos;
			pos.ra = params->getDouble ("ra",rts2_nan("f"));
			pos.dec = params->getDouble ("dec",rts2_nan("f"));
			double radius = params->getDouble ("radius",rts2_nan("f"));
			if (isnan (pos.ra) || isnan (pos.dec) || isnan (radius))
				throw JSONException ("invalid ra, dec or radius parameter");
			rts2db::TargetSet ts (&pos, radius, Rts2Config::instance ()->getObserver ());
			ts.load ();
			jsonTargets (ts, os, params, &pos);
		}
		// try to parse and understand string (similar to new target), return either target or all target information
		else if (vals[0] == "tbystring")
		{
			const char *tar_name = params->getString ("ts", "");
			if (tar_name[0] == '\0')
				throw JSONException ("empty ts parameter");
			rts2db::Target *target = createTargetByString (tar_name);
			if (target == NULL)
				throw JSONException ("cannot parse target");
			struct ln_equ_posn pos;
			target->getPosition (&pos);
			os << "\"name\":\"" << tar_name << "\",\"ra\":" << pos.ra << ",\"dec\":" << pos.dec << ",\"desc\":\"" << target->getTargetInfo () << "\"";
			double nearest = params->getDouble ("nearest", -1);
			if (nearest >= 0)
			{
				os << ",\"nearest\":{";
				rts2db::TargetSet ts (&pos, nearest, Rts2Config::instance ()->getObserver ());
				ts.load ();
				jsonTargets (ts, os, params, &pos);
				os << "}";
			}
		}
		else if (vals[0] == "ibyoid")
		{
			int obsid = params->getInteger ("oid", -1);
			if (obsid == -1)
				throw JSONException ("empty oid parameter");
			rts2db::Observation obs (obsid);
			if (obs.load ())
				throw JSONException ("Cannot load observation set");
			obs.loadImages ();	
			jsonImages (obs.getImageSet (), os, params);
		}
		else if (vals[0] == "labels")
		{
			const char *label = params->getString ("l", "");
			if (label[0] == '\0')
				throw JSONException ("empty l parameter");
			int t = params->getInteger ("t", -1);
			if (t < 0)
				throw JSONException ("invalid type parametr");
			rts2db::Labels lb;
			os << lb.getLabel (label, t);
		}
		// violated constrainsts..
		else if (vals[0] == "violated")
		{
			const char *cn = params->getString ("consts", "");
			if (cn[0] == '\0')
				throw JSONException ("unknow constraint name");
			int tar_id = params->getInteger ("id", -1);
			if (tar_id < 0)
				throw JSONException ("unknown target ID");
			double from = params->getDouble ("from", master->getNow ());
			double to = params->getDouble ("to", from + 86400);
			// 60 sec = 1 minute step (by default)
			double step = params->getDouble ("step", 60);

			rts2db::Target *tar = createTarget (tar_id, Rts2Config::instance ()->getObserver (), master->getNotifyConnection ());
			rts2db::Constraints *cons = tar->getConstraints ();

			os << '"' << cn << "\":[";

			if (cons->find (std::string (cn)) != cons->end ())
			{
				rts2db::ConstraintPtr cptr = (*cons)[std::string (cn)];
				bool first_it = true;

				rts2db::interval_arr_t intervals;
				cptr->getViolatedIntervals (tar, from, to, step, intervals);
				for (rts2db::interval_arr_t::iterator iter = intervals.begin (); iter != intervals.end (); iter++)
				{
					if (first_it)
						first_it = false;
					else
						os << ",";
					os << "[" << iter->first << "," << iter->second << "]";
				}
			}
			os << "]";
		}
		// find unviolated time interval..
		else if (vals[0] == "satisfied")
		{
			int tar_id = params->getInteger ("id", -1);
			if (tar_id < 0)
				throw JSONException ("unknow target ID");
			time_t from = params->getDouble ("from", master->getNow ());
			time_t to = params->getDouble ("to", from + 86400);
			double length = params->getDouble ("length", rts2_nan ("f"));
			int step = params->getInteger ("step", 60);

			rts2db::Target *tar = createTarget (tar_id, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
			if (isnan (length))
				length = 1800;
			rts2db::interval_arr_t si;
			from -= from % step;
			to += step - (to % step);
			tar->getSatisfiedIntervals (from, to, length, step, si);
			os << "\"id\":" << tar_id << ",\"satisfied\":[";
			for (rts2db::interval_arr_t::iterator sat = si.begin (); sat != si.end (); sat++)
			{
				if (sat != si.begin ())
					os << ",";
				os << "[" << sat->first << "," << sat->second << "]";
			}
			os << "]";
		}
		// return intervals of altitude constraints (or violated intervals)
		else if (vals[0] == "cnst_alt" || vals[0] == "cnst_alt_v")
		{
			int tar_id = params->getInteger ("id", -1);
			if (tar_id < 0)
				throw JSONException ("unknow target ID");
			rts2db::Target *tar = createTarget (tar_id, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());

			std::map <std::string, std::vector <rts2db::ConstraintDoubleInterval> > ac;

			if (vals[0] == "cnst_alt")
				tar->getAltitudeConstraints (ac);
			else
				tar->getAltitudeViolatedConstraints (ac);

			os << "\"id\":" << tar_id << ",\"altitudes\":{";

			for (std::map <std::string, std::vector <rts2db::ConstraintDoubleInterval> >::iterator iter = ac.begin (); iter != ac.end (); iter++)
			{
				if (iter != ac.begin ())
					os << ",\"";
				else
					os << "\"";
				os << iter->first << "\":[";
				for (std::vector <rts2db::ConstraintDoubleInterval>::iterator di = iter->second.begin (); di != iter->second.end (); di++)
				{
					if (di != iter->second.begin ())
						os << ",[";
					else
						os << "[";
					os << JsonDouble (di->getLower ()) << "," << JsonDouble (di->getUpper ()) << "]";
				}
				os << "]";
			}
			os << "}";
		}
		// return intervals of time constraints (or violated time intervals)
		else if (vals[0] == "cnst_time" || vals[0] == "cnst_time_v")
		{
			int tar_id = params->getInteger ("id", -1);
			if (tar_id < 0)
				throw JSONException ("unknow target ID");
			time_t from = params->getInteger ("from", master->getNow ());
			time_t to = params->getInteger ("to", from + 86400);
			double steps = params->getDouble ("steps", 1000);

			steps = double ((to - from)) / steps;

			rts2db::Target *tar = createTarget (tar_id, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());

			std::map <std::string, rts2db::ConstraintPtr> cons;

			tar->getTimeConstraints (cons);

			os << "\"id\":" << tar_id << ",\"constraints\":{";

			for (std::map <std::string, rts2db::ConstraintPtr>::iterator iter = cons.begin (); iter != cons.end (); iter++)
			{
				if (iter != cons.begin ())
					os << ",\"";
				else
					os << "\"";
				os << iter->first << "\":[";

				rts2db::interval_arr_t intervals;
				if (vals[0] == "cnst_time")
					iter->second->getSatisfiedIntervals (tar, from, to, steps, intervals);
				else
					iter->second->getViolatedIntervals (tar, from, to, steps, intervals);

				for (rts2db::interval_arr_t::iterator it = intervals.begin (); it != intervals.end (); it++)
				{
					if (it != intervals.begin ())
						os << ",[";
					else
						os << "[";
					os << it->first << "," << it->second << "]";
				}
				os << "]";
			}
			os << "}";
		}
		else if (vals[0] == "create_target")
		{
			const char *tn = params->getString ("tn", "");
			double ra = params->getDouble ("ra", rts2_nan("f"));
			double dec = params->getDouble ("dec", rts2_nan("f"));
			const char *desc = params->getString ("desc", "");
			const char *type = params->getString ("type", "O");
			bool overwrite = params->getBoolean ("overwrite", false);

			if (strlen (tn) == 0)
				throw JSONException ("empty target name");
			if (strlen (type) != 1)
				throw JSONException ("invalid target type");

			rts2db::ConstTarget nt;
			nt.setTargetName (tn);
			nt.setPosition (ra, dec);
			nt.setTargetInfo (std::string (desc));
			nt.setTargetType (type[0]);
			nt.save (overwrite);

			os << "\"id\":" << nt.getTargetID ();
		}
		else if (vals[0] == "change_script")
		{
			int tar_id = params->getInteger ("id", -1);
			if (tar_id < 0)
				throw JSONException ("unknow target ID");
			rts2db::Target *tar = createTarget (tar_id, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
			const char *cam = params->getString ("c", NULL);
			if (strlen (cam) == 0)
				throw JSONException ("unknow camera");
			const char *s = params->getString ("s", "");
			if (strlen (s) == 0)
				throw JSONException ("empty script");
			tar->setScript (cam, s);
			os << "\"id\":" << tar_id << ",\"camera\":\"" << cam << "\",\"script\":\"" << s << "\"";
		}
		else if (vals[0] == "plan")
		{
			rts2db::PlanSet ps (params->getDouble ("from", master->getNow ()), params->getDouble ("to", rts2_nan ("f")));
			ps.load ();

			os << "\"h\":["
				"{\"n\":\"Plan ID\",\"t\":\"a\",\"prefix\":\"" << master->getPagePrefix () << "/plan/\",\"href\":0,\"c\":0},"
				"{\"n\":\"Target ID\",\"t\":\"a\",\"prefix\":\"" << master->getPagePrefix () << "/targets/\",\"href\":1,\"c\":1},"
				"{\"n\":\"Target Name\",\"t\":\"a\",\"prefix\":\"" << master->getPagePrefix () << "/targets/\",\"href\":1,\"c\":2},"
				"{\"n\":\"Obs ID\",\"t\":\"a\",\"prefix\":\"" << master->getPagePrefix () << "/observations/\",\"href\":3,\"c\":3},"
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
		else if (vals[0] == "labellist")
		{
			rts2db::LabelList ll;
			ll.load ();

			os << "\"h\":["
				"{\"n\":\"Label ID\",\"t\":\"n\",\"c\":0},"
				"{\"n\":\"Label type\",\"t\":\"n\",\"c\":1},"
				"{\"n\":\"Label text\",\"t\":\"s\",\"c\":2}],"
				"\"d\":[";

			for (rts2db::LabelList::iterator iter = ll.begin (); iter != ll.end (); iter++)
			{
				if (iter != ll.begin ())
					os << ",";
				os << "[" << iter->labid << ","
					<< iter->tid << ",\""
					<< iter->text << "\"]";
			}
			os << "]";
		}	
#endif
		else
		{
			throw JSONException ("invalid request " + path);
		}
		os << "}";
	}

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
		case RTS2_VALUE_DOUBLE:	
		case RTS2_VALUE_TIME:
			for (std::vector <double>::iterator iter = ((rts2core::DoubleArray *) value)->valueBegin (); iter != ((rts2core::DoubleArray *) value)->valueEnd (); iter++)
			{
				if (iter != ((rts2core::DoubleArray *) value)->valueBegin ())
					os << ",";
				os << JsonDouble (*iter);
			}
			break;
		default:
			os << "\"" << value->getDisplayValue () << "\"";
			break;
	}
	os << "]";
}

void API::sendStatValue (rts2core::Value *value, std::ostringstream &os)
{
	os << "[";
	switch (value->getValueBaseType ())
	{
		case RTS2_VALUE_DOUBLE:
			{
				rts2core::ValueDoubleStat *vds = (rts2core::ValueDoubleStat *) value;
				os << vds->getNumMes () << ","
					<< JsonDouble (vds->getValueDouble ()) << ","
					<< JsonDouble (vds->getMode ()) << ","
					<< JsonDouble (vds->getStdev ()) << ","
					<< JsonDouble (vds->getMin ()) << ","
					<< JsonDouble (vds->getMax ());
			}
			break;
		default:
			os << "\"" << value->getDisplayValue () << "\"";
			break;
	}
	os << "]";
}

void API::sendRectangleValue (rts2core::Value *value, std::ostringstream &os)
{
	rts2core::ValueRectangle *r = (rts2core::ValueRectangle *) value;
	os << "[" << r->getXInt () << "," << r->getYInt () << "," << r->getWidthInt () << "," << r->getHeightInt () << "]";
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
			os << JsonDouble (value->getValueDouble ());
			break;
		case RTS2_VALUE_INTEGER:
		case RTS2_VALUE_LONGINT:
		case RTS2_VALUE_SELECTION:
		case RTS2_VALUE_BOOL:
			os << value->getValue ();
			break;
		case RTS2_VALUE_RADEC:
			os << "{\"ra\":" << JsonDouble (((rts2core::ValueRaDec *) value)->getRa ()) << ",\"dec\":" << JsonDouble (((rts2core::ValueRaDec *) value)->getDec ()) << "}";
			break;
		case RTS2_VALUE_ALTAZ:
			os << "{\"alt\":" << JsonDouble (((rts2core::ValueAltAz *) value)->getAlt ()) << ",\"az\":" << JsonDouble (((rts2core::ValueAltAz *) value)->getAz ()) << "}";
			break;
		default:
			os << "\"" << value->getDisplayValue () << "\"";
			break;
	}
}

void API::sendSelection (std::ostringstream &os, rts2core::ValueSelection *value)
{
	os << "[";
	for (std::vector <rts2core::SelVal>::iterator iter = value->selBegin (); iter != value->selEnd (); iter++)
	{
		if (iter != value->selBegin ())
			os << ",";
		os << '"' << iter->name << '"';
	}
	os << "]";

}

void API::sendConnectionValues (std::ostringstream & os, Rts2Conn * conn, HttpParams *params, double from)
{
	bool extended = true;
	if (params)
		extended = params->getInteger ("e", false);

	os << "\"d\":{" << std::fixed;
	double mfrom = rts2_nan ("f");
	bool first = true;
	for (rts2core::ValueVector::iterator iter = conn->valueBegin (); iter != conn->valueEnd (); iter++)
	{
		if (conn->getOtherDevClient ())
		{
			double ch = ((XmlDevInterface *) (conn->getOtherDevClient ()))->getValueChangedTime (*iter);
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
	  	if ((*iter)->getValueExtType() & RTS2_VALUE_ARRAY)
		  	sendArrayValue (*iter, os);
		else switch ((*iter)->getValueExtType())
		{
			case RTS2_VALUE_STAT:
				sendStatValue (*iter, os);
				break;
			case RTS2_VALUE_RECTANGLE:
				sendRectangleValue (*iter, os);
				break;
			default:
		  		sendValue (*iter, os);
				break;
		}
		if (extended)
			os << "," << (*iter)->isError () << "," << (*iter)->isWarning () << ",\"" << (*iter)->getDescription () << "\"]";
	}
	os << "},\"idle\":" << conn->isIdle () << ",\"state\":" << conn->getState () << ",\"f\":" << JsonDouble (mfrom);
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
void API::jsonTargets (rts2db::TargetSet &tar_set, std::ostream &os, XmlRpc::HttpParams *params, struct ln_equ_posn *dfrom)
{
	bool extended = params->getInteger ("e", false);
	time_t from = params->getInteger ("from", getMasterApp()->getNow ());
	int c = 4;
	os << "\"h\":["
		"{\"n\":\"Target ID\",\"t\":\"n\",\"prefix\":\"" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/targets/\",\"href\":0,\"c\":0},"
		"{\"n\":\"Target Name\",\"t\":\"a\",\"prefix\":\"" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/targets/\",\"href\":0,\"c\":1},"
		"{\"n\":\"RA\",\"t\":\"r\",\"c\":2},"
		"{\"n\":\"DEC\",\"t\":\"d\",\"c\":3}";
	if (dfrom)
	{
		os << ",{\"n\":\"Distance\",\"t\":\"d\",\"c\":4}";
		c = 5;
	}
	if (extended)
		os << ",{\"n\":\"Duration\",\"t\":\"dur\",\"c\":" << (c) << "},"
		"{\"n\":\"Scripts\",\"t\":\"scripts\",\"c\":" << (c + 1) << "},"
		"{\"n\":\"Satisfied\",\"t\":\"s\",\"c\":" << (c + 2) << "},"
		"{\"n\":\"Violated\",\"t\":\"s\",\"c\":" << (c + 3) << "},"
		"{\"n\":\"Transit\",\"t\":\"t\",\"c\":" << (c + 4) << "},"
		"{\"n\":\"Observable\",\"t\":\"t\",\"c\":" << (c + 5) << "}";
	os << "],\"d\":[" << std::fixed;

	double JD = ln_get_julian_from_timet (&from);
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

		os << JsonDouble (equ.ra) << ',' << JsonDouble (equ.dec);

		if (dfrom)
			os << ',' << JsonDouble (tar->getDistance (dfrom, JD));

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
			os << "," << md << ",[" << cs.str () << "],";

			tar->getSatisfiedConstraints (JD).printJSON (os);
			
			os << ",";
		
			tar->getViolatedConstraints (JD).printJSON (os);

			if (isnan (equ.ra) || isnan (equ.dec))
			{
				os << ",null,true";
			}
			else
			{
				struct ln_hrz_posn hrz;
				tar->getAltAz (&hrz, JD);

				struct ln_rst_time rst;
				tar->getRST (&rst, JD, 0);

				os << "," << JsonDouble (rst.transit) 
					<< "," << (tar->isAboveHorizon (&hrz) ? "true" : "false");
			}
		}
		os << "]";
	}
	os << "]";
}

void API::jsonImages (rts2db::ImageSet *img_set, std::ostream &os, XmlRpc::HttpParams *params)
{
	os << "\"h\":["
		"{\"n\":\"Image\",\"t\":\"ccdi\",\"c\":0},"
		"{\"n\":\"Start\",\"t\":\"tT\",\"c\":1},"
		"{\"n\":\"Exptime\",\"t\":\"dur\",\"c\":2},"
		"{\"n\":\"Filter\",\"t\":\"s\",\"c\":3},"
		"{\"n\":\"Path\",\"t\":\"ip\",\"c\":4}"
	"],\"d\":[" << std::fixed;

	for (rts2db::ImageSet::iterator iter = img_set->begin (); iter != img_set->end (); iter++)
	{
		if (iter != img_set->begin ())
			os << ",";
		os << "[\"" << (*iter)->getFileName () << "\","
			<< JsonDouble ((*iter)->getExposureSec () + (*iter)->getExposureUsec () / USEC_SEC) << ","
			<< JsonDouble ((*iter)->getExposureLength ()) << ",\""
			<< (*iter)->getFilter () << "\",\""
			<< (*iter)->getAbsoluteFileName () << "\"]";
	}

	os << "]";
}
#endif // HAVE_PGSQL

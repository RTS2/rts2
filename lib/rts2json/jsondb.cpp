/* 
 * JSON database API calls.
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

#include "configuration.h"
#include "radecparser.h"

#include "rts2db/camlist.h"
#include "rts2db/constraints.h"
#include "rts2db/devicedb.h"
#include "rts2db/imageset.h"
#include "rts2db/labellist.h"
#include "rts2db/simbadtargetdb.h"
#include "rts2db/messagedb.h"
#include "rts2db/planset.h"
#include "rts2db/target_auger.h"
#include "rts2db/tletarget.h"
#include "rts2db/targetres.h"

#include "rts2json/jsondb.h"
#include "rts2json/jsonvalue.h"

#include "rts2script/script.h"

#include "xmlrpc++/XmlRpcException.h"

using namespace rts2json;

rts2db::Target * rts2json::getTarget (XmlRpc::HttpParams *params, const char *paramname)
{
	int id = params->getInteger (paramname, -1);
	if (id <= 0)
		throw XmlRpc::JSONException ("invalid id parameter");
	rts2db::Target *target = createTarget (id, rts2core::Configuration::instance ()->getObserver (), rts2core::Configuration::instance ()->getObservatoryAltitude ());
	if (target == NULL)
		throw XmlRpc::JSONException ("cannot find target with given ID");
	return target;
}

void JSONDBRequest::dbJSON (const std::vector <std::string> vals, XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, std::ostringstream &os)
{
	// returns all targets in database
	if (vals[0] == "tlist")
	{
		rts2db::TargetSet tar_set;
		tar_set.load ();
		jsonTargets (tar_set, os, params);
	}
	// returns target information specified by target name
	else if (vals[0] == "tbyname")
	{
		rts2db::TargetSet tar_set;
		const char *name = params->getString ("n", "");
		bool ic = params->getInteger ("ic",1);
		bool pm = params->getInteger ("pm",1);
		bool chunked = params->getInteger ("ch", 0);
		if (name[0] == '\0')
			throw XmlRpc::JSONException ("empty n parameter");
		tar_set.loadByName (name, pm, ic);
		if (chunked)
		{
			sendAsyncDataHeader (0, connection, "application/json");
			jsonTargets (tar_set, os, params, NULL, connection);
			connection->asyncFinished ();
			return;
		}
		else
		{
			jsonTargets (tar_set, os, params);
		}

	}
	// returns target specified by target ID
	else if (vals[0] == "tbyid")
	{
		rts2db::TargetSet tar_set;
		int id = params->getInteger ("id", -1);
		if (id <= 0)
			throw XmlRpc::JSONException ("empty id parameter");
		tar_set.load (id);
		jsonTargets (tar_set, os, params);
	}
	// returns target with given label
	else if (vals[0] == "tbylabel")
	{
		rts2db::TargetSet tar_set;
		int label = params->getInteger ("l", -1);
		if (label == -1)
			tar_set.loadByName ("%", false, false);
		else	
			tar_set.loadByLabelId (label);
		jsonTargets (tar_set, os, params);
	}
	// returns targets within certain radius from given ra dec
	else if (vals[0] == "tbydistance")
	{
		struct ln_equ_posn pos;
		pos.ra = params->getDouble ("ra", NAN);
		pos.dec = params->getDouble ("dec", NAN);
		double radius = params->getDouble ("radius", NAN);
		if (std::isnan (pos.ra) || std::isnan (pos.dec) || std::isnan (radius))
			throw XmlRpc::JSONException ("invalid ra, dec or radius parameter");
		rts2db::TargetSet ts (&pos, radius, rts2core::Configuration::instance ()->getObserver ());
		ts.load ();
		jsonTargets (ts, os, params, &pos);
	}
	// try to parse and understand string (similar to new target), return either target or all target information
	else if (vals[0] == "tbystring")
	{
		const char *tar_name = params->getString ("ts", "");
		if (tar_name[0] == '\0')
			throw XmlRpc::JSONException ("empty ts parameter");
		rts2db::Target *target = createTargetByString (tar_name, getServer ()->getDebug ());
		if (target == NULL)
			throw XmlRpc::JSONException ("cannot parse target");
		struct ln_equ_posn pos;
		target->getPosition (&pos);
		os << "\"name\":\"" << tar_name << "\",\"ra\":" << pos.ra << ",\"dec\":" << pos.dec << ",\"info\":\"" << target->getTargetInfo () << "\"";
		double nearest = params->getDouble ("nearest", -1);
		if (nearest >= 0)
		{
			os << ",\"nearest\":{";
			rts2db::TargetSet ts (&pos, nearest, rts2core::Configuration::instance ()->getObserver ());
			ts.load ();
			jsonTargets (ts, os, params, &pos);
			os << "}";
		}
	}
	else if (vals[0] == "ibyoid")
	{
		int obsid = params->getInteger ("oid", -1);
		if (obsid == -1)
			throw XmlRpc::JSONException ("empty oid parameter");
		rts2db::Observation obs (obsid);
		if (obs.load ())
			throw XmlRpc::JSONException ("Cannot load observation set");
		obs.loadImages ();	
		jsonImages (obs.getImageSet (), os, params);
	}
	else if (vals[0] == "labels")
	{
		const char *label = params->getString ("l", "");
		int t = params->getInteger ("t", -1);
		if (label[0] == '\0' && t < 0)
		{

		}
		if (label[0] == '\0')
			throw XmlRpc::JSONException ("empty l parameter");
		if (t < 0)
			throw XmlRpc::JSONException ("invalid type parametr");
		rts2db::Labels lb;
		os << "\"lid\":" << lb.getLabel (label, t);
	}
	else if (vals[0] == "consts")
	{
		rts2db::Target *target = getTarget (params);
		target->getConstraints ()->printJSON (os);
	}
	// violated constrainsts..
	else if (vals[0] == "violated")
	{
		const char *cn = params->getString ("consts", "");
		if (cn[0] == '\0')
			throw XmlRpc::JSONException ("unknow constraint name");
		rts2db::Target *tar = getTarget (params);
		double from = params->getDouble ("from", getNow ());
		double to = params->getDouble ("to", from + 86400);
		// 60 sec = 1 minute step (by default)
		double step = params->getDouble ("step", 60);

		rts2db::Constraints *cons = tar->getConstraints ();

		os << '"' << cn << "\":[";

		if (cons->find (std::string (cn)) != cons->end ())
		{
			rts2db::ConstraintPtr cptr = (*cons)[std::string (cn)];
			bool first_it = true;

			rts2db::interval_arr_t intervals;
			cptr->getViolatedIntervals (tar, from, to, step, intervals);
			for (auto iter : intervals)
			{
				if (first_it)
					first_it = false;
				else
					os << ",";
				os << "[" << iter.first << "," << iter.second << "]";
			}
		}
		os << "]";
	}
	// find unviolated time interval..
	else if (vals[0] == "satisfied")
	{
		rts2db::Target *tar = getTarget (params);
		time_t from = params->getDouble ("from", getNow ());
		time_t to = params->getDouble ("to", from + 86400);
		double length = params->getDouble ("length", 1800);
		int step = params->getInteger ("step", 60);

		rts2db::interval_arr_t si;
		from -= from % step;
		to += step - (to % step);
		tar->getSatisfiedIntervals (from, to, length, step, si);
		os << "\"id\":" << tar->getTargetID () << ",\"satisfied\":[";
		for (auto sat : si)
		{
			if (sat != *(si.begin ()))
				os << ",";
			os << "[" << sat.first << "," << sat.second << "]";
		}
		os << "]";
	}
	// return intervals of altitude constraints (or violated intervals)
	else if (vals[0] == "cnst_alt" || vals[0] == "cnst_alt_v")
	{
		rts2db::Target *tar = getTarget (params);

		std::map <std::string, std::vector <rts2db::ConstraintDoubleInterval> > ac;

		if (vals[0] == "cnst_alt")
			tar->getAltitudeConstraints (ac);
		else
			tar->getAltitudeViolatedConstraints (ac);

		os << "\"id\":" << tar->getTargetID () << ",\"altitudes\":{";

		for (auto iter = ac.begin(); iter != ac.end(); ++iter)
		{
			if (iter != ac.begin ())
				os << ",\"";
			else
				os << "\"";
			os << iter->first << "\":[";
			for (auto di = iter->second.begin (); di != iter->second.end (); ++di)
			{
				if (di != iter->second.begin ())
					os << ",[";
				else
					os << "[";
				os << rts2json::JsonDouble (di->getLower ()) << "," << rts2json::JsonDouble (di->getUpper ()) << "]";
			}
			os << "]";
		}
		os << "}";
	}
	// return intervals of time constraints (or violated time intervals)
	else if (vals[0] == "cnst_time" || vals[0] == "cnst_time_v")
	{
		rts2db::Target *tar = getTarget (params);
		time_t from = params->getInteger ("from", getNow ());
		time_t to = params->getInteger ("to", from + 86400);
		double steps = params->getDouble ("steps", 1000);

		steps = double ((to - from)) / steps;

		std::map <std::string, rts2db::ConstraintPtr> cons;

		tar->getTimeConstraints (cons);

		os << "\"id\":" << tar->getTargetID () << ",\"constraints\":{";

		for (auto iter = cons.begin (); iter != cons.end (); ++iter)
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

			for (auto it = intervals.begin (); it != intervals.end (); ++it)
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
	// resolve string to target name and coordinates
	// parameters:
	//    tn - string to resolve
	// returns:
	//    JSON array with target names, IDs (for existing targets), RA DEC of the possible target
	else if (vals[0] == "resolve")
	{
		const char *ts = params->getString ("s", "");
		if (strlen (ts) == 0)
			throw XmlRpc::JSONException ("empty target name");

		os << "\"data\":[";
	
		bool first = true;
		struct ln_equ_posn pos;

		// check if target with given name already exists in the database
		rts2db::TargetSetByName ts_n = rts2db::TargetSetByName (ts);
		ts_n.load ();
		if (ts_n.size () > 0)
		{
			for (auto iter = ts_n.begin (); iter != ts_n.end (); ++iter)
			{
				iter->second->getPosition (&pos);
				if (first)
					first = false;
				else
					os << ",";

				os << "[\"" << iter->second->getTargetName () << "\"," << iter->second->getTargetID () << "," << rts2json::JsonDouble (pos.ra) << "," << rts2json::JsonDouble (pos.dec) << "]";
			}
			os << "]";
		}
		else if (parseRaDec (ts, pos.ra, pos.dec) == 0)
		{
				os << "[\"Created " << ts << "\",-1," << pos.ra << "," << pos.dec << "]]";
		}
		else
		{
			rts2db::Target *target = new rts2db::SimbadTargetDb (ts);
			target->load ();
			target->getPosition (&pos);

			os << "[\"" << ts << "\",-2," << pos.ra << "," << pos.dec << "]]";
		}

	}
	else if (vals[0] == "create_target")
	{
		const char *tn = params->getString ("tn", "");
		double ra = params->getDouble ("ra", NAN);
		double dec = params->getDouble ("dec", NAN);
		const char *type = params->getString ("type", "O");
		const char *info = params->getString ("info", "");
		const char *comment = params->getString ("comment", "");

		if (strlen (tn) == 0)
			throw XmlRpc::JSONException ("empty target name");
		if (strlen (type) != 1)
			throw XmlRpc::JSONException ("invalid target type");

		rts2db::ConstTarget nt;
		nt.setTargetName (tn);
		nt.setPosition (ra, dec);
		nt.setTargetInfo (std::string (info));
		nt.setTargetComment (comment);
		nt.setTargetType (type[0]);
		nt.save (false);

		os << "\"id\":" << nt.getTargetID ();
	}
	else if (vals[0] == "create_tle_target")
	{
		const char *tn = params->getString ("tn", "");
		std::string tle1 = std::string (params->getString ("tle1", ""));
		std::string tle2 = std::string (params->getString ("tle2", ""));
		const char *comment = params->getString ("comment", "");

		rts2db::TLETarget nt;
		nt.setTargetName (tn);
		nt.setTargetInfo (tle1 + "|" + tle2);
		nt.setTargetComment (comment);
		nt.setTargetType (TYPE_TLE);
		nt.save (false);

		os << "\"id\":" << nt.getTargetID ();
	}
	else if (vals[0] == "update_target")
	{
		rts2db::Target *t = getTarget (params);
		switch (t->getTargetType ())
		{
			case TYPE_OPORTUNITY:
			case TYPE_GRB:
			case TYPE_FLAT:
			case TYPE_CALIBRATION:
			case TYPE_GPS:
			case TYPE_TERESTIAL:
			case TYPE_AUGER:
			case TYPE_LANDOLT:
			{
				rts2db::ConstTarget *tar = (rts2db::ConstTarget *) t;
				const char *tn = params->getString ("tn", "");
				double ra = params->getDouble ("ra", -1000);
				double dec = params->getDouble ("dec", -1000);
				double pm_ra = params->getDouble ("pm_ra", NAN);
				double pm_dec = params->getDouble ("pm_dec", NAN);
				bool enabled = params->getInteger ("enabled", tar->getTargetEnabled ());
				const char *info = params->getString ("info", NULL);

				if (strlen (tn) > 0)
					tar->setTargetName (tn);
				if (ra > -1000 && dec > -1000)
					tar->setPosition (ra, dec);
				if (!(std::isnan (pm_ra) && std::isnan (pm_dec)))
				{
					switch (tar->getTargetType ())
					{
						case TYPE_CALIBRATION:
						case TYPE_OPORTUNITY:
							((rts2db::ConstTarget *) (tar))->setProperMotion (pm_ra, pm_dec);
							break;
						default:
							throw XmlRpc::JSONException ("only calibration and oportunity targets can have proper motion");
					}
				}
				tar->setTargetEnabled (enabled, true);
				if (info != NULL)
					tar->setTargetInfo (std::string (info));
				tar->save (true);
	
				os << "\"id\":" << tar->getTargetID ();
				delete tar;
				break;
			}	
			default:
			{
				std::ostringstream _err;
				_err << "can update only subclass of constant targets, " << t->getTargetType () << " is unknow";
				throw XmlRpc::JSONException (_err.str ());
			}	
		}		

	}
	else if (vals[0] == "change_script")
	{
		rts2db::Target *tar = getTarget (params);
		const char *cam = params->getString ("c", NULL);
		if (strlen (cam) == 0)
			throw XmlRpc::JSONException ("unknow camera");
		const char *s = params->getString ("s", "");
		if (strlen (s) == 0)
			throw XmlRpc::JSONException ("empty script");

		rts2script::Script script (s);
		script.parseScript (tar);
		int failedCount = script.getFaultLocation ();
		if (failedCount != -1)
		{
			throw XmlRpc::JSONException (std::string ("canno parse script ") + s);
		}

		tar->setScript (cam, s);
		os << "\"id\":" << tar->getTargetID () << ",\"camera\":\"" << cam << "\",\"script\":\"" << s << "\"";
		delete tar;
	}
	else if (vals[0] == "change_constraints")
	{
		rts2db::Target *tar = getTarget (params);
		const char *cn = params->getString ("cn", NULL);
		const char *ci = params->getString ("ci", NULL);
		if (cn == NULL)
			throw XmlRpc::JSONException ("constraint not specified");
		rts2db::Constraints constraints;
		constraints.parse (cn, ci);
		tar->appendConstraints (constraints);

		os << "\"id\":" << tar->getTargetID () << ",";
		constraints.printJSON (os);

		delete tar;
	}
	else if (vals[0] == "tlabs_list")
	{
		rts2db::Target *tar = getTarget (params);
		jsonLabels (tar, os);
	}
	else if (vals[0] == "tlabs_delete")
	{
		rts2db::Target *tar = getTarget (params);
		int ltype = params->getInteger ("ltype", -1);
		if (ltype < 0)
			throw XmlRpc::JSONException ("unknow/missing label type");
		tar->deleteLabels (ltype);
		jsonLabels (tar, os);
	}
	else if (vals[0] == "tlabs_add" || vals[0] == "tlabs_set")
	{
		rts2db::Target *tar = getTarget (params);
		int ltype = params->getInteger ("ltype", -1);
		if (ltype < 0)
			throw XmlRpc::JSONException ("unknow/missing label type");
		const char *ltext = params->getString ("ltext", NULL);
		if (ltext == NULL)
			throw XmlRpc::JSONException ("missing label text");
		if (vals[0] == "tlabs_set")
			tar->deleteLabels (ltype);
		tar->addLabel (ltext, ltype, true);
		jsonLabels (tar, os);
	}
	else if (vals[0] == "obytid")
	{
		int tar_id = params->getInteger ("id", -1);
		if (tar_id < 0)
			throw XmlRpc::JSONException ("unknow target ID");
		rts2db::ObservationSet obss = rts2db::ObservationSet ();

		obss.loadTarget (tar_id);

		jsonObservations (&obss, os);
		
	}
	else if (vals[0] == "lastobs")
	{
		rts2db::Observation obs;

		int ret = obs.loadLastObservation ();
		if (ret)
			throw XmlRpc::JSONException ("cannot find last observation");

		jsonObservation (&obs, os);
	}
	else if (vals[0] == "obyid")
	{
		int obs_id = params->getInteger ("id", -1);
		if (obs_id < 0)
			throw XmlRpc::JSONException ("unknow observation ID");

		rts2db::Observation obs (obs_id);
		if (obs.load ())
			throw XmlRpc::JSONException ("cannot load observation with given ID");

		jsonObservation (&obs, os);
	}
	else if (vals[0] == "stat_obylid")
	{
		int label_id = params->getInteger ("id", -1);
		if (label_id < 0)
			throw XmlRpc::JSONException ("unknow label ID");

		double from = params->getDouble ("from", NAN);
		double to = params->getDouble ("to", NAN);

		rts2db::ImageSetLabel isl (label_id, from, to);
		if (isl.load ())
			throw XmlRpc::JSONException ("cannot load observations with given label ID");

		os << "\"observations\":" << isl.getAllStat ().count << ",\"skytime\":" << isl.getAllStat ().exposure;
	}
	else if (vals[0] == "plan")
	{
		rts2db::PlanSet ps (params->getDouble ("from", getNow ()), params->getDouble ("to", NAN));
		ps.load ();

		os << "\"h\":["
			"{\"n\":\"Plan ID\",\"t\":\"a\",\"prefix\":\"" << getServer ()->getPagePrefix () << "/plan/\",\"href\":0,\"c\":0},"
			"{\"n\":\"Target ID\",\"t\":\"a\",\"prefix\":\"" << getServer ()->getPagePrefix () << "/targets/\",\"href\":1,\"c\":1},"
			"{\"n\":\"Target Name\",\"t\":\"a\",\"prefix\":\"" << getServer ()->getPagePrefix () << "/targets/\",\"href\":1,\"c\":2},"
			"{\"n\":\"Start\",\"t\":\"t\",\"c\":4},"
			"{\"n\":\"End\",\"t\":\"t\",\"c\":5},"
			"{\"n\":\"RA\",\"t\":\"r\",\"c\":6},"
			"{\"n\":\"DEC\",\"t\":\"d\",\"c\":7},"
			"{\"n\":\"Alt start\",\"t\":\"altD\",\"c\":8},"
			"{\"n\":\"Az start\",\"t\":\"azD\",\"c\":9}],"
			"\"d\":[" << std::fixed;

		for (auto iter = ps.begin (); iter != ps.end (); ++iter)
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

		for (auto iter = ll.begin (); iter != ll.end (); ++iter)
		{
			if (iter != ll.begin ())
				os << ",";
			os << "[" << iter->labid << ","
				<< iter->tid << ",\""
				<< iter->text << "\"]";
		}
		os << "]";
	}
	else if (vals[0] == "messages")
	{
		double to = params->getDouble ("to", getNow ());
		double from = params->getDouble ("from", to - 86400);
		int typemask = params->getInteger ("type", MESSAGE_MASK_ALL);

		rts2db::MessageSet ms;
		ms.load (from, to, typemask);

		os << "\"h\":["
			"{\"n\":\"Time\",\"t\":\"t\",\"c\":0},"
			"{\"n\":\"Component\",\"t\":\"s\",\"c\":1},"
			"{\"n\":\"Type\",\"t\":\"n\",\"c\":2},"
			"{\"n\":\"Text\",\"t\":\"s\",\"c\":3}],"
			"\"d\":[";

		for (rts2db::MessageSet::iterator iter = ms.begin (); iter != ms.end (); iter++)
		{
			if (iter != ms.begin ())
				os << ",";
			os << "[" << iter->getMessageTime () << ",\"" << JsonString (iter->getMessageOName ()) << "\"," << iter->getType () << ",\"" << JsonString (iter->getMessageString ()) << "\"]";
		}
		os << "]";
	}
	else if (vals[0] == "auger")
	{
		int a_id = params->getInteger ("id", -1);
		rts2db::TargetAuger ta (-1, rts2core::Configuration::instance ()->getObserver (), rts2core::Configuration::instance ()->getObservatoryAltitude (), 10);
		if (a_id < 0)
		{
			a_id = params->getInteger ("obs_id", -1);
			if (a_id < 0)
				throw XmlRpc::JSONException ("invalid observation ID");
			ta.loadByOid (a_id);
		}
		else
		{
			ta.load (a_id);
		}

		os << "\"t3id\":" << ta.t3id 
			<< ",\"auger_date\":" << rts2json::JsonDouble (ta.auger_date)
			<< ",\"Eye\":" << ta.Eye
			<< ",\"Run\":" << ta.Run
			<< ",\"Event\":" << ta.Event
			<< ",\"AugerId\":\"" << ta.AugerId 
			<< "\",\"GPSSec\":" << ta.GPSSec
			<< ",\"GPSNSec\":" << ta.GPSNSec
			<< ",\"SDId\":" << ta.SDId
			<< ",\"NPix\":" << ta.NPix

			<< ",\"SDPTheta\":" << rts2json::JsonDouble (ta.SDPTheta)       /// Zenith angle of SDP normal vector (deg)
			<< ",\"SDPThetaErr\":" << rts2json::JsonDouble (ta.SDPThetaErr)    /// Uncertainty of SDPtheta
			<< ",\"SDPPhi\":" << rts2json::JsonDouble (ta.SDPPhi)         /// Azimuth angle of SDP normal vector (deg)
			<< ",\"SDPPhiErr\":" << rts2json::JsonDouble (ta.SDPPhiErr)      /// Uncertainty of SDPphi
			<< ",\"SDPChi2\":" << rts2json::JsonDouble (ta.SDPChi2)        /// Chi^2 of SDP db_it
			<< ",\"SDPNdf\":" << ta.SDPNdf         /// Degrees of db_reedom of SDP db_it

			<< ",\"Rp\":" << rts2json::JsonDouble (ta.Rp)             /// Shower impact parameter Rp (m)
			<< ",\"RpErr\":" << rts2json::JsonDouble (ta.RpErr)          /// Uncertainty of Rp (m)
			<< ",\"Chi0\":" << rts2json::JsonDouble (ta.Chi0)           /// Angle of shower in the SDP (deg)
			<< ",\"Chi0Err\":" << rts2json::JsonDouble (ta.Chi0Err)        /// Uncertainty of Chi0 (deg)
			<< ",\"T0\":" << rts2json::JsonDouble (ta.T0)             /// FD time db_it T_0 (ns)
			<< ",\"T0Err\":" << rts2json::JsonDouble (ta.T0Err)          /// Uncertainty of T_0 (ns)
			<< ",\"TimeChi2\":" << rts2json::JsonDouble (ta.TimeChi2)       /// Full Chi^2 of axis db_it
			<< ",\"TimeChi2FD\":" << rts2json::JsonDouble (ta.TimeChi2FD)     /// Chi^2 of axis db_it (FD only)
			<< ",\"TimeNdf\":" << ta.TimeNdf        /// Degrees of db_reedom of axis db_it

			<< ",\"Easting\":" << rts2json::JsonDouble (ta.Easting)        /// Core position in easting coordinate (m)
			<< ",\"Northing\":" << rts2json::JsonDouble (ta.Northing)       /// Core position in northing coordinate (m)
			<< ",\"Altitude\":" << rts2json::JsonDouble (ta.Altitude)       /// Core position altitude (m)
			<< ",\"NorthingErr\":" << rts2json::JsonDouble (ta.NorthingErr)    /// Uncertainty of northing coordinate (m)
			<< ",\"EastingErr\":" << rts2json::JsonDouble (ta.EastingErr)     /// Uncertainty of easting coordinate (m)
			<< ",\"Theta\":" << rts2json::JsonDouble (ta.Theta)          /// Shower zenith angle in core coords. (deg)
			<< ",\"ThetaErr\":" << rts2json::JsonDouble (ta.ThetaErr)       /// Uncertainty of zenith angle (deg)
			<< ",\"Phi\":" << rts2json::JsonDouble (ta.Phi)            /// Shower azimuth angle in core coords. (deg)
			<< ",\"PhiErr\":" << rts2json::JsonDouble (ta.PhiErr)         /// Uncertainty of azimuth angle (deg)

			<< ",\"dEdXmax\":" << rts2json::JsonDouble (ta.dEdXmax)        /// Energy deposit at shower max (GeV/(g/cm^2))
			<< ",\"dEdXmaxErr\":" << rts2json::JsonDouble (ta.dEdXmaxErr)     /// Uncertainty of Nmax (GeV/(g/cm^2))
			<< ",\"Xmax\":" << rts2json::JsonDouble (ta.Xmax)           /// Slant depth of shower maximum (g/cm^2)
			<< ",\"XmaxErr\":" << rts2json::JsonDouble (ta.XmaxErr)        /// Uncertainty of Xmax (g/cm^2)
			<< ",\"X0\":" << rts2json::JsonDouble (ta.X0)             /// X0 Gaisser-Hillas db_it (g/cm^2)
			<< ",\"X0Err\":" << rts2json::JsonDouble (ta.X0Err)          /// Uncertainty of X0 (g/cm^2)
			<< ",\"Lambda\":" << rts2json::JsonDouble (ta.Lambda)         /// Lambda of Gaisser-Hillas db_it (g/cm^2)
			<< ",\"LambdaErr\":" << rts2json::JsonDouble (ta.LambdaErr)      /// Uncertainty of Lambda (g/cm^2)
			<< ",\"GHChi2\":" << rts2json::JsonDouble (ta.GHChi2)         /// Chi^2 of Gaisser-Hillas db_it
			<< ",\"GHNdf\":" << ta.GHNdf          /// Degrees of db_reedom of GH db_it
			<< ",\"LineFitChi2\":" << rts2json::JsonDouble (ta.LineFitChi2)    /// Chi^2 of linear db_it to profile

			<< ",\"EmEnergy\":" << rts2json::JsonDouble (ta.EmEnergy)       /// Calorimetric energy db_rom GH db_it (EeV)
			<< ",\"EmEnergyErr\":" << rts2json::JsonDouble (ta.EmEnergyErr)    /// Uncertainty of Eem (EeV)
			<< ",\"Energy\":" << rts2json::JsonDouble (ta.Energy)         /// Total energy db_rom GH db_it (EeV)
			<< ",\"EnergyErr\":" << rts2json::JsonDouble (ta.EnergyErr)      /// Uncertainty of Etot (EeV)

			<< ",\"MinAngle\":" << rts2json::JsonDouble (ta.MinAngle)       /// Minimum viewing angle (deg)
			<< ",\"MaxAngle\":" << rts2json::JsonDouble (ta.MaxAngle)       /// Maximum viewing angle (deg)
			<< ",\"MeanAngle\":" << rts2json::JsonDouble (ta.MeanAngle)      /// Mean viewing angle (deg)
	
			<< ",\"NTank\":" << ta.NTank          /// Number of stations in hybrid db_it
			<< ",\"HottestTank\":" << ta.HottestTank    /// Station used in hybrid-geometry reco
			<< ",\"AxisDist\":" << rts2json::JsonDouble (ta.AxisDist)       /// Shower axis distance to hottest station (m)
			<< ",\"SDPDist\":" << rts2json::JsonDouble (ta.SDPDist)        /// SDP distance to hottest station (m)

			<< ",\"SDFDdT\":" << rts2json::JsonDouble (ta.SDFDdT)         /// SD/FD time offset after the minimization (ns)
			<< ",\"XmaxEyeDist\":" << rts2json::JsonDouble (ta.XmaxEyeDist)    /// Distance to shower maximum (m)
			<< ",\"XTrackMin\":" << rts2json::JsonDouble (ta.XTrackMin)      /// First recorded slant depth of track (g/cm^2)
			<< ",\"XTrackMax\":" << rts2json::JsonDouble (ta.XTrackMax)      /// Last recorded slant depth of track (g/cm^2)
			<< ",\"XFOVMin\":" << rts2json::JsonDouble (ta.XFOVMin)        /// First slant depth inside FOV (g/cm^2)
			<< ",\"XFOVMax\":" << rts2json::JsonDouble (ta.XFOVMax)        /// Last slant depth inside FOV (g/cm^2)
			<< ",\"XTrackObs\":" << rts2json::JsonDouble (ta.XTrackObs)      /// Observed track length depth (g/cm^2)
			<< ",\"DegTrackObs\":" << rts2json::JsonDouble (ta.DegTrackObs)    /// Observed track length angle (deg)
			<< ",\"TTrackObs\":" << rts2json::JsonDouble (ta.TTrackObs)      /// Observed track length time (100 ns)

			<< ",\"cut\":" << ta.cut               /// Cuts pased by shower

			<< ",\"profile\":[";

		for (std::vector <std::pair <double, double> >::iterator iter = ta.showerparams.begin (); iter != ta.showerparams.end (); iter++)
		{
			if (iter != ta.showerparams.begin ())
				os << ",";
			os << "[" << iter->first << "," << iter->second << "]";
		}
		os << "]";
	}
	else
	{
		throw XmlRpc::JSONException ("invalid request " + path);
	}
}

void JSONDBRequest::jsonTargets (rts2db::TargetSet &tar_set, std::ostringstream &os, XmlRpc::HttpParams *params, struct ln_equ_posn *dfrom, XmlRpc::XmlRpcServerConnection * chunked)
{
	bool extended = params->getInteger ("e", false);
	bool withpm = params->getInteger ("propm", false);
	time_t from = params->getInteger ("from", getNow ());
	int c = 5;
	if (params->getInteger ("jqapi", 0))
	{
		os << "\"sEcho\":" << params->getInteger ("sEcho", 0) << ",\"iTotalRecords\":\"" << tar_set.size () << "\",\"iTotalDisplayRecords\":\"" << tar_set.size () << "\","
	"\"aaData\" : [";
	}
	else
	{
		if (chunked)
			os << "\"rows\":" << tar_set.size () << ",";

		os << "\"h\":["
			"{\"n\":\"Target ID\",\"t\":\"n\",\"prefix\":\"" << getServer ()->getPagePrefix () << "/targets/\",\"href\":0,\"c\":0},"
			"{\"n\":\"Target Name\",\"t\":\"a\",\"prefix\":\"" << getServer ()->getPagePrefix () << "/targets/\",\"href\":0,\"c\":1},"
			"{\"n\":\"RA\",\"t\":\"r\",\"c\":2},"
			"{\"n\":\"DEC\",\"t\":\"d\",\"c\":3},"
			"{\"n\":\"Description\",\"t\":\"s\",\"c\":4}";

		struct ln_equ_posn oradec;

		if (dfrom == NULL)
		{
				oradec.ra = params->getDouble ("ra", NAN);
				oradec.dec = params->getDouble ("dec", NAN);
				if (!std::isnan (oradec.ra) && !std::isnan (oradec.dec))
					dfrom = &oradec;
		}
		if (dfrom)
		{
			os << ",{\"n\":\"Distance\",\"t\":\"d\",\"c\":" << (c) << "}";
			c++;
		}
		if (extended)
		{
			os << ",{\"n\":\"Duration\",\"t\":\"dur\",\"c\":" << (c) << "},"
			"{\"n\":\"Scripts\",\"t\":\"scripts\",\"c\":" << (c + 1) << "},"
			"{\"n\":\"Satisfied\",\"t\":\"s\",\"c\":" << (c + 2) << "},"
			"{\"n\":\"Violated\",\"t\":\"s\",\"c\":" << (c + 3) << "},"
			"{\"n\":\"Transit\",\"t\":\"t\",\"c\":" << (c + 4) << "},"
			"{\"n\":\"Observable\",\"t\":\"t\",\"c\":" << (c + 5) << "}";
			c += 6;
		}
		if (withpm)
		{
			os << ",{\"n\":\"Proper motion RA\",\"t\":\"d\",\"c\":" << (c) << "},"
			"{\"n\":\"Proper motion DEC\",\"t\":\"d\",\"c\":" << (c + 1) << "}";
			c += 2;
		}

		if (chunked)
		{
			os << "]}";
			chunked->sendChunked (os.str ());
			os.str ("");
			os << std::fixed;
		}
		else
		{
			os << "],\"d\":[" << std::fixed;
		}
	}

	double JD = ln_get_julian_from_timet (&from);
	for (rts2db::TargetSet::iterator iter = tar_set.begin (); iter != tar_set.end (); iter++)
	{
		if (iter != tar_set.begin () && chunked == NULL)
			os << ",";
		struct ln_equ_posn equ;
		rts2db::Target *tar = iter->second;
		tar->getPosition (&equ, JD);
		const char *n = tar->getTargetName ();
		os << "[" << tar->getTargetID () << ",";
		if (n == NULL)
			os << "null,";
		else
			os << "\"" << rts2json::JsonString (n) << "\",";

		os << rts2json::JsonDouble (equ.ra) << ',' << rts2json::JsonDouble (equ.dec) << ",\"" << rts2json::JsonString (tar->getTargetInfo ()) << "\"";

		if (dfrom)
			os << ',' << rts2json::JsonDouble (tar->getDistance (dfrom, JD));

		if (extended)
		{
			double md = -1;
			std::ostringstream cs;
			for (auto cam : (*(getServer()->getCameras())))
			{
				try
				{
					std::string script_buf;
					rts2script::Script script;
					tar->getScript (cam.c_str(), script_buf);
					script.setTarget (cam.c_str (), tar);
					double d = script.getExpectedDuration ();
					int e = script.getExpectedImages ();
					if (cam != *(getServer ()->getCameras ()->begin ()))
						cs << ",";
					cs << "{\"" << cam << "\":[\"" << script_buf << "\"," << d << "," << e << "]}";
					if (d > md)
						md = d;  
				}
				catch (rts2core::Error &er)
				{
					logStream (MESSAGE_ERROR) << "cannot parsing script for camera " << cam << ": " << er << sendLog;
				}
			}
			os << "," << md << ",[" << cs.str () << "],";

			tar->getSatisfiedConstraints (JD).printJSON (os);
			
			os << ",";
		
			tar->getViolatedConstraints (JD).printJSON (os);

			if (std::isnan (equ.ra) || std::isnan (equ.dec))
			{
				os << ",null,true";
			}
			else
			{
				struct ln_hrz_posn hrz;
				tar->getAltAz (&hrz, JD);

				struct ln_rst_time rst;
				tar->getRST (&rst, JD, 0);

				os << "," << rts2json::JsonDouble (rst.transit) 
					<< "," << (tar->isAboveHorizon (&hrz) ? "true" : "false");
			}
		}
		if (withpm)
		{
			struct ln_equ_posn pm;
			switch (tar->getTargetType ())
			{
				case TYPE_CALIBRATION:
				case TYPE_OPORTUNITY:
					((rts2db::ConstTarget *) tar)->getProperMotion (&pm);
					break;
				default:
					pm.ra = pm.dec = NAN;
			}

			os << "," << rts2json::JsonDouble (pm.ra) << "," << rts2json::JsonDouble (pm.dec);
		}
		os << "]";
		if (chunked)
		{
			chunked->sendChunked (os.str ());
			os.str ("");
		}
	}
	if (chunked == NULL)
	{
		os << "]";
	}	
}

void JSONDBRequest::jsonObservation (rts2db::Observation *obs, std::ostream &os)
{
	struct ln_equ_posn equ;
	obs->getTarget ()->getPosition (&equ);

	os << std::fixed << "\"observation\":{\"id\":" << obs->getObsId ()
		<< ",\"slew\":" << rts2json::JsonDouble (obs->getObsSlew ())
		<< ",\"start\":" << rts2json::JsonDouble (obs->getObsStart ())
		<< ",\"end\":" << rts2json::JsonDouble (obs->getObsEnd ())
		<< ",\"images\":" << obs->getNumberOfImages ()
		<< ",\"good\":" << obs->getNumberOfGoodImages ()
		<< "},\"target\":{\"id\":" << obs->getTarget ()->getTargetID ()
		<< ",\"name\":\"" << rts2json::JsonString (obs->getTarget ()->getTargetName ())
		<< "\",\"ra\":" << rts2json::JsonDouble (equ.ra)
		<< ",\"dec\":" << rts2json::JsonDouble (equ.dec)
		<< ",\"description\":\"" << rts2json::JsonString (obs->getTarget ()->getTargetInfo ())
		<< "\"}";
}

void JSONDBRequest::jsonObservations (rts2db::ObservationSet *obss, std::ostream &os)
{
	os << "\"h\":["
		"{\"n\":\"ID\",\"t\":\"n\",\"c\":0,\"prefix\":\"" << getServer ()->getPagePrefix () << "/observations/\",\"href\":0},"
		"{\"n\":\"Slew\",\"t\":\"t\",\"c\":1},"
		"{\"n\":\"Start\",\"t\":\"tT\",\"c\":2},"
		"{\"n\":\"End\",\"t\":\"tT\",\"c\":3},"
		"{\"n\":\"Number of images\",\"t\":\"n\",\"c\":4},"
		"{\"n\":\"Number of good images\",\"t\":\"n\",\"c\":5}"
		"],\"d\":[";

	os << std::fixed;

	for (rts2db::ObservationSet::iterator iter = obss->begin (); iter != obss->end (); iter++)
	{
		if (iter != obss->begin ())
			os << ",";
		os << "[" << iter->getObsId () << ","
			<< rts2json::JsonDouble(iter->getObsSlew ()) << ","
			<< rts2json::JsonDouble(iter->getObsStart ()) << ","
			<< rts2json::JsonDouble(iter->getObsEnd ()) << ","
			<< iter->getNumberOfImages () << ","
			<< iter->getNumberOfGoodImages ()
			<< "]";
	}

	os << "]";
}

void JSONDBRequest::jsonImages (rts2db::ImageSet *img_set, std::ostream &os, XmlRpc::HttpParams *params)
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
			<< rts2json::JsonDouble ((*iter)->getExposureSec () + (*iter)->getExposureUsec () / USEC_SEC) << ","
			<< rts2json::JsonDouble ((*iter)->getExposureLength ()) << ",\""
			<< (*iter)->getFilter () << "\",\""
			<< (*iter)->getAbsoluteFileName () << "\"]";
	}

	os << "]";
}

void JSONDBRequest::jsonLabels (rts2db::Target *tar, std::ostream &os)
{
	rts2db::LabelsVector tlabels = tar->getLabels ();
	os << "\"id\":" << tar->getTargetID () << ",\"labels\":[";
	for (rts2db::LabelsVector::iterator iter = tlabels.begin (); iter != tlabels.end (); iter++)
	{
		if (iter == tlabels.begin ())
			os << "[";
		else
			os << ",[";
		os << iter->lid << "," << iter->ltype << ",\"" << iter->ltext << "\"]";
	}
	os << "]";
}

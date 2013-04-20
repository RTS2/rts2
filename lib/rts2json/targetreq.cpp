/*
 * Target display and manipulation.
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

#include "rts2json/altplot.h"
#include "rts2json/imgpreview.h"
#include "rts2json/targetreq.h"
#include "dirsupport.h"
#include "xmlrpc++/urlencoding.h"
#include "command.h"

#ifdef RTS2_HAVE_PGSQL
#include "rts2db/observationset.h"
#include "rts2db/imageset.h"
#include "rts2db/targetset.h"
#include "rts2db/constraints.h"
#include "rts2db/planset.h"
#include "rts2db/labels.h"
#endif /* RTS2_HAVE_PGSQL */

using namespace rts2json;

#ifdef RTS2_HAVE_PGSQL

Targets::Targets (const char *prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer *s):GetRequestAuthorized (prefix, _http_server, "target list", s)
{
	displaySeconds = false;
}

void Targets::authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	// get path and possibly date range
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));

	if (vals.size () == 0)
	{
		listTargets (params, response_type, response, response_length);
	}
	else
	{
		if (vals[0] == "form")
		{
			processForm (params, response_type, response, response_length);
			return;
		}
		if (vals[0] == "api")
		{
			processAPI (params, response_type, response, response_length);
			return;
		}
		rts2db::Target *tar = NULL;
		// get target id..
		char *endptr;
		int tar_id = strtol (vals[0].c_str (), &endptr, 10);
		if (*endptr != '\0')
		{
			rts2db::TargetSet ts (rts2core::Configuration::instance()->getObserver ());
			ts.loadByName (vals[0].c_str ());
			if (ts.size () != 1)
				throw rts2core::Error ("Cannot find target with name" + vals[0]);
			tar = ts.begin ()->second;
			ts.clear ();
		}
		else
		{
			if (tar_id < 0)
				throw rts2core::Error ("Target id < 0");

			tar = createTarget (tar_id, rts2core::Configuration::instance ()->getObserver ());
		}	
		if (tar == NULL)
			throw rts2core::Error ("Cannot find target with given ID");

		switch (vals.size ())
		{
			case 1:
				printTarget (tar, response_type, response, response_length);
				break;
			case 2:
				if (vals[1] == "api")
				{
					callAPI (tar, params, response_type, response, response_length);
					break;
				}
				if (vals[1] == "main")
				{
					printTarget (tar, response_type, response, response_length);
					break;
				}
				if (vals[1] == "details")
				{
					printTargetInfo (tar, response_type, response, response_length);
					break;
				}
				if (vals[1] == "stat")
				{
					printTargetStat (tar, response_type, response, response_length);
					break;
				}
				if (vals[1] == "images")
				{
					printTargetImages (tar, params, response_type, response, response_length);
					break;
				}
				if (vals[1] == "obs")
				{
					printTargetObservations (tar, response_type, response, response_length);
					break;
				}
				if (vals[1] == "plan")
				{
					printTargetPlan (tar, response_type, response, response_length);
					break;
				}
#ifdef RTS2_HAVE_LIBJPEG
				if (vals[1] == "altplot")
				{
					plotTarget (tar, params, response_type, response, response_length);
					break;
				}
#endif /* RTS2_HAVE_LIBJPEG */
			case 3:
				if (vals[1] == "api")
				{
					callTargetAPI (tar, vals[2], params, response_type, response, response_length);
					break;
				}
			default:
				throw rts2core::Error ("Invalid path!");
		}
	}
}

void Targets::listTargets (XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;
	printHeader (_os, "List of targets", NULL, "/css/datatables.css");

	includeJavaScript (_os, "equ.js");
	includeJavaScript (_os, "jquery.js");
	includeJavaScript (_os, "datatables.js");

	_os <<
		"<script type='text/javascript' charset='utf-8'>\n"
			"$(document).ready(function() {\n"
				"$('#targets').dataTable( {\n"
					"'bProcessing': true,\n"
					"'sAjaxSource': 'api',\n"
					"'bPaginate': false,\n"
					"'aoColumnDefs': [{\n"
						"'aTargets' : [2],\n"
						"'mRender': function(data,type,full) {\n"
							"return (new HMS(data)).toString();\n"
						"}\n"
					"},{\n"
						"'aTargets' : [3],\n"
						"'mRender': function(data,type,full) {\n"
							"return (new DMS(data)).toString();\n"
						"}\n"
					"},{\n"
						"'aTargets' : [4],\n"
						"'mRender': function(data,type,full) {\n"
							"return (new DMS(data)).toStringSignedDeg();\n"
						"}\n"
					"},{\n"
						"'aTargets' : [5],\n"
						"'mRender': function(data,type,full) {\n"
							"return (new DMS(data)).toStringDeg();\n"
						"}\n"
					"}]\n"
				"} );\n"
			"} );\n"
		"</script>\n"

		"<table cellpadding='0' cellspacing='0' border='0' class='display' id='targets'>\n"
			"<thead>\n"
				"<tr>\n"
					"<th>ID</th>\n"
					"<th>Target name</th>\n"
					"<th>RA</th>\n"
					"<th>DEC</th>\n"
					"<th>Alt</th>\n"
					"<th>Az</th>\n"
					"<th>Priority</th>\n"
					"<th>Bonus</th>\n"
					"<th>Enabled</th>\n"
					"<th>Violated</th>\n"
					"<th>Satisfied</th>\n"
					"<th>NOBS</th>\n"
				"</tr>\n"
			"</thead>\n"
			"<tbody>\n"
				"<tr>\n"
					"<td colspan='5' class='dataTables_empty'>Loading data from server</td>\n"
				"</tr>\n"
			"</tbody>\n"
			"<tfoot>\n"
				"<tr>\n"
					"<th>ID</th>\n"
					"<th>Target name</th>\n"
					"<th>RA</th>\n"
					"<th>DEC</th>\n"
					"<th>Alt</th>\n"
					"<th>Az</th>\n"
					"<th>Priority</th>\n"
					"<th>Bonus</th>\n"
					"<th>Enabled</th>\n"
					"<th>Violated</th>\n"
					"<th>Satisfied</th>\n"
					"<th>NOBS</th>\n"
				"</tr>\n"
			"</tfoot>\n"
		"</table>\n";

#ifdef RTS2_HAVE_LIBJPEG
	_os << "<form action='form' method='GET'>\n"
	"<input type='submit' value='Plot target altitude' name='plot'/> (please select at least one target to plot something interesting)\n"
	"</form>\n";
#endif // RTS2_HAVE_LIBJPEG
	
	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Targets::processForm (XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
#ifdef RTS2_HAVE_LIBJPEG
	if (!strcmp (params->getString ("plot", "xxx"), "Plot target altitude"))
	{
		response_type = "image/jpeg";

		AltPlot ap (params->getInteger ("w", 800), params->getInteger ("h", 600));
		Magick::Geometry size (params->getInteger ("w", 800), params->getInteger ("h", 600));
	
		double from = params->getDouble ("from", 0);
		double to = params->getDouble ("to", 0);
	
		if (from < 0 && to == 0)
		{
			// just fr specified - from
			to = time (NULL);
			from += to;
		}
		else if (from == 0 && to == 0)
		{
			// default - one hour
			to = time (NULL);
			from = to - 86400;
		}


		std::list < int > ti;

		for (XmlRpc::HttpParams::iterator iter = params->begin (); iter != params->end (); iter++)
		{
			if (!strcmp (iter->getName (), "tid"))
				ti.push_back (atoi (iter->getValue ()));
		}

		rts2db::TargetSet ts = rts2db::TargetSet ();
		ts.load (ti);
	
		Magick::Image mimage (size, "white");
		ap.getPlot (from, to, &ts, &mimage);
	
		Magick::Blob blob;
		mimage.write (&blob, "jpeg");
	
		response_length = blob.length();
		response = new char[response_length];
		memcpy (response, blob.data(), response_length);
		return;
	}
#endif // RTS2_HAVE_LIBJPEG
}

void Targets::processAPI (XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	const char *t = params->getString ("t", NULL);

	double JD = params->getDouble ("jd", ln_get_julian_from_sys ());

	rts2db::TargetSet ts;

	if (t != NULL)
		ts = rts2db::TargetSet (t);
	else
		ts = rts2db::TargetSet ();
	ts.load ();

	_os << "{\"sEcho\":" << params->getInteger ("sEcho", 0) << ",\"iTotalRecords\":\"" << ts.size () << "\",\"iTotalDisplayRecords\":\"" << ts.size () << "\","
"\"aaData\" : [";

	_os << std::fixed;

	for (rts2db::TargetSet::iterator iter = ts.begin (); iter != ts.end (); iter++)
	{
		struct ln_equ_posn pos;
		struct ln_hrz_posn hrz;

		rts2db::Target *tar = iter->second;

		tar->getPosition (&pos, JD);
		tar->getAltAz (&hrz, JD);
		if (iter != ts.begin ())
			_os << ",";
		const char *tar_name = tar->getTargetName ();
		_os << "[" << tar->getTargetID () << ",\"" << (tar_name == NULL ? "(null)" : tar_name) << "\",";
		if (isnan (pos.ra) || isnan (pos.dec) || isnan (hrz.alt) || isnan (hrz.az))
			_os << "0,0,0,0";
		else
			_os << pos.ra << "," << pos.dec << "," << hrz.alt << "," << hrz.az;

		_os << "," << std::setprecision (3) << tar->getTargetPriority ()
			<< ",";
		double b = tar->getBonus (JD);
		if (isnan (b))
			_os << -1;
		else
			_os << tar->getBonus ();
		
		_os << "," << tar->getTargetEnabled ()
			<< ",\"" << tar->getViolatedConstraints (JD)
			<< "\",\"" << tar->getSatisfiedConstraints (JD)
			<< "\"," << tar->getTotalNumberOfObservations ()
		//	<< "," << tar->getFirstObs ()
		//	<< "," << tar->getLastObsTime ()
			<< "]";
	}

	_os << "] }";

	returnJSON (_os, response_type, response, response_length);
}

void Targets::printTargetHeader (int tar_id, const char *current, std::ostringstream &_os)
{
	std::ostringstream prefix;
	prefix << getServer ()->getPagePrefix () << "/targets/" << tar_id << "/";

	_os << "<p>";

	const char *subm[][2] = {{"main", "main page"},{"details", "details"},{"stat", "statistics"},{"images", "images"},{"obs", "observations"},{"plan", "plan"},{"altplot","altitude plot"},{NULL, NULL}};

	printSubMenus (_os, prefix.str ().c_str (), current, subm);
}

void Targets::callAPI (rts2db::Target *tar, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	const char *e = params->getString ("jd", NULL);
	if (e != NULL)
	{
		// return target values in API format..
		std::ostringstream os;
		struct ln_equ_posn tp;
		struct ln_hrz_posn hp;
		double JD = NAN;
		JD = atof (e);
		if (isnan (JD))
			JD = ln_get_julian_from_sys ();

		tar->getPosition (&tp, JD);
		tar->getAltAz (&hp, JD);
		os << std::fixed << "{\"name\":\"" << tar->getTargetName ()
			<< "\",\"id\":" << tar->getTargetID ()
			<< ",\"ra\":" << tp.ra << ",\"dec\":" << tp.dec 
			<< ",\"alt\":" << hp.alt << ",\"az\":" << hp.az
			<< ",\"airmass\":" << tar->getAirmass (JD) << ",\"satisfied\":";
		
		tar->getSatisfiedConstraints (JD).printJSON (os);

		os << ",\"violated\":";

		tar->getViolatedConstraints (JD).printJSON (os);

		struct ln_rst_time rst;
		tar->getRST (&rst, JD, 0);

		os << ",\"transit\":" << rst.transit << ",\"is_above\":" << tar->isAboveHorizon (&hp) << "}";

		returnJSON (os, response_type, response, response_length);
		return;
	}	
	if (!canExecute ())
		throw XmlRpc::XmlRpcException ("you do not have permission to change anything");
	e = params->getString ("e", NULL);
	if (e != NULL)
	{
		tar->setTargetEnabled (!strcmp (e, "true"), true);
		tar->save (true);
		returnJSON ("1", response_type, response, response_length);
		return;
	}
	e = params->getString ("slew", NULL);
	if (e != NULL)
	{
		rts2core::connections_t::iterator iter = getServer ()->getConnections ()->begin ();
		getServer ()->getOpenConnectionType (DEVICE_TYPE_MOUNT, iter);
		if (iter == getServer ()->getConnections ()->end ())
		  	throw XmlRpc::XmlRpcException ("Telescope is not connected");
		struct ln_equ_posn pos;
		tar->getPosition (&pos);
		(*iter)->queCommand (new rts2core::CommandMove (((rts2core::Block *) getMasterApp ()), NULL, pos.ra, pos.dec));

		// return status..
		rts2core::ValueRaDec *telRaDec = (rts2core::ValueRaDec *) ((*iter)->getValueType ("TEL", RTS2_VALUE_RADEC));
		rts2core::ValueAltAz *telAltAz = (rts2core::ValueAltAz *) ((*iter)->getValueType ("TEL_", RTS2_VALUE_RADEC));
		rts2core::ValueRaDec *tarRaDec = (rts2core::ValueRaDec *) ((*iter)->getValueType ("TAR", RTS2_VALUE_RADEC));

		std::ostringstream os;
		os << "{\"remaining\":" << ((*iter)->getValueDouble ("move_end") - getNow ())
			<< ", \"tel\": {\"ra\":" << telRaDec->getRa () << ", \"dec\":" << telRaDec->getDec ()
			<< ", \"alt\":" << telAltAz->getAlt () << ", \"az\":" << telAltAz->getAz ()
			<< "}, \"tar\": {\"ra\":" << tarRaDec->getRa () << ", \"dec\":" << tarRaDec->getDec ()
			<< "} }";
		returnJSON (os.str ().c_str (), response_type, response, response_length);
		return;
	}
	e = params->getString ("next", NULL);
	if (e != NULL)
	{
		rts2core::connections_t::iterator iter = getServer ()->getConnections ()->begin ();
		getServer ()->getOpenConnectionType (DEVICE_TYPE_EXECUTOR, iter);
		if (iter == getServer ()->getConnections ()->end ())
		  	throw XmlRpc::XmlRpcException ("Executor is not running");
		(*iter)->queCommand (new rts2core::CommandExecNext (((rts2core::Block *) getMasterApp ()), tar->getTargetID ()));

		std::ostringstream os;
		os << "{\"status\":0}";
		returnJSON (os.str ().c_str (), response_type, response, response_length);
		return;
	}
	e = params->getString ("now", NULL);
	if (e != NULL)
	{
		rts2core::connections_t::iterator iter = getServer ()->getConnections ()->begin ();
		getServer ()->getOpenConnectionType (DEVICE_TYPE_EXECUTOR, iter);
		if (iter == getServer ()->getConnections ()->end ())
		  	throw XmlRpc::XmlRpcException ("Executor is not running");
		(*iter)->queCommand (new rts2core::CommandExecNow (((rts2core::Block *) getMasterApp ()), tar->getTargetID ()));

		returnJSON ("{\"status\":0}", response_type, response, response_length);
		return;
	}
	e = params->getString ("script", NULL);
	const char *c = params->getString ("camera", NULL);
	if (e != NULL && c != NULL)
	{
		tar->setScript (c, e);
		returnJSON ("{\"status\":0}", response_type, response, response_length);
		return;
	}
	throw XmlRpc::XmlRpcException ("invalid API request");
}


void Targets::callTargetAPI (rts2db::Target *tar, const std::string &req, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;
	if (req == "obs")
	{

		rts2db::ObservationSet os = rts2db::ObservationSet ();
		os.loadTarget (tar->getTargetID ());

		_os << "{\"h\":["
			"{\"n\":\"ID\",\"t\":\"a\",\"prefix\":\"" << getServer ()->getPagePrefix () << "/observations/\",\"href\":0,\"c\":0},"
			"{\"n\":\"RA\",\"t\":\"r\",\"c\":1},"
			"{\"n\":\"DEC\",\"t\":\"d\",\"c\":2},"
			"{\"n\":\"Slew\",\"t\":\"t\",\"c\":3},"
			"{\"n\":\"Start\",\"t\":\"t\",\"c\":4},"
			"{\"n\":\"End\",\"t\":\"t\",\"c\":5},"
			"{\"n\":\"Images\",\"t\":\"n\",\"c\":6},"
			"{\"n\":\"Good images\",\"t\":\"n\",\"c\":7}],"
			"\"d\" : [" << std::fixed;

		for (rts2db::ObservationSet::iterator iter = os.begin (); iter != os.end (); iter++)
		{
			if (iter != os.begin ())
				_os << ",";
			_os << "[" << iter->getObsId () << "," 
				<< iter->getObsRa () << ","
				<< iter->getObsDec () << ",\""
				<< iter->getObsSlew () << "\",\""
				<< iter->getObsStart () << "\",\""
				<< iter->getObsEnd () << "\","
				<< iter->getNumberOfImages () << ","
				<< iter->getNumberOfGoodImages () << "]";
		}

		_os << "]}";

		returnJSON (_os, response_type, response, response_length);
		return;
	}
	if (req == "plan")
	{
		rts2db::PlanSetTarget ps (tar->getTargetID ());
		ps.load ();

		_os << "{\"h\":["
			"{\"n\":\"Plan ID\",\"t\":\"a\",\"prefix\":\"" << getServer ()->getPagePrefix () << "/plan/\",\"href\":0,\"c\":0},"
			"{\"n\":\"Start\",\"t\":\"t\",\"c\":2},"
			"{\"n\":\"End\",\"t\":\"t\",\"c\":3},"
			"{\"n\":\"RA\",\"t\":\"r\",\"c\":4},"
			"{\"n\":\"DEC\",\"t\":\"d\",\"c\":5},"
			"{\"n\":\"Alt start\",\"t\":\"altD\",\"c\":6},"
			"{\"n\":\"Az start\",\"t\":\"azD\",\"c\":7}],"
			"\"d\" : [" << std::fixed;

		for (rts2db::PlanSetTarget::iterator iter = ps.begin (); iter != ps.end (); iter++)
		{
			if (iter != ps.begin ())
				_os << ",";
			time_t t = iter->getPlanStart ();
			double JDstart = ln_get_julian_from_timet (&t);
			struct ln_equ_posn equ;
			tar->getPosition (&equ, JDstart);
			struct ln_hrz_posn hrz;
			tar->getAltAz (&hrz, JDstart);
			_os << "[" << iter->getPlanId () << ",\"" 
				<< iter->getPlanStart () << "\",\""
				<< iter->getPlanEnd () << "\","
				<< equ.ra << "," << equ.dec << ","
				<< hrz.alt << "," << hrz.az << "]";
		}

		_os << "]}";

		returnJSON (_os, response_type, response, response_length);
		return;
	}
	throw XmlRpc::XmlRpcException ("invalid API call");
}

void Targets::printTarget (rts2db::Target *tar, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	printHeader (_os, (std::string ("Target ") + tar->getTargetName ()).c_str (), ".b_left { float: left; width:50%; } .b_right {float: left; width: 50%;} ", NULL, "altAzTimer ();");

	printTargetHeader (tar->getTargetID (), "main", _os);

	includeJavaScript (_os, "equ.js");

	struct ln_equ_posn tradec;

	tar->getPosition (&tradec);

	_os << "<script type='text/javascript'>\n"
	"radec = new RaDec(" << tradec.ra << "," << tradec.dec << ", new LngLat(" << rts2core::Configuration::instance ()->getObserver ()->lng << "," << rts2core::Configuration::instance ()->getObserver ()->lat << "));\n"
	"hAlt = new DMS();\n"
	"hAz = new DMS();\n"
	"hSt = new HMS();\n"

	"function altAzTimer() {\n"
		"altaz = radec.altaz ();\n"
		"ln_deg_to_dms(altaz.alt,hAlt);\n"
		"ln_deg_to_dms(altaz.az,hAz);\n"
		"var jd = ln_get_julian_from_sys();\n"
		"var st = ln_get_mean_sidereal_time(jd);\n"
		"ln_deg_to_hms(st * 15.0,hSt);\n"
//ln_get_mean_sidereal_time(ln_get_julian_from_sys()),hSt);\n"
		"document.getElementById('altaz').innerHTML = hAlt.toString() + ' ' + hAz.toStringSigned(false);\n"
		"document.getElementById('st').innerHTML = hSt.toString();\n"
		"document.getElementById('jd').innerHTML = jd;\n"
		"setTimeout('altAzTimer()',100);\n"
	"}\n";

	if (canExecute ())
	{
		//enable target
		_os << "function tar_enable (enable) {\n"
			"var status;\n"
			"var hr = new XMLHttpRequest();\n"
			"hr.open('GET','api?e=' + enable);\n"
			"hr.onreadystatechange = function () {\n"
				"if (hr.readyState == 4 && hr.status == 200 ) {\n"
					"status = JSON.parse(hr.responseText);\n"
					"document.getElementById('tar_enable').checked = enable;\n"
				"}\n"
			"}\n"
			"hr.send(null);\n"
			"return false;\n"
		"}\n"

		"function tar_slew () {\n"
			"var status = {};\n"
			"var hr = new XMLHttpRequest();\n"
			"document.getElementById('slew').innerHTML = '<div id=\"slew_message\">requesting slew..</div><div><span id=\"slew_tar_ra\"></span>&nbsp;<span id=\"slew_tar_dec\"></span></div><div><span id=\"slew_tel_ra\"></span>&nbsp;<span id=\"slew_tel_dec\"></span></div><div><span id=\"slew_tel_alt\"></span>&nbsp;<span id=\"slew_tel_az\"></span></div>';\n"
			"document.getElementById('slew').style.display = '';\n"
			"hr.open('GET','api?slew=yes');\n"
			"hr.onreadystatechange = function () {\n"
				"if (hr.readyState == 4 && hr.status == 200 ) {\n"
					"status = JSON.parse(hr.responseText);\n"
					"var d = new DMS();\n"
					"var h = new HMS();\n"
					"document.getElementById('slew_message').innerHTML = 'slewing ' + status.remaining;\n"
					"ln_deg_to_hms(status.tar.ra,h);\n"
					"ln_deg_to_dms(status.tar.dec,d);\n"
					"document.getElementById('slew_tar_ra').innerHTML = h.toString ();\n"
					"document.getElementById('slew_tar_dec').innerHTML = d.toString ();\n"
					"ln_deg_to_hms(status.tel.ra,h);\n"
					"ln_deg_to_dms(status.tel.dec,d);\n"
					"document.getElementById('slew_tel_ra').innerHTML = h.toString ();\n"
					"document.getElementById('slew_tel_dec').innerHTML = d.toString ();\n"
					"ln_deg_to_hms(status.tel.az,d);\n"
					"document.getElementById('slew_tel_az').innerHTML = d.toString ();\n"
					"ln_deg_to_dms(status.tel.alt,d);\n"
					"document.getElementById('slew_tel_alt').innerHTML = d.toString ();\n"
				"}\n"
			"}\n"
			"hr.send(null);\n"
		"}\n"

		"function tar_next () {\n"
			"var status = {};\n"
			"var hr = new XMLHttpRequest();\n"
			"document.getElementById('next').innerHTML = '<div id=\"next_message\">requesting next..</div>';\n"
			"document.getElementById('next').style.display = '';\n"
			"hr.open('GET','api?next=yes');\n"
			"hr.onreadystatechange = function () {\n"
				"if (hr.readyState == 4 && hr.status == 200 ) {\n"
					"status = JSON.parse(hr.responseText);\n"
					"document.getElementById('next_message').innerHTML = 'queued';\n"
				"}\n"
			"}\n"
			"hr.send(null);\n"
		"}\n"

		"function tar_now (){\n"
			"var status = {};\n"
			"var hr = new XMLHttpRequest();\n"
			"document.getElementById('now').innerHTML = '<div id=\"now_message\">requesting now..</div>';\n"
			"document.getElementById('now').style.display = '';\n"
			"hr.open('GET','api?now=yes');\n"
			"hr.onreadystatechange = function(){\n"
				"if (hr.readyState == 4 && hr.status == 200 ) {\n"
					"status = JSON.parse(hr.responseText);\n"
					"document.getElementById('now_message').innerHTML = 'executed';\n"
				"}\n"
			"}\n"
			"hr.send(null);\n"
		"}\n"

		"function updateScript(){\n"
			"var status;\n"
			"var hr = new XMLHttpRequest();\n"
			"var st = scriptToText('scriptedit');\n"
			"hr.open('GET','api?script=' + encodeURIComponent(st) + '&camera=' + encodeURI(document.getElementById('cam').value));\n"
			"hr.onreadystatechange = function (){\n"
				"if (hr.readyState == 4){\n"
					"if (hr.status == 200) status = 'Updated to ' + st;\n"
						"else status = 'Failed to update';\n"
					"document.getElementById('scriptlog').innerHTML += status + '<br/>';\n"
				"}\n"
			"}\n"
			"hr.send(null);\n"
		"}\n";

	}

	_os << "</script>\n";

	// javascript to send requests..
	double JD = ln_get_julian_from_sys ();
	
	_os << "<div id='info' class='b_left'><table><tr><td>Name</td><td>" << tar->getTargetName () << "</td></tr>"
		"<tr><td>RA DEC</td><td>" << LibnovaRaDec (&tradec) << "</td></tr>"
		"<tr><td>ALT AZ</td><td><span id='altaz'/></td></tr>"
		"<tr><td>Bonus</td><td>" << tar->getBonus () << "</td></tr>"
		"<tr><td>Priority</td><td>" << tar->getTargetPriority () << "</td></tr>"
		"<tr><td>Sidereal Time</td><td><span id='st'/></td></tr>"
		"<tr><td>JD</td><td><span id='jd'/></td></tr>"
		"<tr><td>Violated constrints</td><td>" << tar->getViolatedConstraints (JD) << "</td></tr>"
		"<tr><td>Satisified constraints</td><td>" << tar->getSatisfiedConstraints (JD) << "</td></tr>";

	// print target labels
	rts2db::LabelsVector labels = tar->getLabels ();
	for (rts2db::LabelsVector::iterator iter = labels.begin (); iter != labels.end (); iter++)
	{
		_os << "<tr><td>" << getLabelName (iter->ltype) << "</td><td>" << iter->ltext << " </td></tr>";
	}
	if (canExecute ())
	{
		_os << "<tr><td cellspan='2'><button type='button' onclick='tar_slew();'>slew to target</button><div id='slew' style='display:none'>not slewing</div></td></tr>"
		"<tr><td cellspan='2'><button type='button' onclick='tar_next();'>next target</button><div id='next' style='display:none'>nothing</div></td></tr>"
		"<tr><td cellspan='2'><button type='button' onclick='tar_now();'>now (immediate) target</button><div id='now' style='display:none'>nothing</div></td></tr>"
		"<tr><td>Enabled</td><td><input type='checkbox' id='tar_enable' onclick='tar_enable (this.checked);' checked='"
		<< (tar->getTargetEnabled () ? "yes" : "no")
		<< "'/></td></tr>";
	}
	else
	{
	  	_os << "<tr><td>Enabled</td><td>" << (tar->getTargetEnabled () ? "yes" : "no") << "</td></tr>";
	}

	_os << "</table></div>";
	
	if (canExecute ())
	{
		_os << "<div id='scripteditor'i class='b_right'>";
		includeJavaScript (_os, "targetedit.js");
		_os << "<form id='scriptedit' name='se'>\n"
			"<div><select name='script' size='10' multiple='multiple' style='width: 200px'></select></div>"
  			"<div>Number of exposures: <input type='text' name='num' value='1'/></div>"
  			"<div>Exposure length (s): <input type='text' name='exposure' value='30'/></div>"
  			"<div>Filter: <select name='filter'>"
				"<option value='R'>Johnson R</option>"
				"<option value='I'>Johnson I</option>"
				"<option value='z'>SDSS z</option>"
			"</select></div>"
			"<div><button type='button' onclick='addScript(\"scriptedit\");document.getElementById(\"genscript\").innerHTML = scriptToText(\"scriptedit\");'>Add script</button></div>"
			"<div><button type='button' onclick='removeScripts(\"scriptedit\");document.getElementById(\"genscript\").innerHTML = scriptToText(\"scriptedit\");'>Remove</button></div>"
			"<div id='genscript'></div>"
			"<div>Camera: <select id='cam'>";

		rts2core::connections_t::iterator camiter = ((rts2core::Block *) getMasterApp ())->getConnections ()->begin ();

		while (true)
		{
			((rts2core::Block *) getMasterApp ())->getOpenConnectionType (DEVICE_TYPE_CCD, camiter);
			if (camiter == ((rts2core::Block *) getMasterApp ())->getConnections ()->end ())
				break;
			const char *n = (*camiter)->getName ();
			_os << "<option value='" << n << "'>" << n << "</option>";
			camiter++;
		}
	
		_os << "</select></div>"
			"<div><button type='button' onclick='updateScript();'>Update</button></div>"
			"<div id='scriptlog'></div>"
			"</form></div>";
	}
	else
	{
		_os << "<div id='scripts'>";

		if (cameras.empty ())
			cameras.load ();

		rts2db::CamList::iterator cam_names;
		for (cam_names = cameras.begin (); cam_names != cameras.end (); cam_names++)
		{
			const char *cam_name = (*cam_names).c_str ();
			_os << "<div>Camera " << cam_name << ":";
			std::string script_buf;
			try
			{
				tar->getScript (cam_name, script_buf);
				_os << script_buf;
			}
			catch (rts2core::Error &er)
			{
				_os << " error : " << er;
			}
			_os << "</div>";
		}

		_os << "</div>";
	}

	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Targets::printTargetInfo (rts2db::Target *tar, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;
	
	printHeader (_os, (std::string ("Target ") + tar->getTargetName ()).c_str ());

	printTargetHeader (tar->getTargetID (), "details", _os);

	_os << "<pre>";

	double JD = ln_get_julian_from_sys ();
	Rts2InfoValOStream ivos (&_os);
	tar->sendInfo (ivos, JD, 0);

	_os << "</pre>";
	
	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Targets::printTargetStat (rts2db::Target *tar, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;
	
	printHeader (_os, (std::string ("Target ") + tar->getTargetName ()).c_str ());

	printTargetHeader (tar->getTargetID (), "stat", _os);

	_os << "<h1>Target statistics data</h1>" << std::endl <<
		"<div id='info' class='b_left'><table><tr><td>First observation</td><td>" << LibnovaDateDouble (tar->getFirstObs ()) << "</td></tr>"
		"<tr><td>Last observation</td><td>" << LibnovaDateDouble (tar->getLastObs ()) << "</td></tr>"
		"<tr><td># of observations</td><td>" << tar->getTotalNumberOfObservations () << "</td></tr>"
		"<tr><td># of images</td<td>" << tar->getTotalNumberOfImages () << "</td></tr>"
		"<tr><td>Total open shutter time</td><td>" << TimeDiff (tar->getTotalOpenTime ()) << "</td></tr>"
		"</table></div>";

	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Targets::printTargetImages (rts2db::Target *tar, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	int pageno = params->getInteger ("p", 1);
	int pagesiz = params->getInteger ("s", 40);

	if (pageno <= 0)
		pageno = 1;

	int istart = (pageno - 1) * pagesiz;
	int ie = istart + pagesiz;
	int in = 0;

	int prevsize = params->getInteger ("ps", 128);
	const char * label = params->getString ("lb", getServer ()->getDefaultImageLabel ());
	std::string lb (label);
	XmlRpc::urlencode (lb);
	const char * label_encoded = lb.c_str ();

	float quantiles = params->getDouble ("q", DEFAULT_QUANTILES);
	int chan = params->getInteger ("chan", getServer ()->getDefaultChannel ());

	Previewer preview = Previewer (getServer ());

	printHeader (_os, (std::string ("Images of target ") + tar->getTargetName ()).c_str (), preview.style ());

	printTargetHeader (tar->getTargetID (), "images", _os);
	
	preview.script (_os, label_encoded, quantiles, chan);
		
	_os << "<p>";

	preview.form (_os, pageno, prevsize, pagesiz, chan, label);
	
	_os << "</p><p>";

	rts2db::ImageSetTarget is = rts2db::ImageSetTarget (tar->getTargetID ());
	is.load ();

	if (is.size () > 0)
	{
		_os << "<p>";

		for (rts2db::ImageSetTarget::iterator iter = is.begin (); iter != is.end (); iter++)
		{
			in++;
			if (in <= istart)
				continue;
			if (in > ie)
				break;
			preview.imageHref (_os, in, (*iter)->getAbsoluteFileName (), prevsize, label_encoded, quantiles, chan);
		}

		_os << "</p>";
	}
	else
	{
		_os << "<p>There isn't any image for target " << tar->getTargetName ();
	}

	_os << "</p><p>Page ";
	int i;
	
	for (i = 1; i <= ((int) is.size ()) / pagesiz; i++)
	 	preview.pageLink (_os, i, pagesiz, prevsize, label_encoded, i == pageno, quantiles, chan);
	if (in % pagesiz)
	 	preview.pageLink (_os, i, pagesiz, prevsize, label_encoded, i == pageno, quantiles, chan);
	_os << "</p>";
	
	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Targets::printTargetObservations (rts2db::Target *tar, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	printHeader (_os, (std::string ("Observations of target ") + tar->getTargetName ()).c_str (), NULL, "/css/table.css", "targetObs.refresh();");
	printTargetHeader (tar->getTargetID (), "obs", _os);

	_os << "<h1>Observations of target " << tar->getTargetName () << "</h1>\n";

	includeJavaScript (_os, "equ.js");
	includeJavaScriptWithPrefix (_os, "table.js");

	_os << "<script type='text/javascript'>\n"
		"targetObs = new Table('../api/obs','observations','targetObs');\n"
		"</script>\n"
		"<div id='observations'>Loading..</div>\n";

	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Targets::printTargetPlan (rts2db::Target *tar, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	printHeader (_os, (std::string ("Plan entries for target ") + tar->getTargetName ()).c_str (), NULL, "/css/table.css", "targetPlan.refresh();");
	printTargetHeader (tar->getTargetID (), "plan", _os);

	_os << "<h1>Plan entries for target <a href='../'>" << tar->getTargetName () << "</a></h1>\n";

	includeJavaScript (_os, "equ.js");
	includeJavaScriptWithPrefix (_os, "table.js");

	_os << "<script type='text/javascript'>\n"
		"targetPlan = new Table('../api/plan','plan','targetPlan');\n"
		"</script>\n"
		"<div id='plan'>Loading..</div>\n";

	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

#ifdef RTS2_HAVE_LIBJPEG

void Targets::plotTarget (rts2db::Target *tar, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	response_type = "image/jpeg";

	AltPlot ap (params->getInteger ("w", 800), params->getInteger ("h", 600));
	Magick::Geometry size (params->getInteger ("w", 800), params->getInteger ("h", 600));

	double from = params->getDouble ("from", 0);
	double to = params->getDouble ("to", 0);

	if (from < 0 && to == 0)
	{
		// just fr specified - from
		to = time (NULL);
		from += to;
	}
	else if (from == 0 && to == 0)
	{
		// default - one day
		to = time (NULL);
		from = to - 86400;
	}

	Magick::Image mimage (size, "white");
	ap.getPlot (from, to, tar, &mimage);

	Magick::Blob blob;
	mimage.write (&blob, "jpeg");

	response_length = blob.length();
	response = new char[response_length];
	memcpy (response, blob.data(), response_length);
}

#endif /* RTS2_HAVE_LIBJPEG */

#endif /* RTS2_HAVE_PGSQL */

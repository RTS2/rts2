/* 
 * Classes for answers to HTTP requests.
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

#include "xmlrpcd.h"
#include "httpreq.h"
#include "altaz.h"

#ifdef HAVE_PGSQL
#include "../utilsdb/observationset.h"
#include "../utilsdb/rts2imgset.h"
#include "../utilsdb/rts2user.h"
#include "../utilsdb/targetset.h"
#endif /* HAVE_PGSQL */

#include "../utils/radecparser.h"

using namespace XmlRpc;
using namespace rts2xmlrpc;

void GetRequestAuthorized::execute (std::string path, HttpParams *params, int &http_code, const char* &response_type, char* &response, int &response_length)
{

	if (getUsername () == std::string ("session_id"))
	{
		if (((XmlRpcd *) getMasterApp ())->existsSession (getPassword ()) == false)
		{
			authorizePage (http_code, response_type, response, response_length);
			return;
		}
	}

#ifdef HAVE_PGSQL
	if (verifyUser (getUsername (), getPassword ()) == false)
	{
		authorizePage (http_code, response_type, response, response_length);
		return;
	}
#else
	if (! (getUsername() ==  std::string ("petr") && getPassword() == std::string ("test")))
	{
		authorizePage (http_code, response_type, response, response_length);
		return;
	}
#endif /* HAVE_PGSQL */
	http_code = HTTP_OK;

	authorizedExecute (path, params, response_type, response, response_length);
}

#ifdef HAVE_PGSQL

#ifdef HAVE_LIBJPEG

void Graph::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, int &response_length)
{
	response_type = "image/jpeg";

	// get path and possibly date range
	std::vector <std::string> vals = SplitStr (path.substr (1), std::string ("/"));
	int valId = 1;
	time_t to = time (NULL);
	time_t from = to - 3600;

	switch (vals.size ())
	{
		case 0:
			// list values;
			printDevices (response_type, response, response_length);
			break;
		case 1:
			if (isdigit (vals[0][0]))
				valId = atoi (vals[0].c_str ());
			else
				valId = 1;
			break;
		case 3:
			// from - to date
			if (vals[2][0] == '-')
				from = to + strtod (vals[2].c_str (), NULL);

		case 2:
			plotValue (vals[0].c_str (), vals[1].c_str (), from, to, response_type, response, response_length);
			break;
		default:
			throw rts2core::Error ("Invalid path for graph!");
	}
}

void Graph::printDevices (const char* &response_type, char* &response, int &response_length)
{
	rts2db::RecvalsSet rs = rts2db::RecvalsSet ();
	rs.load ();
	std::ostringstream _os;
	_os << "<html><head><title>Record value list</title></head><body><table>";
	for (rts2db::RecvalsSet::iterator iter = rs.begin (); iter != rs.end (); iter++)
	{
		_os << "<tr><td>"
			<< iter->getDevice () << "</td><td><a href='"
			<< iter->getDevice () << "/" << iter->getValueName () << "'>" << iter->getValueName () << "</a></td><td>"
			<< LibnovaDateDouble (iter->getFrom ()) << "</td><td>"
			<< LibnovaDateDouble (iter->getTo ()) << "</td></tr>";
	}
	_os << "</table>";
	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Graph::plotValue (const char *device, const char *value, double from, double to, const char* &response_type, char* &response, int &response_length)
{
	rts2db::RecvalsSet rs = rts2db::RecvalsSet ();
	rs.load ();
	rts2db::Recval *rv = rs.searchByName (device, value);
	if (rv == NULL)
		throw rts2core::Error ("Cannot find device/value pair with given name");
	int valId = rv->getId ();

	ValuePlot vp (valId);

	Magick::Image* mimage = vp.getPlot (from, to);

	Magick::Blob blob;
	mimage->write (&blob, "jpeg");

	response_length = blob.length();
	response = new char[response_length];
	memcpy (response, blob.data(), response_length);

	delete mimage;
}

void AltAzTarget::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, int &response_length)
{
	AltAz altaz = AltAz ();

	altaz.plotAltAzGrid ();

	struct ln_hrz_posn hrz;

	rts2db::TargetSet ts = rts2db::TargetSet ();
	ts.load ();

	for (rts2db::TargetSet::iterator iter = ts.begin (); iter != ts.end (); iter++)
	{
		(*iter).second->getAltAz (&hrz);
		if (hrz.alt > -2)
			altaz.plotCross (&hrz, (*iter).second->getTargetName ());
	}
	
	Magick::Blob blob;
	altaz.write (&blob, "jpeg");

	response_type = "image/jpeg";

	response_length = blob.length();
	response = new char[response_length];
	memcpy (response, blob.data(), response_length);
}
void CurrentPosition::execute (std::string path, XmlRpc::HttpParams *params, int &http_code, const char* &response_type, char* &response, int &response_length)
{
	AltAz altaz = AltAz (250, 250);
	altaz.plotAltAzGrid ();

	struct ln_equ_posn pos;
	struct ln_hrz_posn hrz;

	double JD = ln_get_julian_from_sys ();

	// get current target position..
	XmlRpcd *serv = (XmlRpcd *) getMasterApp ();
	Rts2Conn *conn = serv->getOpenConnection (DEVICE_TYPE_MOUNT);
	Rts2Value *val;
	if (conn)
	{
		val = conn->getValue ("TEL_");
		if (val && val->getValueType () == RTS2_VALUE_ALTAZ)
		{
			hrz.alt = ((Rts2ValueAltAz *) val)->getAlt ();
			hrz.az = ((Rts2ValueAltAz *) val)->getAz ();

			altaz.plotCross (&hrz, "Telescope", "green");
		}

		val = conn->getValue ("TAR");
		if (val && val->getValueType () == RTS2_VALUE_RADEC)
		{
			pos.ra = ((Rts2ValueRaDec *) val)->getRa ();
			pos.dec  = ((Rts2ValueRaDec *) val)->getDec ();

			ln_get_hrz_from_equ (&pos, Rts2Config::instance ()->getObserver (), JD, &hrz);

			altaz.plotCross (&hrz, NULL, "blue");
		}
	}

	conn = serv->getOpenConnection (DEVICE_TYPE_EXECUTOR);
	if (conn)
	{
		Target *tar;
		val = conn->getValue ("current");
		if (val)
		{
			tar = createTarget (val->getValueInteger (), Rts2Config::instance ()->getObserver ());
			if (tar)
			{
				tar->getAltAz (&hrz, JD);
				altaz.plotCross (&hrz, tar->getTargetName (), "green");
				delete tar;
			}
		}
		val = conn->getValue ("next");
		if (val)
		{
			tar = createTarget (val->getValueInteger (), Rts2Config::instance ()->getObserver ());
			if (tar)
			{
				tar->getAltAz (&hrz, JD);
				altaz.plotCross (&hrz, tar->getTargetName (), "blue");
				delete tar;
			}
		}
	}

	Magick::Blob blob;
	altaz.write (&blob, "jpeg");

	response_type = "image/jpeg";

	response_length = blob.length();
	response = new char[response_length];
	memcpy (response, blob.data(), response_length);
}

#endif /* HAVE_LIBJPEG */

void Targets::authorizedExecute (std::string path, HttpParams *params, const char* &response_type, char* &response, int &response_length)
{
	// get path and possibly date range
	std::vector <std::string> vals = SplitStr (path.substr (1), std::string ("/"));

	if (vals.size () == 0)
	{
		listTargets (response_type, response, response_length);
	}
	else
	{
		// get target id..
		char *endptr;
		int tar_id = strtol (vals[0].c_str (), &endptr, 10);
		if (*endptr != '\0')
			throw rts2core::Error ("Expected target number!");

		Target *tar = createTarget (tar_id, Rts2Config::instance ()->getObserver ());
		if (tar == NULL)
			throw rts2core::Error ("Cannot find target with given ID");

		switch (vals.size ())
		{
			case 1:
				printTarget (tar, response_type, response, response_length);
				break;
			case 2:
				if (vals[1] == "images")
				{
					printTargetImages (tar, response_type, response, response_length);
					break;
				}
				if (vals[1] == "obs")
				{
					printTargetObservations (tar, response_type, response, response_length);
					break;
				}
			default:
				throw rts2core::Error ("Invalid path!");
		}
	}
}

void Targets::listTargets (const char* &response_type, char* &response, int &response_length)
{
	std::ostringstream _os;
	_os << "<html><head><title>List of targets</title></head><body><table>";

	rts2db::TargetSet ts = rts2db::TargetSet ();
	ts.load ();

	for (rts2db::TargetSet::iterator iter = ts.begin (); iter != ts.end (); iter++)
	{
		_os << "<tr><td><a href='" << iter->first << "/'>" << iter->first
			<< "</a></td><td><a href='" << iter->first << "/'>" << iter->second->getTargetName () << "</a></tr></td>";
	}

	_os << "</table></body></html>";

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Targets::printTarget (Target *tar, const char* &response_type, char* &response, int &response_length)
{
	std::ostringstream _os;

	_os << "<html><head><title>Target " << tar->getTargetName () << "</title></head><body>";

	_os << "<p><a href='images/'>images</a>&nbsp;<a href='obs/'>observations</a></p>";

	_os << "<pre>";

	double JD = ln_get_julian_from_sys ();
	Rts2InfoValOStream ivos (&_os);
	tar->sendInfo (ivos, JD);

	_os << "</pre></body></html>";

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Targets::printTargetImages (Target *tar, const char* &response_type, char* &response, int &response_length)
{
	std::ostringstream _os;

	_os << "<html><head><title>Images of target " << tar->getTargetName () << "</title></head><body>";

	Rts2ImgSetTarget is = Rts2ImgSetTarget (tar->getTargetID ());
	is.load ();

	if (is.size () > 0)
	{
		_os << "<table>";

		for (Rts2ImgSetTarget::iterator iter = is.begin (); iter != is.end (); iter++)
		{
			std::string fn = (*iter)->getFileName ();
			_os << "<tr><td><a href='" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/jpeg" << fn
				<< "'><img src='" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/preview" << fn
				<< "'/></a>" << (*iter)->getFileName () << "</td><td>" << (*iter)->getExposureLength () << "</td></tr>";
		}

		_os << "</table>";
	}
	else
	{
		_os << "<p>There isn't any image for target " << tar->getTargetName ();
	}

	_os << "</body></html>";

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Targets::printTargetObservations (Target *tar, const char* &response_type, char* &response, int &response_length)
{
	std::ostringstream _os;

	_os << "<html><head><title>Observations of target " << tar->getTargetName () << "</title></head><body>";

	rts2db::ObservationSet os = rts2db::ObservationSet (tar->getTargetID ());

	if (os.size () > 0)
	{
		_os << "<table>";

		for (rts2db::ObservationSet::iterator iter = os.begin (); iter != os.end (); iter++)
		{
			_os << "<tr><td>" << iter->getObsId () << "</td><td>" << iter->getTargetName () << "</td></tr>";
		}

		_os << "</table>";
	}
	else
	{
		_os << "<p>There isn't any observation for target " << tar->getTargetName ();
	}

	_os << "</body></html>";

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void AddTarget::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, int &response_length)
{
	// get path and possibly date range
	std::vector <std::string> vals = SplitStr (path.substr (1), std::string ("/"));

	switch (vals.size ())
	{
		case 0:
			askForTarget (response_type, response, response_length);
			return;
		case 1:
			if (vals[0] == "confirm")
			{
				confimTarget (params->getString ("target", ""), response_type, response, response_length);
				return;
			}
			if (vals[0] == "new_target")
			{
				newTarget (params->getString ("oriname", ""), params->getString ("name", ""), params->getInteger ("tarid", INT_MAX), params->getDouble ("ra", rts2_nan ("f")), params->getDouble ("dec", rts2_nan ("f")), response_type, response, response_length);
				return;
			}
			if (vals[0] == "schedule")
			{
				schedule  (params->getInteger ("tarid", INT_MAX), response_type, response, response_length);
				return;
			}
	}

	throw rts2core::Error ("Unknown action for addtarget");
}

void AddTarget::askForTarget (const char* &response_type, char* &response, int &response_length)
{
	std::ostringstream _os;

	_os << "<html><head><title>Add new target</title></head><body>";

	_os << "<p>Please provide anything which can be used to identify what you would like to observer. Valid inputs are:<ul>";

	_os << "<li>RA DEC in various formats. DEC must be divided with + or - sign. Those are valid inputs<ul><li>300 +24</li> <li>10:20 +33:12</li> <li>10:20:30 -12:34:59</li></ul></li>";

	_os << "<li>SIMBAD resolvable name</li>";

	_os << "<li>any target from RTS2 target database</li>";

	_os << "<li>any Minor planet name resolvable by MPEC</li>";

	_os << "<li>one line MPEC</li>";

	_os << "</ul></p><form name='add_target' action='confirm' method='get'><input type='text' textwidth='20' name='target'/><input type='submit' value='Add'/></form>";

	_os << "</body></html>";

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void AddTarget::confimTarget (const char *tar, const char* &response_type, char* &response, int &response_length)
{
	std::ostringstream _os;

	if (strlen (tar) == 0)
	{
		
		_os << "<html><head><title>Invalid target</title></head></body><p>Please specify valid target name - it must contain at least one character.</p></body></html>";
	}
	else
	{
		_os << "<html><head><title>Confirm target</title></head><body>";

		struct ln_equ_posn pos;

		pos.ra = pos.dec = rts2_nan ("f");

		// check if its among already known targets
		rts2db::TargetSetByName ts_n = rts2db::TargetSetByName (tar);
		ts_n.load ();

		_os << "<p>You entered " << tar << "</p>";
		if (ts_n.size () > 0)
		{
			_os << "<p>Following targets with name similar to which you provide were found. Please click on link to select one for scheduling, or continue bellow to create a new one.<ul>";

			for (rts2db::TargetSetByName::iterator iter = ts_n.begin (); iter != ts_n.end (); iter++)
			{
				// print target together with option to create schedule from this one..
				iter->second->getPosition (&pos);
				_os << "<li><a href='schedule?tarid=" << iter->second->getTargetID () << "'>" << iter->second->getTargetName () << "</a> " << LibnovaRa (pos.ra) << " " << LibnovaDec (pos.dec) << "</li>";
			}
			_os << "</ul></p>";
		}

		// check if when we parse it by ra dec, we will get something..
		if (parseRaDec (tar, pos.ra, pos.dec) == 0)
		{
			// then look for targets close to this one - within 1 deg
			rts2db::TargetSet ts_r = rts2db::TargetSet (&pos, 1);
			ts_r.load ();
			if (ts_r.size () > 0)
			{
				_os << "<p>Following targets were found within one degree from given position. Please click on link to select one for scheduling, or continue bellow entering a new target.<ul>";
				for (rts2db::TargetSetByName::iterator iter = ts_r.begin (); iter != ts_r.end (); iter++)
				{
					// print target together with option to create schedule from this one..
					iter->second->getPosition (&pos);
					_os << "<li><a href='schedule?tarid=" << iter->second->getTargetID () << "'>" << iter->second->getTargetName () << "</a> " << LibnovaRa (pos.ra) << " " << LibnovaDec (pos.dec) << "</li>";
				}
				_os << "</ul></p>";
			}
		}
		// check for simbad

		// print new target
		if (! (isnan (pos.ra) || isnan (pos.dec)))
		{
			_os << "If you would like to enter new target with RA DEC " << LibnovaRaDec (pos.ra, pos.dec)
				<< ", then fill fields bellow and click submit: <form name='new_target' action='new_target' method='get'><input type='hidden' name='ra' value='"
				<< pos.ra << "'/><input type='hidden' name='dec' value='" << pos.dec 
				<< "'><input type='hidden' name='oriname' value='" << tar << "'/>Target name:<input type='text' name='name' value='"
				<< tar << "'/><br/>Optional target ID - left blank for generated<input type='text' name='tarid' value=' '/><br/><input type='submit' value='Create target'/></form>";
		}

		_os << "</body></html>";
	}

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void AddTarget::newTarget (const char *oriname, const char *name, int tarid, double ra, double dec, const char* &response_type, char* &response, int &response_length)
{
	std::ostringstream _os;

	ConstTarget *constTarget = new ConstTarget ();
	constTarget->setPosition (ra, dec);
	constTarget->setTargetName (name);
	constTarget->setTargetType (TYPE_OPORTUNITY);

	int ret;

	if (tarid != INT_MAX)
		ret = constTarget->save (false, tarid);
	else
		ret = ((Target *) constTarget)->save (false);
	
	if (ret)
		throw XmlRpcException ("Target with given ID already exists");

	_os << "<html><head><title>Created new target.</title></head><body><p>Target with name " << name << " and ID " << constTarget->getTargetID ()
		<< " sucessfully created. Please click <a href='schedule?tarid=" << constTarget->getTargetID ()
		<< "'>here</a> to continue with scheduling the target.</p></body></html>";

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void AddTarget::schedule (int tarid, const char* &response_type, char* &response, int &response_length)
{
	std::ostringstream _os;

	Target *tar = createTarget (tarid, Rts2Config::instance ()->getObserver ());

	if (tar == NULL)
		throw XmlRpcException ("Cannot find target with given ID!");

	_os << "<html><head><title>Scheduling target " << tar->getTargetName ()
		<< "</title></head><body><p>Please choose how your observations should be scheduled and peformed:<form name=''";

	_os << "</p></body></html>";

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

#endif /* HAVE_PGSQL */

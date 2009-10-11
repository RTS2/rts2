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

#ifdef HAVE_PGSQL
#include "../utilsdb/rts2imgset.h"
#include "../utilsdb/rts2user.h"
#endif /* HAVE_PGSQL */

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
			default:
				throw rts2core::Error ("Invalid path!");
		}
	}
}

void Targets::listTargets (const char* &response_type, char* &response, int &response_length)
{
	std::ostringstream _os;
	_os << "<html><head><title>List of targets</title></head><body><table>";

	Rts2TargetSet ts = Rts2TargetSet ();

	for (Rts2TargetSet::iterator iter = ts.begin (); iter != ts.end (); iter++)
	{
		_os << "<tr><td>" << iter->first << "</td><td>" << iter->second->getTargetName () << "</tr></td>";
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

	_os << "<html><head><title>Target " << tar->getTargetName () << "</title></head><body><pre>";

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

	if (is.size () > 0)
	{
		_os << "<table>";

		for (Rts2ImgSetTarget::iterator iter = is.begin (); iter != is.end (); iter++)
		{
			_os << "<tr><td>" << (*iter)->getFileName () << "</td><td>" << (*iter)->getExposureLength () << "</td></tr>";
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

#endif /* HAVE_PGSQL */

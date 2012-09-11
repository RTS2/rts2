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
#include "altplot.h"
#include "directory.h"
#include "dirsupport.h"

#ifdef RTS2_HAVE_PGSQL
#include "rts2db/observationset.h"
#include "rts2db/imageset.h"
#include "rts2db/user.h"
#include "rts2db/targetset.h"
#endif /* RTS2_HAVE_PGSQL */

#include "radecparser.h"

using namespace XmlRpc;
using namespace rts2xmlrpc;

void GetRequestAuthorized::execute (XmlRpc::XmlRpcSource *source, struct sockaddr_in *saddr, std::string path, HttpParams *params, int &http_code, const char* &response_type, char* &response, size_t &response_length)
{
	// if it is public page..
	if (((XmlRpcd *) getMasterApp ())->isPublic (saddr, getPrefix () + path))
	{
		http_code = HTTP_OK;
		authorizedExecute (path, params, response_type, response, response_length);
		return;
	}

	if (getUsername () == std::string ("session_id"))
	{
		if (((XmlRpcd *) getMasterApp ())->existsSession (getPassword ()) == false)
		{
			authorizePage (http_code, response_type, response, response_length);
			return;
		}
	}

	if (verifyUser (getUsername (), getPassword (), executePermission) == false)
	{
		authorizePage (http_code, response_type, response, response_length);
		return;
	}
	http_code = HTTP_OK;

	authorizedExecute (path, params, response_type, response, response_length);

	((XmlRpcd *) getMasterApp ())->addExecutedPage ();
}

void GetRequestAuthorized::printHeader (std::ostream &os, const char *title, const char *css, const char *cssLink, const char *onLoad)
{
	os << "<html><head><title>" << title << "</title>";
	
	if (css)
		os << "<style type='text/css'>" << css << "</style>";

	if (cssLink)
		os << "<link href='" << ((rts2xmlrpc::XmlRpcd *)getMasterApp ())->getPagePrefix () << cssLink << "' rel='stylesheet' type='text/css'/>";

	os << "</head><body";
	
	if (onLoad)
		os << " onload='" << onLoad << "'";
	
	os << ">";
}

void GetRequestAuthorized::printSubMenus (std::ostream &os, const char *prefix, const char *current, const char *submenus[][2])
{
	const char **p = submenus[0];  
	const char *a = p[0];
	const char *n = p[1];
	while (true)
	{
		if (strcmp (current, a))
		{
			os << "<a href='" << prefix << a << "/'>" << n << "</a>";
		}
		else
		{
			os << n;
		}
		p += 2;
		a = p[0];
		n = p[1];
		if (a == NULL)
			break;
		os << "&nbsp;";
	}
}

void GetRequestAuthorized::printFooter (std::ostream &os)
{
	os << "</body></html>";
}

void GetRequestAuthorized::includeJavaScript (std::ostream &os, const char *name)
{
	os << "<script type='text/javascript' src='" << ((rts2xmlrpc::XmlRpcd *)getMasterApp ())->getPagePrefix () << "/js/" << name << "'></script>\n";
}

void GetRequestAuthorized::includeJavaScriptWithPrefix (std::ostream &os, const char *name)
{
	os << "<script type='text/javascript'>pagePrefix = '" << ((rts2xmlrpc::XmlRpcd *)getMasterApp ())->getPagePrefix () << "';</script>\n";
	includeJavaScript (os, name);
}

#ifdef RTS2_HAVE_LIBJPEG

void CurrentPosition::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	int s = params->getInteger ("s", 250);
	AltAz altaz = AltAz (s, s);
	altaz.plotAltAzGrid ();

	struct ln_equ_posn pos;
	struct ln_hrz_posn hrz;

	double JD = ln_get_julian_from_sys ();

	// get current target position..
	XmlRpcd *serv = (XmlRpcd *) getMasterApp ();
	rts2core::Connection *conn = serv->getOpenConnection (DEVICE_TYPE_MOUNT);
	rts2core::Value *val;
	if (conn)
	{
		val = conn->getValue ("TEL_");
		if (val && val->getValueType () == RTS2_VALUE_ALTAZ)
		{
			hrz.alt = ((rts2core::ValueAltAz *) val)->getAlt ();
			hrz.az = ((rts2core::ValueAltAz *) val)->getAz ();

			altaz.plotCross (&hrz, "Telescope", "green");
		}

		val = conn->getValue ("TAR");
		if (val && val->getValueType () == RTS2_VALUE_RADEC)
		{
			pos.ra = ((rts2core::ValueRaDec *) val)->getRa ();
			pos.dec  = ((rts2core::ValueRaDec *) val)->getDec ();

			if (!(isnan (pos.ra) || isnan (pos.dec)))
			{
				ln_get_hrz_from_equ (&pos, Configuration::instance ()->getObserver (), JD, &hrz);
				altaz.plotCross (&hrz, NULL, "blue");
			}
		}
	}

#ifdef RTS2_HAVE_PGSQL
	conn = serv->getOpenConnection (DEVICE_TYPE_EXECUTOR);
	if (conn)
	{
		rts2db::Target *tar;
		val = conn->getValue ("current");
		if (val && val->getValueInteger () >= 0)
		{
			tar = createTarget (val->getValueInteger (), Configuration::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
			if (tar)
			{
				tar->getAltAz (&hrz, JD);
				altaz.plotCross (&hrz, tar->getTargetName (), "green");
				delete tar;
			}
		}
		val = conn->getValue ("next");
		if (val && val->getValueInteger () >= 0)
		{
			tar = createTarget (val->getValueInteger (), Configuration::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
			if (tar)
			{
				tar->getAltAz (&hrz, JD);
				altaz.plotCross (&hrz, tar->getTargetName (), "blue");
				delete tar;
			}
		}
	}
#endif /* RTS2_HAVE_PGSQL */

	Magick::Blob blob;
	altaz.write (&blob, "jpeg");

	response_type = "image/jpeg";

	response_length = blob.length();
	response = new char[response_length];
	memcpy (response, blob.data(), response_length);
}

#endif /* RTS2_HAVE_LIBJPEG */

#ifdef RTS2_HAVE_PGSQL

#ifdef RTS2_HAVE_LIBJPEG

void AltAzTarget::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	// get new AltAz graph
	AltAz altaz = AltAz ();

	// plot its AltAz grid
	altaz.plotAltAzGrid ();

	struct ln_hrz_posn hrz;

	// retrieve from database set of all targets..
	rts2db::TargetSet ts = rts2db::TargetSet ();
	ts.load ();

	// iterate through the set, plot location of each target
	for (rts2db::TargetSet::iterator iter = ts.begin (); iter != ts.end (); iter++)
	{
	  	// retrieve target AltAz coordinates
		(*iter).second->getAltAz (&hrz);

		// and plot them with proper caption
		if (hrz.alt > -2)
			altaz.plotCross (&hrz, (*iter).second->getTargetName ());
	}
	
	// write image to blob as JPEG
	Magick::Blob blob;
	altaz.write (&blob, "jpeg");

	// set MIME response type
	response_type = "image/jpeg";

	// lenght of response
	response_length = blob.length();
	// create and fill response buffer
	response = new char[response_length];
	memcpy (response, blob.data(), response_length);
}

#endif /* RTS2_HAVE_LIBJPEG */

#endif /* RTS2_HAVE_PGSQL */

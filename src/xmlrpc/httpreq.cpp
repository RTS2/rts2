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
#include "dirsupport.h"
#include "imgpreview.h"

#ifdef HAVE_PGSQL
#include "../utilsdb/observationset.h"
#include "../utilsdb/imageset.h"
#include "../utilsdb/user.h"
#include "../utilsdb/targetset.h"
#endif /* HAVE_PGSQL */

#include "../utils/radecparser.h"

using namespace XmlRpc;
using namespace rts2xmlrpc;

void GetRequestAuthorized::execute (std::string path, HttpParams *params, int &http_code, const char* &response_type, char* &response, size_t &response_length)
{
	// if it is public page..
	if (((XmlRpcd *) getMasterApp ())->isPublic (getPrefix () + path))
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

#ifdef HAVE_PGSQL
	if (verifyUser (getUsername (), getPassword (), executePermission) == false)
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

Directory::Directory (const char* prefix, const char *_dirPath, const char *_defaultFile, XmlRpc::XmlRpcServer* s):GetRequestAuthorized (prefix, _dirPath, s)
{
	dirPath = std::string (_dirPath);
	defaultFile = std::string (_defaultFile);
}

void Directory::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	// check if file exists..
	std::string fn = dirPath + '/' + path;
	if (path.length () > 0 && path[path.length () - 1] == '/')
		fn += defaultFile;
	
	// check if file exists..
	int f = open (fn.c_str (), O_RDONLY);
	if (f != -1)
	{
		struct stat st;
		if (fstat (f, &st) == -1)
			throw XmlRpcException ("Cannot get file properties");

		response_length = st.st_size;
		response = new char[response_length];
		ssize_t ret = read (f, response, response_length);
		if (ret != (ssize_t) response_length)
		{
			delete[] response;
			throw XmlRpcException ("Cannot read file");
		}
		close (f);
		response_type = "text/html";
		return;
	}

	printHeader (_os, path.c_str ());

	struct dirent **namelist;
	int n;
	int i;
	int ret;

	const char *pagesort = params->getString ("o", "filename");

	enum {SORT_FILENAME, SORT_DATE} sortby = SORT_FILENAME;
	if (!strcmp (pagesort, "date"))
		sortby = SORT_DATE;

	switch (sortby)
	{
	 	case SORT_DATE:
			/* if following fails to compile, please have a look to value of your
			 * _POSIX_C_SOURCE #define, record it and send it to petr@kubanek.net.
			 * Please contact petr@kubanek.net if you don't know how to get
			 * _POSIX_C_SOURCE. */
			n = scandir ((dirPath + path).c_str (), &namelist, 0, cdatesort);
			break;
		case SORT_FILENAME:
		  	n = scandir ((dirPath + path).c_str (), &namelist, 0, alphasort);
			break;
	}

	if (n < 0)
	{
		throw XmlRpcException ("Cannot open directory");
	}

	// first show directories..
	_os << "<p>";
	for (i = 0; i < n; i++)
	{
		char *fname = namelist[i]->d_name;
		struct stat sbuf;
		ret = stat ((dirPath + path + fname).c_str (), &sbuf);
		if (ret)
			continue;
		if (S_ISDIR (sbuf.st_mode) && strcmp (fname, ".") != 0)
		{
			_os << "<a href='" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << getPrefix () << path << fname << "/'>" << fname << "</a> ";
		}
	}

	_os << "</p>";
	printFooter (_os);

	for (i = 0; i < n; i++)
	{
		free (namelist[i]);
	}

	free (namelist);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

#ifdef HAVE_LIBJPEG

void CurrentPosition::execute (std::string path, XmlRpc::HttpParams *params, int &http_code, const char* &response_type, char* &response, size_t &response_length)
{
	int s = params->getInteger ("s", 250);
	AltAz altaz = AltAz (s, s);
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

			if (!(isnan (pos.ra) || isnan (pos.dec)))
			{
				ln_get_hrz_from_equ (&pos, Rts2Config::instance ()->getObserver (), JD, &hrz);
				altaz.plotCross (&hrz, NULL, "blue");
			}
		}
	}

#ifdef HAVE_PGSQL
	conn = serv->getOpenConnection (DEVICE_TYPE_EXECUTOR);
	if (conn)
	{
		rts2db::Target *tar;
		val = conn->getValue ("current");
		if (val && val->getValueInteger () >= 0)
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
		if (val && val->getValueInteger () >= 0)
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
#endif /* HAVE_PGSQL */

	Magick::Blob blob;
	altaz.write (&blob, "jpeg");

	http_code = HTTP_OK;

	response_type = "image/jpeg";

	response_length = blob.length();
	response = new char[response_length];
	memcpy (response, blob.data(), response_length);
}

#endif /* HAVE_LIBJPEG */

#ifdef HAVE_PGSQL

#ifdef HAVE_LIBJPEG

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
	
	// write image to blob as JEPEG
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

#endif /* HAVE_LIBJPEG */

#endif /* HAVE_PGSQL */

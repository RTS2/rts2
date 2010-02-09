/*
 * Classes for generating pages with observations by nights.
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

#include "imgpreview.h"
#include "nightreq.h"
#include "nightdur.h"
#include "xmlrpcd.h"
#include "xmlrpc++/urlencoding.h"

#ifdef HAVE_PGSQL
#include "../utilsdb/observationset.h"
#ifdef HAVE_LIBJPEG
#include "altaz.h"
#endif // HAVE_LIBJPEG
#include "../utils/rts2config.h"

using namespace XmlRpc;
using namespace rts2xmlrpc;

void Night::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	response_type = "text/html";

	// get path and possibly date range
	std::vector <std::string> vals = SplitStr (path.substr (1), std::string ("/"));
	int year = -1;
	int month = -1;
	int day = -1;
	
	enum {NONE, IMAGES, ALTAZ} action = NONE;

	switch (vals.size ())
	{
		case 4:
			// print all images..
			if (vals[3] == "all")
				action = IMAGES;
			else if (vals[3] == "altaz")
			  	action = ALTAZ;
			else
				throw rts2core::Error ("Invalid path for all observations");
		case 3:
			day = atoi (vals[2].c_str ());
		case 2:
			month = atoi (vals[1].c_str ());
		case 1:
			year = atoi (vals[0].c_str ());
		case 0:
			switch (action)
			{
				case IMAGES:
					printAllImages (year, month, day, params, response, response_length);
					break;
				case ALTAZ:
					printAltAz (year, month, day, params, response_type, response, response_length);
					break;
				default:
					printTable (year, month, day, response, response_length);
			}
			break;
		default:
			throw rts2core::Error ("Invalid path for observations!");
	}
}

void Night::listObs (int year, int month, int day, std::ostringstream &_os)
{
	rts2db::ObservationSet os = rts2db::ObservationSet ();

	time_t from;
	int64_t duration;

	getNightDuration (year, month, day, from, duration);

	time_t end = from + duration;

	os.loadTime (&from, &end);

	for (rts2db::ObservationSet::iterator iter = os.begin (); iter != os.end (); iter++)
	{
		_os << "<tr><td><a href='" <<  ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/observations/" << iter->getObsId () << "'>"
			<< iter->getObsId () << " " << iter->getTargetName () << "</a>"
			<< "</td><td>" << LibnovaDateDouble (iter->getObsStart ())
			<< "</td><td>" << LibnovaDateDouble (iter->getObsEnd ())
			<< "</td></tr>";
	}
}

void Night::printAllImages (int year, int month, int day, XmlRpc::HttpParams *params, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	_os << "<html><head><title>Observations";

	if (year > 0)
	{
		_os << " for " << year;
		if (month > 0)
		{
			_os << "-" << month;
			if (day > 0)
			{
				_os << "-" << day;
			}
		}
	}

	int pageno = params->getInteger ("p", 1);
	int pagesiz = params->getInteger ("s", 40);

	if (pageno <= 0)
		pageno = 1;

	int istart = (pageno - 1) * pagesiz;
	int ie = istart + pagesiz;
	int in = 0;

	int prevsize = params->getInteger ("ps", 128);
	const char * label = params->getString ("lb", "%Y-%m-%d %H:%M:%S @OBJECT");
	std::string lb (label);
	XmlRpc::urlencode (lb);
	const char * label_encoded = lb.c_str ();

	time_t from;
	int64_t duration;

	getNightDuration (year, month, day, from, duration);

	time_t end = from + duration;

	_os << "</title>";
	
	Previewer preview = Previewer ();
	preview.script (_os, label_encoded);

	_os << "</head><body><p>";

	preview.form (_os, label_encoded);

	_os << "</p><p>";

	rts2db::ImageSetDate is = rts2db::ImageSetDate (from, end);
	is.load ();

	for (rts2db::ImageSetDate::iterator iter = is.begin (); iter != is.end (); iter++)
	{
		in++;
		if (in <= istart)
			continue;
		if (in > ie)
			break;
		preview.imageHref (_os, in, (*iter)->getAbsoluteFileName (), prevsize, label_encoded);
	}

	_os << "</p><p>Page ";
	int i;
	for (i = 1; i <= ((int) is.size ()) / pagesiz; i++)
	 	preview.pageLink (_os, i, pagesiz, prevsize, label_encoded, i == pageno);
	if (in % pagesiz)
	 	preview.pageLink (_os, i, pagesiz, prevsize, label_encoded, i == pageno);
	_os << "</p></body></html>";

	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Night::printAltAz (int year, int month, int day, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
#ifdef HAVE_LIBJPEG
	response_type = "image/jpeg";

	time_t from;
	int64_t duration;

	getNightDuration (year, month, day, from, duration);

	time_t end = from + duration;

	rts2db::ImageSetDate is = rts2db::ImageSetDate (from, end);
	is.load ();

	int s = params->getInteger ("s", 250);
	AltAz altaz = AltAz (s, s);
	altaz.plotAltAzGrid ();

	for (rts2db::ImageSetDate::iterator iter = is.begin (); iter != is.end (); iter++)
	{
		struct ln_hrz_posn hrz;
		try
		{
			(*iter)->getCoordBestAltAz (hrz, Rts2Config::instance ()->getObserver ());
			(*iter)->closeFile ();
			altaz.plotCross (&hrz, NULL, "green");
		}
		catch (rts2core::Error &er)
		{
			(*iter)->closeFile ();
		}
	}

	Magick::Blob blob;
	altaz.write (&blob, "jpeg");

	response_length = blob.length();
	response = new char[response_length];
	memcpy (response, blob.data(), response_length);
#else
	throw rts2core::Error ("missing libjpeg support");
#endif
}

void Night::printTable (int year, int month, int day, char* &response, size_t &response_length)
{
	bool do_list = false;
	std::ostringstream _os;

	_os << "<html><head><title>Observations";

	if (year > 0)
	{
		_os << " for " << year;
		if (month > 0)
		{
			_os << "-" << month;
			if (day > 0)
			{
				_os << "-" << day;
				do_list = true;
			}
		}
	}

	if (year == 0 || month == 0 || day == 0)
		do_list = true;

	_os << "</title></head><body><p><a href='all'>All images</a>&nbsp;<a href='altaz'>Night images alt-az plot</a></p><p><table>";

	if (do_list == true)
	{
		listObs (year, month, day, _os);
	}
	else
	{
		rts2db::ObservationSetDate as = rts2db::ObservationSetDate ();
		as.load (year, month, day);

		for (rts2db::ObservationSetDate::iterator iter = as.begin (); iter != as.end (); iter++)
		{
			_os << "<tr><td><a href='" << iter->first << "/'>" << iter->first << "</a></td><td>" << iter->second << "</td></tr>";
		}

	}

	_os << "</table><p></body></html>";

	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

#endif /* HAVE_PGSQL */

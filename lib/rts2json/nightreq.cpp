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

#include "rts2json/altaz.h"
#include "rts2json/nightdur.h"
#include "rts2json/nightreq.h"
#include "rts2json/jsonvalue.h"
#include "rts2json/imgpreview.h"
#include "xmlrpc++/urlencoding.h"

#ifdef RTS2_HAVE_PGSQL
#include "rts2db/observationset.h"
#include "rts2db/imageset.h"
#ifdef RTS2_HAVE_LIBJPEG
#include "rts2json/altaz.h"
#include "rts2json/altplot.h"
#endif // RTS2_HAVE_LIBJPEG
#include "configuration.h"

using namespace XmlRpc;
using namespace rts2json;

void Night::authorizedExecute (__attribute__ ((unused)) XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	response_type = "text/html";

	// get path and possibly date range
	std::vector <std::string> vals = SplitStr (path.substr (1), std::string ("/"));
	int year = -1;
	int month = -1;
	int day = -1;

	enum {NONE, IMAGES, API, ALT, ALTAZ} action = NONE;

	switch (vals.size ())
	{
		case 4:
			// print all images..
			if (vals[3] == "all")
				action = IMAGES;
			else if (vals[3] == "api")
				action = API;
#ifdef RTS2_HAVE_LIBJPEG
			else if (vals[3] == "alt")
				action = ALT;
			else if (vals[3] == "altaz")
			  	action = ALTAZ;
#endif // RTS2_HAVE_LIBJPEG
			else
				throw rts2core::Error ("Invalid path for all observations");
			break;
		case 3:
			if (vals[2] == "api")
				action = API;
			else
				day = atoi (vals[2].c_str ());
			RTS2_FALLTHRU;
		case 2:
			if (vals[1] == "api")
				action = API;
			else
				month = atoi (vals[1].c_str ());
			RTS2_FALLTHRU;
		case 1:
			if (vals[0] == "api")
				action = API;
			else
				year = atoi (vals[0].c_str ());
			RTS2_FALLTHRU;
		case 0:
			switch (action)
			{
				case IMAGES:
					printAllImages (year, month, day, params, response, response_length);
					break;
				case API:
					callAPI (year, month, day, response, response_type, response_length);
					break;
#ifdef RTS2_HAVE_LIBJPEG
				case ALT:
					printAlt (year, month, day, params, response_type, response, response_length);
					break;
				case ALTAZ:
					printAltAz (year, month, day, params, response_type, response, response_length);
					break;
#endif // RTS2_HAVE_LIBJPEG
				default:
					printTable (year, month, day, response, response_length);
			}
			break;
		default:
			throw rts2core::Error ("Invalid path for observations!");
	}
}

void Night::printAllImages (int year, int month, int day, XmlRpc::HttpParams *params, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	std::ostringstream title;
	title << "Observations";

	if (year > 0)
	{
		title << " for " << year;
		if (month > 0)
		{
			title << "-" << month;
			if (day > 0)
			{
				title << "-" << day;
			}
		}
	}

	printHeader (_os, title.str ().c_str ());

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
	int colourVariant = params->getInteger ("cv", DEFAULT_COLOURVARIANT);

	time_t from;
	int64_t duration;

	getNightDuration (year, month, day, from, duration);

	time_t end = from + duration;

	Previewer preview = Previewer (getServer ());
	preview.script (_os, label_encoded, quantiles, chan, colourVariant);

	_os << "<p>";

	preview.form (_os, pageno, prevsize, pagesiz, chan, label, quantiles, colourVariant);

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
		preview.imageHref (_os, in, (*iter)->getAbsoluteFileName (), prevsize, label_encoded, quantiles, chan, colourVariant);
	}

	_os << "</p><p>Page ";
	int i;
	for (i = 1; i <= ((int) is.size ()) / pagesiz; i++)
	 	preview.pageLink (_os, i, pagesiz, prevsize, label_encoded, i == pageno, quantiles, chan, colourVariant);
	if (in % pagesiz)
	 	preview.pageLink (_os, i, pagesiz, prevsize, label_encoded, i == pageno, quantiles, chan, colourVariant);
	_os << "</p></body></html>";

	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

#ifdef RTS2_HAVE_LIBJPEG
void Night::printAlt (int year, int month, int day, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	response_type = "image/jpeg";

	AltPlot ap (params->getInteger ("w", 800), params->getInteger ("h", 600));
	Magick::Geometry size (params->getInteger ("w", 800), params->getInteger ("h", 600));

	time_t from;
	int64_t duration;

	getNightDuration (year, month, day, from, duration);

	time_t end = from + duration;

	rts2db::ImageSetDate is = rts2db::ImageSetDate (from, end);
	is.load ();

	Magick::Image mimage (size, "white");
	ap.getPlot (from, end, &is, &mimage);

	Magick::Blob blob;
	mimage.write (&blob, "JPEG");

	response_length = blob.length();
	response = new char[response_length];
	memcpy (response, blob.data(), response_length);
}

void Night::printAltAz (int year, int month, int day, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
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
			(*iter)->getCoordBestAltAz (hrz, rts2core::Configuration::instance ()->getObserver ());
			(*iter)->closeFile ();
			altaz.plotCross (&hrz, NULL, "green");
		}
		catch (rts2core::Error &er)
		{
			(*iter)->closeFile ();
		}
	}

	Magick::Blob blob;
	altaz.write (&blob, "JPEG");

	response_length = blob.length();
	response = new char[response_length];
	memcpy (response, blob.data(), response_length);
}
#endif // RTS2_HAVE_LIBJPEG

void Night::callAPI (int year, int month, int day, char* &response, const char* &response_type, size_t &response_length)
{
	std::ostringstream _os;

	if (year <= 0 || month <= 0 || day <= 0)
	{
		_os << "{\"h\":["
			"{\"n\":\"Date part\",\"t\":\"a\",\"c\":0,\"prefix\":\"\",\"href\":0},"
			"{\"n\":\"Number of observations\",\"t\":\"n\",\"c\":1},"
			"{\"n\":\"Number of images\",\"t\":\"n\",\"c\":2},"
			"{\"n\":\"Number of good images\",\"t\":\"n\",\"c\":3},"
			"{\"n\":\"Time on sky\",\"t\":\"dur\",\"c\":4}"
			"],\"d\":[";

		rts2db::ObservationSetDate as = rts2db::ObservationSetDate ();
		as.load (year, month, day);

		for (rts2db::ObservationSetDate::iterator iter = as.begin (); iter != as.end (); iter++)
		{
			if (iter != as.begin ())
				_os << ",";
			_os << "[" << iter->first << "," << iter->second.c << "," << iter->second.i << "," << iter->second.gi << "," << rts2json::JsonDouble (iter->second.tt) << "]\n";
		}
		_os << "]}";
	}
	else
	{
		_os << "{\"h\":["
			"{\"n\":\"ID\",\"t\":\"a\",\"c\":0,\"prefix\":\"" << getServer ()->getPagePrefix () << "/observations/\",\"href\":0},"
			"{\"n\":\"TargetID\",\"t\":\"a\",\"c\":1,\"prefix\":\"" << getServer ()->getPagePrefix () << "/targets/\",\"href\":1},"
			"{\"n\":\"Target name\",\"t\":\"a\",\"c\":2,\"prefix\":\"" << getServer ()->getPagePrefix () << "/targets/\",\"href\":1},"
			"{\"n\":\"Slew\",\"t\":\"t\",\"c\":3},"
			"{\"n\":\"Start\",\"t\":\"tT\",\"c\":4},"
			"{\"n\":\"End\",\"t\":\"tT\",\"c\":5},"
			"{\"n\":\"Number of images\",\"t\":\"n\",\"c\":6},"
			"{\"n\":\"Number of good images\",\"t\":\"n\",\"c\":7},"
			"{\"n\":\"Time on sky\",\"t\":\"dur\",\"c\":8}"
			"],\"d\":[";

		rts2db::ObservationSet os = rts2db::ObservationSet ();

		time_t from;
		int64_t duration;

		getNightDuration (year, month, day, from, duration);

		time_t end = from + duration;

		os.loadTime (&from, &end);

		_os << std::fixed;

		for (rts2db::ObservationSet::iterator iter = os.begin (); iter != os.end (); iter++)
		{
			if (iter != os.begin ())
				_os << ",";
			_os << "[" << iter->getObsId () << ","
				<< iter->getTargetId () << ","
				<< "\"" << iter->getTargetName () << "\","
				<< rts2json::JsonDouble (iter->getObsSlew ()) << ","
				<< rts2json::JsonDouble (iter->getObsStart ()) << ","
				<< rts2json::JsonDouble (iter->getObsEnd ()) << ","
				<< iter->getNumberOfImages () << ","
				<< iter->getNumberOfGoodImages () << ","
				<< rts2json::JsonDouble (iter->getTimeOnSky ())
				<< "]";
		}

		_os << "]}";
	}

	returnJSON (_os, response_type, response, response_length);
}

void Night::printTable (int year, int month, int day, char* &response, size_t &response_length)
{
	std::ostringstream _os, _title;

	_title << "Observations";

	if (year > 0)
	{
		_title << " for " << year;
		if (month > 0)
		{
			_title << "-" << month;
			if (day > 0)
			{
				_title << "-" << day;
			}
		}
	}

	printHeader (_os, _title.str ().c_str (), NULL, "/css/table.css", "observations.refresh ()");

	includeJavaScriptWithPrefix (_os, "table.js");

	_os << "<p><a href='all'>All images</a>";

	_os << "<script type='text/javascript'>\n"
		"observations = new Table('api','observations','observations');\n"

		"</script>\n";

#ifdef RTS2_HAVE_LIBJPEG
	_os << "&nbsp;<a href='altaz'>Night images alt-az plot</a>&nbsp;<a href='alt'>Night images alt</a>";
#endif // RTS2_HAVE_LIBJPEG

	_os << "</p><p><div id='observations'>Loading..</div>";

	_os << "</p>";

	printFooter (_os);

	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

#endif /* RTS2_HAVE_PGSQL */

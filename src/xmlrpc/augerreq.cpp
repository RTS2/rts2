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

#include "augerreq.h"
#include "rts2json/nightdur.h"

#ifdef RTS2_HAVE_PGSQL
#include "../../lib/rts2db/augerset.h"
#if defined(RTS2_HAVE_LIBJPEG) && RTS2_HAVE_LIBJPEG == 1
#include "rts2json/altaz.h"
#endif // RTS2_HAVE_LIBJPEG
#include "configuration.h"

using namespace XmlRpc;
using namespace rts2xmlrpc;

void Auger::authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	response_type = "text/html";

	// get path and possibly date range
	std::vector <std::string> vals = SplitStr (path.substr (1), std::string ("/"));
	int year = -1;
	int month = -1;
	int day = -1;

	switch (vals.size ())
	{
		case 4:
			// assumes that all previous are OK, get just target
			printTarget (atoi (vals[3].c_str ()), response_type, response, response_length);
			break;			
		case 3:
			day = atoi (vals[2].c_str ());
		case 2:
			month = atoi (vals[1].c_str ());
		case 1:
			year = atoi (vals[0].c_str ());
		case 0:
			printTable (year, month, day, response, response_length);
			break;
		default:
			throw rts2core::Error ("Invalid path for graph!");
	}
}

void Auger::printTarget (int auger_id, const char* &response_type, char* &response, size_t &response_length)
{
	rts2db::TargetAuger ta (-1, rts2core::Configuration::instance ()->getObserver (), -1);

	ta.load (auger_id);

	std::vector <struct ln_equ_posn> pos;
	ta.getEquPositions (pos);


#if defined(RTS2_HAVE_LIBJPEG) && RTS2_HAVE_LIBJPEG == 1
	double JD = ta.getShowerJD ();

	AltAz aa = AltAz ();

	aa.plotAltAzGrid ();

	int i = 1;

	struct ln_hrz_posn hrz;

	// plot origin
	ta.getAltAz (&hrz, JD);
	aa.plotCross (&hrz, "Origin", "green");

	for (std::vector <struct ln_equ_posn>::iterator iter = pos.begin (); iter != pos.end (); iter++)
	{
		ln_get_hrz_from_equ (&(*iter), rts2core::Configuration::instance ()->getObserver (), JD, &hrz);

		std::ostringstream _os;
		_os << i;

		aa.plotCross (&hrz, _os.str ().c_str ());
		i++;
	}

	Magick::Blob blob;
	aa.write (&blob, "jpeg");

	response_type = "image/jpeg";

	response_length = blob.length();
	response = new char[response_length];
	memcpy (response, blob.data(), response_length);
#else
	throw XmlRpcException ("Images not supported");
#endif // RTS2_HAVE_LIBJPEG
}

void Auger::listAuger (int year, int month, int day, std::ostringstream &_os)
{
	rts2db::AugerSet as = rts2db::AugerSet ();

	time_t from;
	int64_t duration;

	getNightDuration (year, month, day, from, duration);

	as.load (from, from + duration);

	as.printHTMLTable (_os);
}

void Auger::printTable (int year, int month, int day, char* &response, size_t &response_length)
{
	bool do_list = false;
	std::ostringstream _os;

	std::ostringstream title;
	title << "Auger showers";

	if (year > 0)
	{
		title << " for " << year;
		if (month > 0)
		{
			title << "-" << month;
			if (day > 0)
			{
				title << "-" << day;
				do_list = true;
			}
		}
	}

	printHeader (_os, title.str ().c_str ());

	if (year == 0 || month == 0 || day == 0)
		do_list = true;

	_os << "<p><table>";

	if (do_list == true)
	{
		listAuger (year, month, day, _os);
	}
	else
	{
		rts2db::AugerSetDate as = rts2db::AugerSetDate ();
		as.load (year, month, day);

		for (rts2db::AugerSetDate::iterator iter = as.begin (); iter != as.end (); iter++)
		{
			_os << "<tr><td><a href='" << iter->first << "/'>" << iter->first << "</a></td><td>" << iter->second << "</td></tr>";
		}

	}

	_os << "</table>";
	
	printFooter (_os);

	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

#endif /* RTS2_HAVE_PGSQL */

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

#include "xmlrpcd.h"
#include "xmlrpc++/XmlRpcException.h"
#include "xmlrpc++/urlencoding.h"
#include "rts2db/observation.h"

#ifdef RTS2_HAVE_PGSQL

using namespace rts2xmlrpc;

void Observation::authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	// get path and possibly date range
	std::vector <std::string> vals = SplitStr (path.substr (1), std::string ("/"));

	switch (vals.size ())
	{
		case 0:
			printQuery (response_type, response, response_length);
			return;
		case 1:
			printObs (atoi (vals[0].c_str ()), params, response_type, response, response_length);
			return;
		case 2:
			if (vals[1] == "api")
			{
				obsApi (atoi (vals[0].c_str ()), params, response_type, response, response_length);
				return;
			}
	}
	throw rts2core::Error ("Invalid path");
}

void Observation::printQuery (const char* &response_type, char* &response, size_t &response_length)
{

}

void Observation::printObs (int obs_id, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	rts2db::Observation obs (obs_id);
	if (obs.load () || obs.loadImages ())
		throw XmlRpc::XmlRpcException ("Cannot load observation");

	int pageno = params->getInteger ("p", 1);
	int pagesiz = params->getInteger ("s", 40);

	if (pageno <= 0)
		pageno = 1;

	int istart = (pageno - 1) * pagesiz;
	int ie = istart + pagesiz;
	int in = 0;

	int prevsize = params->getInteger ("ps", 128);
	const char * label = params->getString ("lb", ((XmlRpcd *) getMasterApp ())->getDefaultImageLabel ());
	std::string lb (label);
	XmlRpc::urlencode (lb);
	const char * label_encoded = lb.c_str ();

	float quantiles = params->getDouble ("q", DEFAULT_QUANTILES);
	int chan = params->getInteger ("chan", ((XmlRpcd *) getMasterApp ())->defchan);

	std::ostringstream _os;
	std::ostringstream _title;
	_title << "Images for observation " << obs.getObsId () << " of " << obs.getTargetName ();

	Previewer preview = Previewer ();

	printHeader (_os, _title.str ().c_str (), preview.style ());

	preview.script (_os, label_encoded, quantiles, chan);

	_os << "<p>";

	preview.form (_os, pageno, prevsize, pagesiz, chan, label);

	_os << "</p><p>";

	for (rts2db::ImageSet::iterator iter = obs.getImageSet ()->begin (); iter != obs.getImageSet ()->end (); iter++)
	{
		in++;
		if (in <= istart)
			continue;
		if (in > ie)
			break;
		preview.imageHref (_os, in, (*iter)->getAbsoluteFileName (), prevsize, label_encoded, quantiles, chan);
	}

	_os << "</p><p>Page ";
	int i;
	for (i = 1; i <= ((int) obs.getImageSet ()->size ()) / pagesiz; i++)
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

void Observation::obsApi (int obs_id, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;
	rts2db::Observation obs (obs_id);
	rts2db::ImageSetObs images (&obs);
	images.load ();
	_os << "[" << std::fixed;
	
	for (rts2db::ImageSetObs::iterator iter = images.begin (); iter != images.end (); iter++)
	{
		if (iter != images.begin ())
			_os << ",";
		_os << "[\"" << (*iter)->getAbsoluteFileName () << "\","
			<< (*iter)->getExposureLength () << ","
			<< (*iter)->getExposureStart ()
			<< "]";
	}

	_os << "]";
	
	returnJSON (_os, response_type, response, response_length);
}

#endif /* RTS2_HAVE_PGSQL */

/* 
 * API and pages for adding new target.
 * Copyright (C) 2013 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "rts2db/target.h"
#include "rts2db/targetset.h"
#include "rts2db/simbadtargetdb.h"
#include "rts2json/addtargetreq.h"

#include "radecparser.h"

using namespace rts2json;

void AddTarget::authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	// get path and possibly date range
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));

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
				newTarget (params->getString ("oriname", ""), params->getString ("name", ""), params->getInteger ("tarid", INT_MAX), params->getDouble ("ra", NAN), params->getDouble ("dec", NAN), response_type, response, response_length);
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

void AddTarget::askForTarget (const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	printHeader (_os, "Add new target");

	_os << "<p>Provide anything that can be used to identify what you would like to observe. These are valid formats:<ul>"
		"<li>RA DEC in various formats. DEC must be separated with +/- sign. These are valid inputs:"
		  "<ul><li>300 +24</li> <li>10:20 +33:12</li> <li>10:20:30 -12:34:59</li>"
		"</ul></li>"
		"<li>SIMBAD resolvable name</li>"
		"<li>any target from RTS2 target database</li>"
		"<li>any Minor planet name resolvable by MPEC</li>"
		"<li>one line MPEC</li>"
		"</ul></p><form name='add_target' action='confirm' method='get'><input type='text' textwidth='20' name='target'/><input type='submit' value='Add'/></form>";
	
	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void AddTarget::confimTarget (const char *tar, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	if (strlen (tar) == 0)
	{
		throw XmlRpc::XmlRpcException ("Invalid target - valid targer name must contain at least one character.");
	}
	else
	{
		printHeader (_os, "Confirm target");

		struct ln_equ_posn pos;

		pos.ra = pos.dec = NAN;

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
		rts2db::Target *target = new rts2db::SimbadTargetDb (tar);
		target->load ();
		target->getPosition (&pos);
		delete target;

		// print new target
		if (! (isnan (pos.ra) || isnan (pos.dec)))
		{
			_os << "<p>If you would like to enter new target with RA DEC " << LibnovaRaDec (pos.ra, pos.dec)
				<< ", then fill fields bellow and click submit: <form name='new_target' action='new_target' method='get'><input type='hidden' name='ra' value='"
				<< pos.ra << "'/><input type='hidden' name='dec' value='" << pos.dec 
				<< "'><input type='hidden' name='oriname' value='" << tar << "'/>Target name:<input type='text' name='name' value='"
				<< tar << "'/><br/>Optional target ID - left blank for autogenerated<input type='text' name='tarid' value=' '/><br/><input type='submit' value='Create target'/></form></p>";
		}

		printFooter (_os);
	}

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void AddTarget::newTarget (const char *oriname, const char *name, int tarid, double ra, double dec, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	rts2db::ConstTarget *constTarget = new rts2db::ConstTarget ();
	constTarget->setPosition (ra, dec);
	constTarget->setTargetName (name);
	constTarget->setTargetType (TYPE_OPORTUNITY);

	int ret;

	if (tarid != INT_MAX)
		ret = constTarget->saveWithID (false, tarid);
	else
		ret = ((rts2db::Target *) constTarget)->save (false);
	
	if (ret)
		throw XmlRpc::XmlRpcException ("Target with given ID already exists");

	printHeader (_os, "Create new target");
	
	_os << "<p>Target with name " << name << " and ID " << constTarget->getTargetID ()
		<< " sucessfully created. Please click <a href='schedule?tarid=" << constTarget->getTargetID ()
		<< "'>here</a> to continue with scheduling the target.</p>";
		
	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void AddTarget::schedule (int tarid, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	rts2db::Target *tar = createTarget (tarid, rts2core::Configuration::instance ()->getObserver (), rts2core::Configuration::instance ()->getObservatoryAltitude ());

	if (tar == NULL)
		throw XmlRpc::XmlRpcException ("Cannot find target with given ID!");

	printHeader (_os, (std::string ("Scheduling target ") + tar->getTargetName ()).c_str ());
	
	_os << "<p>Please choose how your observations should be scheduled and peformed:<form name=''";

	_os << "</p>";

	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

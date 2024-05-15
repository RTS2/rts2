/*
 * Target resultion.
 * Copyright (C) 2005-2016 Petr Kubanek <petr@kubanek.net>
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

#include "configuration.h"
#include "rts2db/simbadtargetdb.h"
#include "rts2db/mpectarget.h"
#include "rts2db/tletarget.h"

#include "rts2db/targetres.h"

#include "xmlrpc++/XmlRpc.h"
#include "xmlrpc++/urlencoding.h"

using namespace rts2db;
using namespace XmlRpc;

Target *createTargetByString (std::string tar_string, bool debug)
{
	Target *rtar = NULL;

	LibnovaRaDec raDec;

	if (tar_string.length() == 0)
	{
		// Empty target
		ConstTarget *constTarget = new ConstTarget ();
		constTarget->setPosition (NAN, NAN);
		constTarget->setTargetName ("Empty target");
		constTarget->setTargetType (TYPE_OPORTUNITY);
		return constTarget;
	}

	int ret = raDec.parseString (tar_string.c_str ());
	if (ret == 0)
	{
		std::string new_prefix;

		// default prefix for new RTS2 sources
		new_prefix = std::string ("RTS2_");

		// set name..
		ConstTarget *constTarget = new ConstTarget ();
		constTarget->setPosition (raDec.getRa (), raDec.getDec ());
		std::ostringstream os;

		rts2core::Configuration::instance ()->getString ("newtarget", "prefix", new_prefix, "RTS2");
		os << new_prefix << LibnovaRaComp (raDec.getRa ()) << LibnovaDeg90Comp (raDec.getDec ());
		constTarget->setTargetName (os.str ().c_str ());
		constTarget->setTargetType (TYPE_OPORTUNITY);
		return constTarget;
	}
	// if it's MPC ephemeris..
	rtar = new EllTarget ();
	ret = ((EllTarget *) rtar)->orbitFromMPC (tar_string.c_str ());
	if (ret == 0)
		return rtar;

	delete rtar;

	try
	{
		rtar = new TLETarget ();
		((TLETarget *) rtar)->orbitFromTLE (tar_string);
		return rtar;
	}
	catch (rts2core::Error &er)
	{
	}

	delete rtar;

	XmlRpcLogHandler::setVerbosity (debug ? 5 : 0);

	// try to get target from SIMBAD
	try
	{
		rtar = new rts2db::SimbadTargetDb (tar_string.c_str ());
		rtar->load ();
		rtar->setTargetType (TYPE_OPORTUNITY);
		return rtar;
	}
	catch (rts2core::Error &er)
	{
		delete rtar;

		// MPEC fallback
		rtar = new rts2db::MPECTarget (tar_string.c_str ());
		rtar->load ();
		return rtar;
	}
}

Target *createEmptyTarget()
{
	ConstTarget *constTarget = new ConstTarget ();

	constTarget->setTargetName("RTS2_EMPTY");
	constTarget->setPosition(NAN, NAN);
	constTarget->setTargetType (TYPE_OPORTUNITY);
	return constTarget;
}

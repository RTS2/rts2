/*
 * Class for elliptical bodies.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#include "rts2db/targetell.h"

#include "infoval.h"
#include "libnova_cpp.h"
#include "rts2fits/image.h"

using namespace rts2db;

// EllTarget - for comments and other solar system rocks
EllTarget::EllTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):Target (in_tar_id, in_obs)
{
}

void EllTarget::load ()
{
	Target::load ();
	// try to parse MPC string..
	int ret = LibnovaEllFromMPC (&orbit, designation, getTargetInfo ()) && LibnovaEllFromMPCComet (&orbit, designation, getTargetInfo ());
	if (ret)
	  	throw rts2core::Error (std::string ("cannot parse MPEC string ") + getTargetInfo ());
}

int EllTarget::orbitFromMPC (const char *mpc)
{
	int ret;
	if ((ret = LibnovaEllFromMPC (&orbit, designation, mpc)) && (ret = LibnovaEllFromMPCComet (&orbit, designation, mpc)))
		return ret;

	setTargetName (designation.c_str ());
	setTargetInfo (mpc);
	setTargetType (TYPE_ELLIPTICAL);
	return ret;
}

void EllTarget::getPosition (struct ln_equ_posn *pos, double JD, struct ln_equ_posn *parallax)
{
	LibnovaCurrentFromOrbit (pos, &orbit, observer, 1706, JD, parallax);
}

void EllTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
	struct ln_equ_posn parallax;
	getPosition (pos, JD, &parallax);
}

int EllTarget::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
	if (orbit.e == 1.0)
	{
		struct ln_par_orbit par_orbit;
		par_orbit.q = orbit.a;
		par_orbit.i = orbit.i;
		par_orbit.w = orbit.w;
		par_orbit.omega = orbit.omega;
		par_orbit.JD = orbit.JD;
		return ln_get_par_body_next_rst_horizon (JD, observer, &par_orbit, horizon, rst);
	}
	else if (orbit.e > 1.0)
	{
		struct ln_hyp_orbit hyp_orbit;
		hyp_orbit.q = orbit.a;
		hyp_orbit.e = orbit.e;
		hyp_orbit.i = orbit.i;
		hyp_orbit.w = orbit.w;
		hyp_orbit.omega = orbit.omega;
		hyp_orbit.JD = orbit.JD;
		return ln_get_hyp_body_next_rst_horizon (JD, observer, &hyp_orbit, horizon, rst);
	}
	return ln_get_ell_body_next_rst_horizon (JD, observer, &orbit, horizon, rst);
}

void EllTarget::printExtra (Rts2InfoValStream & _os, double JD)
{
	struct ln_equ_posn pos, parallax;
	getPosition (&pos, JD, &parallax);
	_os
		<< InfoVal<TimeJD> ("EPOCH", TimeJD (orbit.JD));
	if (orbit.e < 1.0)
	{
		_os
			<< InfoVal<double> ("n", orbit.n)
			<< InfoVal<double> ("a", orbit.a);
	}
	else if (orbit.e > 1.0)
	{
		_os
			<< InfoVal<double> ("q", orbit.a);
	}
	_os
		<< InfoVal<double> ("e", orbit.e)
		<< InfoVal<double> ("Peri.", orbit.w)
		<< InfoVal<double> ("Node", orbit.omega)
		<< InfoVal<double> ("Incl.", orbit.i)
		<< std::endl
		<< InfoVal<double> ("EARTH DISTANCE", getEarthDistance (JD))
		<< InfoVal<double> ("SOLAR DISTANCE", getSolarDistance (JD))
		<< InfoVal<LibnovaDegDist> ("PARALLAX RA", LibnovaDegDist (parallax.ra))
		<< InfoVal<LibnovaDegDist> ("PARALLAX DEC", LibnovaDegDist (parallax.dec))
		<< std::endl;
	Target::printExtra (_os, JD);
}

void EllTarget::writeToImage (rts2image::Image * image, double JD)
{
	Target::writeToImage (image, JD);
	image->setValue ("ELL_EPO", orbit.JD, "epoch of the orbit");
	if (orbit.e < 1.0)
	{
		image->setValue ("ELL_N", orbit.n, "n parameter of the orbit");
		image->setValue ("ELL_A", orbit.a, "a parameter of the orbit");
	}
	else if (orbit.e > 1.0)
	{
		image->setValue ("ELL_Q", orbit.a, "q parameter of the orbit");
	}
	image->setValue ("ELL_E", orbit.e, "orbit eccentricity");
	image->setValue ("ELL_PERI", orbit.w, "perihelium parameter");
	image->setValue ("ELL_NODE", orbit.omega, "node angle");
	image->setValue ("ELL_INCL", orbit.i, "orbit inclination");

	image->setValue ("EARTH_DISTANCE", getEarthDistance (JD), "Earth distance (in AU)");
	image->setValue ("SOLAR_DISTANCE", getSolarDistance (JD), "Solar distance (in AU)");

	struct ln_equ_posn pos, parallax;
	getPosition (&pos, JD, &parallax);

	image->setValue ("PARALLAX_RA", parallax.ra, "RA parallax");
	image->setValue ("PARALLAX_RA", parallax.dec, "DEC parallax");
}

double EllTarget::getEarthDistance (double JD)
{
	return LibnovaEarthDistanceFromMpec (&orbit, JD);
}

double EllTarget::getSolarDistance (double JD)
{
	if (orbit.e == 1.0)
	{
		struct ln_par_orbit par_orbit;
		par_orbit.q = orbit.a;
		par_orbit.i = orbit.i;
		par_orbit.w = orbit.w;
		par_orbit.omega = orbit.omega;
		par_orbit.JD = orbit.JD;
		return ln_get_par_body_solar_dist (JD, &par_orbit);
	}
	else if (orbit.e > 1.0)
	{
		struct ln_hyp_orbit hyp_orbit;
		hyp_orbit.q = orbit.a;
		hyp_orbit.e = orbit.e;
		hyp_orbit.i = orbit.i;
		hyp_orbit.w = orbit.w;
		hyp_orbit.omega = orbit.omega;
		hyp_orbit.JD = orbit.JD;
		return ln_get_hyp_body_solar_dist (JD, &hyp_orbit);
	}
	return ln_get_ell_body_solar_dist (JD, &orbit);
}

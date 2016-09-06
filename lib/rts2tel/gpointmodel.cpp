/* 
 * RTS2 telescope pointing model.
 * Copyright (C) 2015 Petr Kubanek <petr@kubanek.net>
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

#include "gpointmodel.h"
#include "libnova_cpp.h"
#include "error.h"

#include <math.h>
#include <fstream>

using namespace rts2telmodel;

ExtraParam::ExtraParam ()
{
	for (int i=0; i < MAX_TERMS; i++)
	{
		params[i] = NAN;
		terms[i] = GPOINT_LASTTERM;
		consts[i] = NAN;
	}
}

// function strings
const char *ExtraParam::fns[] = {"sin", "cos", "tan", "sincos", "coscos", "sinsin"};
const char *ExtraParam::pns[] = {"az", "el", "zd"};

void ExtraParam::parse (char *line)
{
#define DELIMS   " \t"
	char *saveptr;
	// first token
	char *p = strtok_r (line, DELIMS, &saveptr);
	if (p == NULL)
		throw rts2core::Error ("missing parameter value");
	char *endp;
	int t = 0;
	char *termssave;
	char *pp = strtok_r (p, ";", &termssave);
	while (t < MAX_TERMS && pp != NULL)
	{
		params[0] = strtod (pp, &endp);
		if (endp == NULL)
			throw rts2core::Error ("invalid parameter value", pp);
		t++;
		pp = strtok_r (NULL, ";", &termssave);
	}
	if (t == 0)
		throw rts2core::Error ("invalid param line", p);

	p = strtok_r (NULL, DELIMS, &saveptr);
	if (p == NULL)
		throw rts2core::Error ("missing function name");
	int i;
	for (i = 0; i < GPOINT_LASTFUN; i++)
	{
		if (strcasecmp (p, fns[i]) == 0)
		{
			function = static_cast<function_t> (i);
			break;
		}
	}
	if (i == GPOINT_LASTFUN)
		throw rts2core::Error ("unknow function name", p);

	p = strtok_r (NULL, DELIMS, &saveptr);
	if (p == NULL)
		throw rts2core::Error ("missing parameters");

	t = 0;
	pp = strtok_r (p, ";", &termssave);
	while (t < MAX_TERMS && pp != NULL)
	{
		for (i = 0; i < GPOINT_LASTTERM; i++)
		{
			if (strcasecmp (pp, pns[i]) == 0)
			{
				terms[t] = static_cast<terms_t> (i);
				break;
			}
		}
		if (i == GPOINT_LASTTERM)
			throw rts2core::Error ("invalid term name", pp);
		t++;
		pp = strtok_r (NULL, ";", &termssave);
	}
	if (t == 0)
		throw rts2core::Error ("invalid terms", p);

	p = strtok_r (NULL, DELIMS, &saveptr);
	if (p == NULL)
	{
		for (i = 0; i < t; i++)
			consts[i] = 1;
		return;
	}
	int tmax = t;
	pp = strtok_r (p, ";", &termssave);
	t = 0;
	while (t < MAX_TERMS && pp != NULL)
	{
		consts[t] = strtod (pp, &endp);
		if (endp == NULL)
			throw rts2core::Error ("invalid constant", pp);
		t++;
		pp = strtok_r (NULL, ";", &termssave);
	}
	for (; t < tmax; t++)
		consts[t] = 1;
}

double ExtraParam::getParamValue (double az, double el, int p)
{
	switch (terms[p])
	{
		case GPOINT_AZ:
			return az;
		case GPOINT_EL:
			return el;
		case GPOINT_ZD:
			return M_PI / 2.0 - el;
		default:
			return 0;
	}
}

double ExtraParam::getValue (double az, double el)
{
	switch (function)
	{
		case GPOINT_SIN:
			return params[0] * sin (consts[0] * getParamValue (az, el, 0));
		case GPOINT_COS:
			return params[0] * cos (consts[0] * getParamValue (az, el, 0));
		case GPOINT_TAN:
			return params[0] * tan (consts[0] * getParamValue (az, el, 0));
		case GPOINT_SINCOS:
			return params[0] * sin (consts[0] * getParamValue (az, el, 0)) * cos (consts[1] * getParamValue (az, el, 1));
		case GPOINT_COSCOS:
			return params[0] * cos (consts[0] * getParamValue (az, el, 0)) * cos (consts[1] * getParamValue (az, el, 1));
		case GPOINT_SINSIN:
			return sin (consts[0] * getParamValue (az, el, 0)) * sin (consts[1] * getParamValue (az, el, 1));
		default:
			return 0;
	}
}

GPointModel::GPointModel (rts2teld::Telescope * in_telescope, const char *in_modelFile):TelModel (in_telescope, in_modelFile)
{
	altaz = false;
	for (int i = 0; i < 9; i++)
		params[i] = 0;
}

GPointModel::~GPointModel (void)
{
}

int GPointModel::load ()
{
	std::ifstream is (modelFile);
	load (is);
	if (is.fail ())
		return -1;
	return 0;
}

int GPointModel::apply (struct ln_equ_posn *pos)
{
	double d_tar, r_tar;
	pos->ra = ln_deg_to_rad (pos->ra);
	pos->dec = ln_deg_to_rad (pos->dec);

	double lat_r = cond->getLatitudeRadians ();

	d_tar = pos->dec - params[0] - params[1] * cos (pos->ra) - params[2] * sin (pos->ra) - params[3] * (cos (lat_r) * sin (pos->dec) * cos (pos->ra) - sin (lat_r) * cos(pos->dec)) - params[8] * cos (pos->ra);
	r_tar = pos->ra - params[4] - params[5] / cos (pos->dec) - params[6] * tan (pos->dec) - (params[1] * sin (pos->ra) - params[2] * cos (pos->ra)) * tan (pos->dec) - params[3] * cos (lat_r) * sin (pos->ra) / cos (pos->dec) - params[7] * (sin (lat_r) * tan (pos->dec) + cos (lat_r) * cos (pos->ra));

	pos->ra = ln_rad_to_deg (r_tar);
	pos->dec = ln_rad_to_deg (d_tar);

	return 0;
}

int GPointModel::applyVerbose (struct ln_equ_posn *pos)
{
	logStream (MESSAGE_DEBUG) << "Before: " << pos->ra << " " << pos->dec << sendLog;
	apply (pos);
	logStream (MESSAGE_DEBUG) << "After: " << pos->ra << " " << pos->dec << sendLog;
	return 0;
}

int GPointModel::reverse (struct ln_equ_posn *pos)
{
	double d_tar, r_tar;
	pos->ra = ln_deg_to_rad (pos->ra);
	pos->dec = ln_deg_to_rad (pos->dec);

	double lat_r = cond->getLatitudeRadians ();

	d_tar = pos->dec + params[0] + params[1] * cos (pos->ra) + params[2] * sin (pos->ra) + params[3] * (cos (lat_r) * sin (pos->dec) * cos (pos->ra) - sin (lat_r) * cos(pos->dec)) + params[8] * cos (pos->ra);
	r_tar = pos->ra + params[4] + params[5] / cos (pos->dec) + params[6] * tan (pos->dec) + (params[1] * sin (pos->ra) - params[2] * cos (pos->ra)) * tan (pos->dec) + params[3] * cos (lat_r) * sin (pos->ra) / cos (pos->dec) + params[7] * (sin (lat_r) * tan (pos->dec) + cos (lat_r) * cos (pos->ra));

	pos->ra = ln_rad_to_deg (r_tar);
	pos->dec = ln_rad_to_deg (d_tar);

	return 0;
}

int GPointModel::reverseVerbose (struct ln_equ_posn *pos)
{
	logStream (MESSAGE_DEBUG) << "Before: " << pos->ra << " " << pos->dec << sendLog;
	reverse (pos);
	logStream (MESSAGE_DEBUG) << "After: " << pos->ra << " " << pos->dec << sendLog;

	return 0;
}

int GPointModel::reverse (struct ln_equ_posn *pos, double sid)
{
	return reverse (pos);
}

void GPointModel::getErrAltAz (struct ln_hrz_posn *hrz, struct ln_hrz_posn *err)
{
	double az_r, el_r;
	az_r = ln_deg_to_rad (hrz->az);
	el_r = ln_deg_to_rad (hrz->alt);

	err->az = - params[0] \
		- params[1] * sin (az_r)  * tan (el_r) \
		- params[2] * cos (az_r) * tan (el_r) \
		- params[3] * tan (el_r) \
		+ params[4] / cos (el_r);

	err->alt = - params[5] \
		- params[2] * sin (az_r) \
		+ params[6] * cos (el_r) \
		+ params[7] * cos (az_r) \
		+ params[8] * sin (az_r);

	err->az = ln_rad_to_deg (err->az);
	err->alt = ln_rad_to_deg (err->alt);

	hrz->az += err->az;
	hrz->alt += err->alt;
}

std::istream & GPointModel::load (std::istream & is)
{
	std::string name;

	is >> name;

	int pn = 9;

	if (name == "RTS2_ALTAZ")
	{
		altaz = true;
	}

	int i = 0;

	while (!is.eof () && i < pn)
	{
		is >> params[i];
		if (is.fail ())
		{
			logStream (MESSAGE_ERROR) << "cannot read parameter " << i << sendLog;
			return is;
		}
		i++;
	}
	return is;
}

std::ostream & GPointModel::print (std::ostream & os)
{
	int pn = 9;
	if (altaz)
	{
		os << "RTS2_ALTAZ";
	}
	else
	{
		os << "RTS2_MODEL";
	}
	for (int i = 0; i < pn; i++)
		os << " " << params[i];
	return os;
}

inline std::istream & operator >> (std::istream & is, GPointModel * model)
{
	return model->load (is);
}

inline std::ostream & operator << (std::ostream & os, GPointModel * model)
{
	return model->print (os);
}

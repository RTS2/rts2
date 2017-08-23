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
const char *ExtraParam::fns[] = {"offset", "sin", "cos", "tan", "sincos", "coscos", "sinsin", "abssin", "abscos", "csc", "sec", "cot", "sinh", "cosh", "tanh", "sech", "csch", "coth"};
const char *ExtraParam::pns[] = {"az", "el", "zd", "ha", "dec", "pd"};

/**
 * Reads from input stream degree (radians, degrees ends with d, arcmin with ' or m, arcsec with " or s).
 */
std::istream & readDeg (std::istream & is, long double &p)
{
	is >> p;
	if (is.fail ())
		throw rts2core::Error ("cannot read parameter");

	// convert parameters to radians
	int nextch = is.get ();
	switch (nextch)
	{
		case 'd':
			p = ln_deg_to_rad (p);
			break;
		case '\'':
		case 'm':
			p = ln_deg_to_rad (p / 60.0);
			break;
		case '"':
		case 's':
			p = ln_deg_to_rad (p / 3600.0);
			break;
		case ';':
			break;
		default:
			if (is.eof () || isspace (nextch))
				return is;
			throw rts2core::Error ("invalid end character");
	}
	return is;
}

std::string printDeg (long double v, char frmt)
{
	std::ostringstream os;
	os.precision (20);
	os << std::fixed;
	switch (frmt)
	{
		case 'd':
			os << ln_rad_to_deg (v) << "d";
			break;
		case '\'':
		case 'm':
			os << (ln_rad_to_deg (v) * 60.0) << frmt;
			break;
		case '"':
		case 's':
			os << (ln_rad_to_deg (v) * 3600.0) << frmt;
			break;
		default:
			os << v;
	}
	return os.str ();
}


void ExtraParam::parse (std::istream &is)
{
	std::string p;
	is >> p;
	if (is.fail ())
		throw rts2core::Error ("missing parameter value", p.c_str ());
	int t = 0;
	std::istringstream pp (p);
	while (t < MAX_TERMS && !pp.eof ())
	{
		readDeg (pp, params[t]);
		if (pp.peek () == ';')
			pp.get ();
		t++;
	}
	if (t == 0)
		throw rts2core::Error ("invalid param line", p.c_str ());

	is >> p;
	if (is.fail ())
		throw rts2core::Error ("missing function name");

	ci_string ci_p (p.c_str ());

	int i;
	for (i = 0; i < GPOINT_LASTFUN; i++)
	{
		if (ci_p == fns[i])
		{
			function = static_cast<function_t> (i);
			break;
		}
	}
	if (i == GPOINT_LASTFUN)
		throw rts2core::Error ("unknow function name", p.c_str ());

	is >> p;
	if (is.fail ())
		throw rts2core::Error ("missing terms");

	t = 0;

	std::vector <std::string> sterms = SplitStr (p, ";");

	while (t < MAX_TERMS && t < (int) sterms.size ())
	{
		ci_string ci_t (sterms[t].c_str ());
		for (i = 0; i < GPOINT_LASTTERM; i++)
		{
			if (ci_t == pns[i])
			{
				terms[t] = static_cast<terms_t> (i);
				break;
			}
		}
		if (i == GPOINT_LASTTERM)
			throw rts2core::Error ("invalid term name", p.c_str ());
		t++;
	}
	if (t == 0)
		throw rts2core::Error ("invalid terms", p.c_str ());

	is >> p;
	if (is.fail ())
	{
		for (i = 0; i < t; i++)
			consts[i] = 1;
		return;
	}
	int tmax = t;
	size_t ci = 0;
	t = 0;
	while (t < MAX_TERMS && ci < p.length ())
	{
		char *endp;
		const char *c = p.c_str () + ci;
		consts[t] = strtod (c, &endp);
		t++;
		if (c  == endp)
			throw rts2core::Error ("invalid constant");
		if (isspace (*endp) || *endp == '\0')
			break;
		if (*endp != ';')
			throw rts2core::Error ("expected ;", p.c_str () + ci);
		ci += endp - c + 1;
	}
	for (; t < tmax; t++)
		consts[t] = 1;
}

double ExtraParam::getParamValue (double az, double el, double ha, double dec, int p)
{
	switch (terms[p])
	{
		case GPOINT_AZ:
			return az;
		case GPOINT_EL:
			return el;
		case GPOINT_ZD:
			return M_PI / 2.0 - el;
		case GPOINT_HA:
			return ha;
		case GPOINT_DEC:
			return dec;
		default:
			return 0;
	}
}

double ExtraParam::getValue (double az, double el, double ha, double dec)
{
	switch (function)
	{
		case GPOINT_OFFSET:
			return params[0];
		case GPOINT_SIN:
			return params[0] * sinl (consts[0] * getParamValue (az, el, ha, dec, 0));
		case GPOINT_COS:
			return params[0] * cosl (consts[0] * getParamValue (az, el, ha, dec, 0));
		case GPOINT_ABSSIN:
			return params[0] * abs (sinl (consts[0] * getParamValue (az, el, ha, dec, 0)));
		case GPOINT_ABSCOS:
			return params[0] * abs (cosl (consts[0] * getParamValue (az, el, ha, dec, 0)));
		case GPOINT_TAN:
			return params[0] * tanl (consts[0] * getParamValue (az, el, ha, dec, 0));
		case GPOINT_CSC:
			return params[0] / cosl (consts[0] * getParamValue (az, el, ha, dec, 0));
		case GPOINT_SEC:
			return params[0] / sinl (consts[0] * getParamValue (az, el, ha, dec, 0));
		case GPOINT_COT:
			return params[0] / tanl (consts[0] * getParamValue (az, el, ha, dec, 0));
		case GPOINT_SINH:
			return params[0] * sinhl (consts[0] * getParamValue (az, el, ha, dec, 0));
		case GPOINT_COSH:
			return params[0] * coshl (consts[0] * getParamValue (az, el, ha, dec, 0));
		case GPOINT_TANH:
			return params[0] * tanhl (consts[0] * getParamValue (az, el, ha, dec, 0));
		case GPOINT_SECH:
			return params[0] / sinhl (consts[0] * getParamValue (az, el, ha, dec, 0));
		case GPOINT_CSCH:
			return params[0] / coshl (consts[0] * getParamValue (az, el, ha, dec, 0));
		case GPOINT_COTH:
			return params[0] / tanhl (consts[0] * getParamValue (az, el, ha, dec, 0));
		case GPOINT_SINCOS:
			return params[0] * sinl (consts[0] * getParamValue (az, el, ha, dec, 0)) * cosl (consts[1] * getParamValue (az, el, ha, dec, 1));
		case GPOINT_COSCOS:
			return params[0] * cosl (consts[0] * getParamValue (az, el, ha, dec, 0)) * cosl (consts[1] * getParamValue (az, el, ha, dec, 1));
		case GPOINT_SINSIN:
			return sinl (consts[0] * getParamValue (az, el, ha, dec, 0)) * sinl (consts[1] * getParamValue (az, el, ha, dec, 1));
		default:
			return 0;
	}
}

std::string ExtraParam::toString (char frmt)
{
	std::ostringstream os;
	os.precision (20);
	os << std::fixed << printDeg (params[0], frmt) << " " << consts[0];
	return os.str ();
}

GPointModel::GPointModel (double in_latitude):TelModel (in_latitude)
{
	altaz = false;
	for (int i = 0; i < 9; i++)
		params[i] = 0;
}

GPointModel::~GPointModel (void)
{
	std::list <ExtraParam *>::iterator it;
	for (it = extraParamsAz.begin (); it != extraParamsAz.end (); it++)
		delete *it;

	for (it = extraParamsEl.begin (); it != extraParamsEl.end (); it++)
		delete *it;

	for (it = extraParamsHa.begin (); it != extraParamsHa.end (); it++)
		delete *it;

	for (it = extraParamsDec.begin (); it != extraParamsDec.end (); it++)
		delete *it;

}

int GPointModel::load (const char *modelFile)
{
	std::ifstream is (modelFile);
	load (is);
	return 0;
}

int GPointModel::apply (struct ln_equ_posn *pos)
{
	double d_tar, r_tar;
	pos->ra = ln_deg_to_rad (pos->ra);
	pos->dec = ln_deg_to_rad (pos->dec);

	double lat_r = getLatitudeRadians ();

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

int GPointModel::reverse (struct ln_equ_posn *pos, struct ln_hrz_posn *hrz)
{
	double d_tar, r_tar;
	double az_r, el_r, ha_r, dec_r;
	az_r = ln_deg_to_rad (hrz->az);
	el_r = ln_deg_to_rad (hrz->alt);
	ha_r = ln_deg_to_rad (pos->ra);
	dec_r = ln_deg_to_rad (pos->dec);

	double lat_r = getLatitudeRadians ();

	d_tar = dec_r
		+ params[0]
		+ params[1] * cos (ha_r)
		+ params[2] * sin (ha_r)
		+ params[3] * (cos (lat_r) * sin (dec_r) * cos (ha_r) - sin (lat_r) * cos(dec_r))
		+ params[8] * cos (ha_r);

	r_tar = ha_r
		+ params[4]
		+ params[5] / cos (dec_r)
		+ params[6] * tan (dec_r)
		+ (params[1] * sin (ha_r) - params[2] * cos (ha_r)) * tan (dec_r)
		+ params[3] * cos (lat_r) * sin (ha_r) / cos (dec_r)
		+ params[7] * (sin (lat_r) * tan (dec_r) + cos (lat_r) * cos (ha_r));

	// now handle extra params
	std::list <ExtraParam *>::iterator it;
	for (it = extraParamsHa.begin (); it != extraParamsHa.end (); it++)
		r_tar += (*it)->getValue (az_r, el_r, ha_r, dec_r);

	for (it = extraParamsDec.begin (); it != extraParamsDec.end (); it++)
		d_tar += (*it)->getValue (az_r, el_r, ha_r, dec_r);

	pos->ra = ln_rad_to_deg (r_tar);
	pos->dec = ln_rad_to_deg (d_tar);

	return 0;
}

int GPointModel::reverseVerbose (struct ln_equ_posn *pos, struct ln_hrz_posn *hrz)
{
	logStream (MESSAGE_DEBUG) << "Before: " << pos->ra << " " << pos->dec << sendLog;
	reverse (pos, hrz);
	logStream (MESSAGE_DEBUG) << "After: " << pos->ra << " " << pos->dec << sendLog;

	return 0;
}

void GPointModel::getErrAltAz (struct ln_hrz_posn *hrz, struct ln_equ_posn *equ, struct ln_hrz_posn *err)
{
	long double az_r, el_r, ha_r, dec_r;
	az_r = ln_deg_to_rad (hrz->az);
	el_r = ln_deg_to_rad (hrz->alt);
	ha_r = ln_deg_to_rad (equ->ra);
	dec_r = ln_deg_to_rad (equ->dec);

	long double sin_az = sinl (az_r);
	long double cos_az = cosl (az_r);
	long double cos_el = cosl (el_r);
	long double tan_el = tanl (el_r);

	err->az = - params[0]
		+ params[1] * sin_az  * tan_el
		- params[2] * cos_az * tan_el
		- params[3] * tan_el
		+ params[4] / cos_el;

	err->alt = - params[5]
		+ params[1] * cos_az
		+ params[2] * sin_az
		+ params[6] * cos_el;

	// now handle extra params
	std::list <ExtraParam *>::iterator it;
	for (it = extraParamsAz.begin (); it != extraParamsAz.end (); it++)
		err->az += (*it)->getValue (az_r, el_r, ha_r, dec_r);

	for (it = extraParamsEl.begin (); it != extraParamsEl.end (); it++)
		err->alt += (*it)->getValue (az_r, el_r, ha_r, dec_r);

	err->az = ln_rad_to_deg (err->az);
	err->alt = ln_rad_to_deg (err->alt);

	hrz->az += err->az;
	hrz->alt += err->alt;
}

std::istream & GPointModel::load (std::istream & is)
{
	std::string line ("");

	while (line.length () < 1 || line[0] == '#')
		std::getline (is, line);

	std::istringstream iss (line);

	std::string name;

	iss >> name;

	int pn = 9;

	if (name == "RTS2_ALTAZ")
	{
		altaz = true;
		pn = 7;
	}

	int i = 0;

	// parse standard parameters

	while (!iss.eof () && i < pn)
	{
		try
		{
			readDeg (iss, params[i]);
		}
		catch (rts2core::Error &er)
		{	
			logStream (MESSAGE_ERROR) << "cannot read parameter " << i << sendLog;
			return is;
		}
		i++;
	}

	if (i != pn)
	{
		throw rts2core::Error ("invalid model line " + line);
	}
	// make sure there aren't extra chars..
	while (!iss.eof ())
	{
		int nextch = iss.get ();
		if (iss.eof())
			break;
		if (!isspace (nextch))
			throw rts2core::Error ("extra after end of line  " + line);
	}

	while (!is.eof ())
	{
		line = "";
		// line must have axis name at the begining..
		while (!is.fail () && (line.length () < 1 || line[0] == '#'))
			std::getline (is, line);
		if (is.fail ())
			break;

		ExtraParam *p = new ExtraParam ();
		try
		{
			std::istringstream pis (line);
			std::string axis;
			pis >> axis;
			ci_string ci_axis (axis.c_str ());
			p->parse (pis);
			if (ci_axis ==  "az")
				extraParamsAz.push_back (p);
			else if (ci_axis == "el")
				extraParamsEl.push_back (p);
			else if (ci_axis ==  "ha")
				extraParamsHa.push_back (p);
			else if (ci_axis == "dec")
				extraParamsDec.push_back (p);
			else
			{
				logStream (MESSAGE_ERROR) << "invalid axis name " << axis << sendLog;
				delete p;
				return is;
			}
		}
		catch (rts2core::Error &er)
		{
			logStream (MESSAGE_ERROR) << "parsing line " << line << ": " << er << sendLog;
			delete p;
			return is;
		}
	}

	return is;
}

std::ostream & GPointModel::print (std::ostream & os, char frmt)
{
	int pn = 9;
	if (altaz)
	{
		os << "RTS2_ALTAZ";
		pn = 7;
	}
	else
	{
		os << "RTS2_MODEL";
	}
	for (int i = 0; i < pn; i++)
		os << " " << printDeg (params[i], frmt);

	os << std::endl;

	std::list <ExtraParam *>::iterator it;
	for (it = extraParamsAz.begin (); it != extraParamsAz.end (); it++)
		os << "AZ " << (*it)->toString (frmt) << std::endl;
	for (it = extraParamsEl.begin (); it != extraParamsEl.end (); it++)
		os << "EL " << (*it)->toString (frmt) << std::endl;
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

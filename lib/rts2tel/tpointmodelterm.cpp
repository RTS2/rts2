/* 
 * Telescope model terms.
 * Copyright (C) 2003-2006 Martin Jelinek <mates@iaa.es>
 * Copyright (C) 2006-2007 Petr Kubanek <petr@kubanek.net>
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

#include "tpointmodelterm.h"
#include <math.h>

using namespace rts2telmodel;

TPointModelTerm::TPointModelTerm (const char *in_name, double in_corr, double in_sigma):rts2core::ValueDouble (in_name, std::string(in_name) + " model value", true, RTS2_VALUE_WRITABLE | RTS2_DT_DEGREES)
{
	name = std::string (in_name);
	setValueDouble (in_corr);
	sigma = in_sigma;
}

std::ostream & TPointModelTerm::print (std::ostream & os)
{
	// getValueDouble ()ection is (internally) in degrees!
	os << name << " " << (getValueDouble () * 3600.0) << " " << sigma << std::endl;
	return os;
}

inline std::ostream & operator << (std::ostream & os, TPointModelTerm * term)
{
	return term->print (os);
}

// status OK
void TermME::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	// simple method
	double dh;
	double dd;

	dh = getValueDouble () * sin (ln_deg_to_rad (pos->ra)) * tan (ln_deg_to_rad (pos->dec));
	dd = getValueDouble () * cos (ln_deg_to_rad (pos->ra));

	pos->ra += dh;
	pos->dec += dd;
}

// status OK
void TermMA::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	double d, h;

	h = -1 * getValueDouble () * cos (ln_deg_to_rad (pos->ra)) * tan (ln_deg_to_rad (pos->dec));
	d = getValueDouble () * sin (ln_deg_to_rad (pos->ra));

	pos->ra += h;
	pos->dec += d;
}

// status: OK
void TermIH::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	pos->ra = pos->ra + getValueDouble ();
}

// status: OK
void TermID::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	// Add a zero point to the declination
	pos->dec = pos->dec + getValueDouble ();
	// No change for hour angle
}

// status: OK
void TermCH::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	pos->ra += getValueDouble () / cos (ln_deg_to_rad (pos->dec));
}

// status: OK
void TermNP::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	pos->ra += getValueDouble () * tan (ln_deg_to_rad (pos->dec));
}

// status: OK
void TermFO::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	pos->dec += getValueDouble () * cos (ln_deg_to_rad (pos->ra));
}

// status: ok
void TermPHH::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	pos->ra += getValueDouble () * ln_deg_to_rad (pos->ra);
}

// status: ok
void TermPDD::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	pos->dec += getValueDouble () * ln_deg_to_rad (pos->dec);
}

void TermTF::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	double d, h, f;
	d = ln_deg_to_rad (pos->dec);
	h = ln_deg_to_rad (pos->ra);
	f = ln_deg_to_rad (obs_conditions->getLatitude ());

	pos->ra += getValueDouble () * cos (f) * sin (h) / cos (d);
	pos->dec += getValueDouble () * (cos (f) * cos (h) * sin (d) - sin (f) * cos (d));
}

void TermTX::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	double d, h, f;
	d = ln_deg_to_rad (pos->dec);
	h = ln_deg_to_rad (pos->ra);
	f = ln_deg_to_rad (obs_conditions->getLatitude ());

	pos->ra += getValueDouble () * cos (f) * sin (h) / ((sin (d) * sin (f) + cos (d) * cos (h) * cos (f)) * cos (d));
	pos->dec += getValueDouble () * (cos (f) * cos (h) * sin (d) - sin (f) * cos (d)) / ((sin (d) * sin (f) + cos (d) * cos (h) * cos (f)) * cos (d));
}

void TermHCEC::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	pos->ra += getValueDouble () * cos (ln_deg_to_rad (pos->ra));
}

void TermHCES::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	pos->ra += getValueDouble () * sin (ln_deg_to_rad (pos->ra));
}

void TermDCEC::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	pos->dec += getValueDouble () * cos (ln_deg_to_rad (pos->dec));
}

void TermDCES::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	pos->dec += getValueDouble () * sin (ln_deg_to_rad (pos->dec));
}

void TermDAB::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	double d, h, f;
	d = ln_deg_to_rad (pos->dec);
	h = ln_deg_to_rad (pos->ra);
	f = ln_deg_to_rad (obs_conditions->getLatitude ());

	double sh = sin (h);
	double sf = sin (f);
	double ch = cos (h);

	pos->ra -= getValueDouble () * (sh * sh * sf * sf + ch * ch) * (sf * tan (d) + cos (f) * ch);
}

void TermDAF::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	double d, h, f;
	d = ln_deg_to_rad (pos->dec);
	h = ln_deg_to_rad (pos->ra);
	f = ln_deg_to_rad (obs_conditions->getLatitude ());

	pos->ra -= getValueDouble () * (sin (f) * tan (d) + cos (f) * cos (h));
}

TermHarmonics::TermHarmonics (double in_corr, double in_sigma, const char *in_name):TPointModelTerm (in_name, in_corr, in_sigma)
{
	func[0] = NOT;
	func[1] = NOT;
	const char *end;
	// parse it
	if (strlen (in_name) < 4)
		return;
	resType = in_name[1];
	end = getFunc (in_name + 2, 0);
	if (!end)
		return;
	if (*end)
		getFunc (end, 1);
}

const char * TermHarmonics::getFunc (const char *in_func, int i)
{
	const char *end = in_func + 1;
	int times = 1;
	if (!end)
		return NULL;
	switch (*in_func)
	{
		case 'S':
			func[i] = SIN;
			break;
		case 'C':
			func[i] = COS;
			break;
		default:
			logStream (MESSAGE_ERROR) << "Unknow function " << in_func << sendLog;
			return NULL;
	}
	param[i] = *end;
	end++;
	mul[i] = 0;
	while (isdigit (*end))
	{
		mul[i] = mul[i] * times + (*end - '0');
		times *= 10;
		end++;
	}
	// if there isn't multiplier..
	if (mul[i] == 0)
		mul[i] = 1;
	return end;
}

double TermHarmonics::getValue (struct ln_equ_posn *pos, ObsConditions * obs_conditions, int i)
{
	double val = mul[i];
	//  struct ln_hrz_posn hrz;
	switch (param[i])
	{
		case 'H':
			val *= pos->ra;
			break;
		case 'D':
			val *= pos->dec;
			break;
			/*    case 'A':
				  ln_get_hrz_from_equ_sidereal_time (pos,
				  break; */
		default:
			logStream (MESSAGE_ERROR) << "Unknow parameter " << param[i] << sendLog;
			val = NAN;
	}
	return val;
}

double TermHarmonics::getMember (struct ln_equ_posn *pos, ObsConditions * obs_conditions, int i)
{
	double val;
	switch (func[i])
	{
		case SIN:
			val = ln_deg_to_rad (getValue (pos, obs_conditions, i));
			return sin (val);
			break;
		case COS:
			val = ln_deg_to_rad (getValue (pos, obs_conditions, i));
			return cos (val);
			break;
		case NOT:
		default:
			return (i == 0 ? 0 : 1);
	}
}

void TermHarmonics::apply (struct ln_equ_posn *pos, ObsConditions * obs_conditions)
{
	double resVal = getValueDouble ();
	for (int i = 0; i < 2; i++)
		resVal *= getMember (pos, obs_conditions, i);
	switch (resType)
	{
		case 'H':
			pos->ra += resVal;
			break;
		case 'D':
			pos->dec += resVal;
			break;
		default:
			logStream (MESSAGE_ERROR) << "Cannot process (yet?) resType " << resType << sendLog;
	}
}

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

#include "rts2model.h"
#include "libnova_cpp.h"

#include <fstream>

using namespace rts2telmodel;

RTS2Model::RTS2Model (rts2teld::Telescope * in_telescope, const char *in_modelFile):TelModel (in_telescope, in_modelFile)
{
}

RTS2Model::~RTS2Model (void)
{
}

int RTS2Model::load ()
{
	std::ifstream is (modelFile);
	load (is);
	if (is.fail ())
		return -1;
	return 0;
}

int RTS2Model::apply (struct ln_equ_posn *pos)
{
        double d_tar, r_tar;
        pos.ra = ln_radians(pos.ra);
        pos.dec = ln_radians(pos.dec);

        double lat_r = cond->getLatitudeRadians ();

        d_tar = pos.dec + params[0] - params[1] * cos (pos.ra) - params[2] * sin (pos.ra) - params[3] * (sin (lat_r) * cos(pos.dec) + cos (lat_r) * sin (pos.dec) * cos (pos.ra);
        r_tar = pos.ra + params[4] + params[5] / cos (pos.dec) - params[6] * tan (pos.dec) + (-params[1] * sin (pos.ra) + params[2] * cos (pos.dec)) * tan (pos.dec) + params[3] * cos (lat_r) * sin (pos.ra) / cos (pos.dec) + params[7] * (sin (lat_r) * tan (pos.dec) + cos (pos.dec) * cos (pos.ra)) + params[8] * pos.ra;

        pos.ra = d_tar;
        pos.dec = d_dec;

	return 0;
}

int RTS2Model::applyVerbose (struct ln_equ_posn *pos)
{
        logStream (MESSAGE_DEBUG) << "Before: " << pos.ra << " " << pos.dec << sendLog;
        apply (pos);
        logStream (MESSAGE_DEBUG) << "After: " << pos.ra << " " << pos.dec << sendLog;
	return 0;
}

int RTS2Model::reverse (struct ln_equ_posn *pos)
{
	return 0;
}

int TPointModel::reverseVerbose (struct ln_equ_posn *pos)
{
	return 0;
}

int TPointModel::reverse (struct ln_equ_posn *pos, double sid)
{
	for (std::vector < TPointModelTerm * >::iterator iter = begin (); iter != end (); iter++)
	{
		(*iter)->reverse (pos, cond);
	}
	return 0;
}

std::istream & TPointModel::load (std::istream & is)
{
	std::string name;

	int lineNo = 1;

	double corr;
	double sigma;
	TPointModelTerm *term;
	// first line
	is.getline (caption, 80);
	if (is.fail ())
	{
		logStream (MESSAGE_ERROR) << "Cannot read first line of the model file" << sendLog;
		return is;
	}
	lineNo++;
	// second line - method, number, refA, refB
	is >> method >> num >> rms >> refA >> refB;
	is.ignore (2000, is.widen ('\n'));
	if (is.fail ())
	{
		logStream (MESSAGE_ERROR) << "Cannot read seconds line of the model file" << sendLog;
		return is;
	}
	lineNo++;
	while (!is.eof ())
	{
		is >> name;
		if (name == "END")
		{
			return is;
		}
		// get corr parameter
		is >> corr;
		if (is.fail ())
		{
			logStream (MESSAGE_ERROR) << "Cannot read correction from line " << lineNo << sendLog;
			return is;
		}
		if (name[0] != '=')
		{
			// only read sigma when needed
			is >> sigma;
			if (is.fail ())
			{
				logStream (MESSAGE_ERROR) << "Cannot read sigma from line " << lineNo << sendLog;
				return is;
			}
		}
		// correction is in degrees to speed up a calculation
		corr /= 3600.0;
		// get rid of fixed terms
		if (name[0] == '=')
			name = name.substr (1);
		if (name == "ME")
		{
			term = new TermME (corr, sigma);
		}
		else if (name == "MA")
		{
			term = new TermMA (corr, sigma);
		}
		else if (name == "IH")
		{
			term = new TermIH (corr, sigma);
		}
		else if (name == "ID")
		{
			term = new TermID (corr, sigma);
		}
		else if (name == "CH")
		{
			term = new TermCH (corr, sigma);
		}
		else if (name == "NP")
		{
			term = new TermNP (corr, sigma);
		}
		else if (name == "PHH")
		{
			term = new TermPHH (corr, sigma);
		}
		else if (name == "PDD")
		{
			term = new TermPDD (corr, sigma);
		}
		else if (name == "A1H")
		{
			term = new TermA1H (corr, sigma);
		}
		else if (name == "A1D")
		{
			term = new TermA1D (corr, sigma);
		}
		else if (name == "TF")
		{
			term = new TermTF (corr, sigma);
		}
		else if (name == "TX")
		{
			term = new TermTX (corr, sigma);
		}
		else if (name == "HCEC")
		{
			term = new TermHCEC (corr, sigma);
		}
		else if (name == "HCES")
		{
			term = new TermHCES (corr, sigma);
		}
		else if (name == "DCEC")
		{
			term = new TermDCEC (corr, sigma);
		}
		else if (name == "DCES")
		{
			term = new TermDCES (corr, sigma);
		}
		else if (name == "DAB")
		{
			term = new TermDAB (corr, sigma);
		}
		else if (name == "DAF")
		{
			term = new TermDAF (corr, sigma);
		}
		// generic harmonics term
		else if (name[0] == 'H')
		{
			term = new TermHarmonics (corr, sigma, name.c_str ());
		}
		else
		{
			logStream (MESSAGE_ERROR) << "Unknow model term '" << name << "' at model line " << lineNo << sendLog;
			return is;
		}
		push_back (term);
		lineNo++;
	}
	return is;
}

std::ostream & TPointModel::print (std::ostream & os)
{
	for (std::vector < TPointModelTerm * >::iterator iter = begin (); iter != end (); iter++)
	{
		(*iter)->print (os);
	}
	return os;
}

inline std::istream & operator >> (std::istream & is, TPointModel * model)
{
	return model->load (is);
}

inline std::ostream & operator << (std::ostream & os, TPointModel * model)
{
	return model->print (os);
}

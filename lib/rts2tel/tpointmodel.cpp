/* 
 * Telescope model reader.
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

#include "tpointmodel.h"
#include "libnova_cpp.h"

#include <fstream>

using namespace rts2telmodel;

TPointModel::TPointModel (double in_latitude):TelModel (in_latitude)
{
}

TPointModel::~TPointModel (void)
{
}

int TPointModel::load (const char *modelFile)
{
	std::ifstream is (modelFile);
	load (is);
	if (is.fail ())
		return -1;
	return 0;
}

int TPointModel::apply (struct ln_equ_posn *pos)
{
	for (std::vector < TPointModelTerm * >::iterator iter = begin (); iter != end (); iter++)
	{
		(*iter)->apply (pos, tel_latitude);
	}
	return 0;
}

int TPointModel::applyVerbose (struct ln_equ_posn *pos)
{
	for (std::vector < TPointModelTerm * >::iterator iter = begin (); iter != end (); iter++)
	{
		struct ln_equ_posn old_pos = *pos;
		logStream (MESSAGE_DEBUG) // << (*iter)
			<< "Before: " << pos->ra << " " << pos->dec << sendLog;
		(*iter)->apply (pos, tel_latitude);
		logStream (MESSAGE_DEBUG) << "After: " << pos->ra << " " << pos->dec 
			<< "(" << LibnovaDegDist (pos->ra - old_pos.ra) 
			<< " " << LibnovaDegDist (pos->dec - old_pos.dec)
			<< ")" << sendLog;
	}
	return 0;
}

int TPointModel::reverse (struct ln_equ_posn *pos, __attribute__ ((unused)) struct ln_hrz_posn *hrz)
{
	struct ln_equ_posn pos2;

	for (std::vector < TPointModelTerm * >::iterator iter = begin (); iter != end (); iter++)
	{
		pos2.ra = pos->ra;
		pos2.dec = pos->dec;

		(*iter)->apply (pos, getLatitudeRadians ());

		pos->ra = 2. * pos2.ra - pos->ra;
		pos->dec = 2. * pos2.dec - pos->dec;
	}
	return 0;
}

int TPointModel::reverseVerbose (struct ln_equ_posn *pos, __attribute__ ((unused)) struct ln_hrz_posn *hrz)
{
	struct ln_equ_posn pos2;

	for (std::vector < TPointModelTerm * >::iterator iter = begin (); iter != end (); iter++)
	{
		struct ln_equ_posn old_pos = *pos;

		pos2.ra = pos->ra;
		pos2.dec = pos->dec;

		logStream (MESSAGE_DEBUG) << //(*iter) << 
			"Before: " << pos->ra << " " << pos->dec << sendLog;

		(*iter)->apply (pos, getLatitudeRadians ());

		logStream (MESSAGE_DEBUG) << "After1: " << pos->ra << " " << pos->dec
			<< "(" << LibnovaDegDist (pos->ra - old_pos.ra) << " "
			<< (pos->dec - old_pos.dec) << ")" << sendLog;

		pos->ra = 2. * pos2.ra - pos->ra;
		pos->dec = 2. * pos2.dec - pos->dec;

		logStream (MESSAGE_DEBUG) << "After2: " << pos->ra << " " << pos->dec
			<< "(" << (pos->ra - old_pos.ra) << " "
			<< (pos->dec - old_pos.dec) << ")" << sendLog;
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

std::ostream & TPointModel::print (std::ostream & os, __attribute__ ((unused)) char frmt)
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

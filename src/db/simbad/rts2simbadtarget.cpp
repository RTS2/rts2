/*
 * Target from SIMBAD database.
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

#include "SesameSoapBinding.nsmap"
#include "soapSesameSoapBindingProxy.h"

#include <sstream>

#include "rts2simbadtarget.h"

using namespace std;

Rts2SimbadTarget::Rts2SimbadTarget (const char *in_name):
ConstTarget ()
{
	setTargetName (in_name);
	simbadBMag = nan ("f");
	propMotions.ra = nan ("f");
	propMotions.dec = nan ("f");

	simbadOut = NULL;
}


Rts2SimbadTarget::~Rts2SimbadTarget (void)
{
	delete simbadOut;
}


int
Rts2SimbadTarget::load ()
{
	#define LINEBUF  200
	char buf[LINEBUF];

	SesameSoapBinding *bind = new SesameSoapBinding();

	#ifdef WITH_FAST
	std::string sesame_r;
	#else
	sesame__sesameResponse sesame_r;
	#endif

	bind->sesame__sesame (std::string (getTargetName ()), std::string ("ui"), sesame_r);

	if (bind->soap->error != SOAP_OK)
	{
		cerr << "Cannot get coordinates for " << getTargetName () << endl;
		soap_print_fault (bind->soap, stderr);
		delete bind;
		return -1;
	}
	istringstream *iss = new istringstream ();

	simbadOut = new ostringstream ();

	#ifdef WITH_FAST
	*simbadOut << sesame_r;
	iss->str (sesame_r);
	#else
	*simbadOut << sesame_r._return_;
	iss->str (sesame_r._return_);
	#endif

	string str_type;
	while (*iss >> str_type)
	{
		if (str_type == "%J")
		{
			double ra, dec;
			*iss >> ra >> dec;
			iss->getline (buf, LINEBUF);
			setPosition (ra, dec);
		}
		else if (str_type == "#=Simbad:")
		{
			int nobj;
			*iss >> nobj;
			if (nobj != 1)
			{
				cerr << "More then 1 object found!" << endl;
				delete bind;
				return -1;
			}
		}
		else if (str_type == "%C")
		{
			iss->getline (buf, LINEBUF);
			simbadType = string (buf);
		}
		else if (str_type == "#B")
		{
			*iss >> simbadBMag;
			simbadBMag /= 100;
		}
		else if (str_type == "%I")
		{
			iss->getline (buf, LINEBUF);
			aliases.push_back (string (buf));
		}
		else if (str_type.substr (0, 2) == "#!")
		{
			cerr << "Not found" << endl;
			delete bind;
			return -1;
		}
		else if (str_type == "%J.E")
		{
			iss->getline (buf, LINEBUF);
			references = string (buf);
		}
		else if (str_type == "%I.0")
		{
			// eat whitespace
			*iss >> std::ws;

			iss->getline (buf, LINEBUF);
			setTargetName (buf);
		}
		else if (str_type == "%@")
		{
			// most probably simbad version, ignore
			iss->getline (buf, LINEBUF);
		}
		else if (str_type == "%P")
		{
			*iss >> propMotions.ra >> propMotions.dec;
			// it's in masec/year
			propMotions.ra /= 360000.0;
			propMotions.dec /= 360000.0;
			iss->getline (buf, LINEBUF);
		}
		else if (str_type.c_str ()[0] == '#')
		{
			// ignore comments
			iss->getline (buf, LINEBUF);
		}
		else
		{
			cerr << "Unknow " << str_type << endl;
			iss->getline (buf, LINEBUF);
		}
	}
	delete bind;
	#undef LINEBUF
	return 0;
}


void
Rts2SimbadTarget::printExtra (Rts2InfoValStream & _ivs)
{
	ConstTarget::printExtra (_ivs, ln_get_julian_from_sys ());

	if (_ivs.getStream ())
	{
		std::ostream* _os = _ivs.getStream ();
		*_os << "REFERENCED " << references << std::endl;
		int old_prec = _os->precision (2);
		*_os << "PROPER MOTION RA " <<
			(propMotions.ra * 360000.0)
			<< " DEC " << (propMotions.dec * 360000.0) << " (mas/y)";
		*_os << "TYPE " << simbadType << std::endl;
		*_os << "B MAG " << simbadBMag << std::endl;
		_os->precision (old_prec);

		for (std::list < std::string >::iterator alias = aliases.begin ();
			alias != aliases.end (); alias++)
		{
			*_os << "ALIAS " << (*alias) << std::endl;
		}
	}
}

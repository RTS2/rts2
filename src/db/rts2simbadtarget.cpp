#include "soapSesameSoapBindingProxy.h"
#include "SesameSoapBinding.nsmap"

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
}

int
Rts2SimbadTarget::load ()
{
#define LINEBUF  200
  char buf[LINEBUF];

  SesameSoapBinding bind = SesameSoapBinding ();
  ns1__sesameResponse r;
  bind.ns1__sesame (string (getTargetName ()), string ("ui"), r);
  if (bind.soap->error != SOAP_OK)
    {
      cerr << "Cannot get coordinates for " << getTargetName () << endl;
      soap_print_fault (bind.soap, stderr);
      return -1;
    }
  istringstream *iss = new istringstream ();
  cout << "Simbad response for '" << getTargetName () << "': " << endl
    << "***************************" << endl
    << r._return_ << endl << "***************************" << endl;
  iss->str (r._return_);
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
	  return -1;
	}
      else if (str_type == "%J.E")
	{
	  iss->getline (buf, LINEBUF);
	  references = string (buf);
	}
      else if (str_type == "%I.0")
	{
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
	  cout << "Unknow " << str_type << endl;
	  iss->getline (buf, LINEBUF);
	}
    }

#undef LINEBUF
  return 0;
}

void
Rts2SimbadTarget::printExtra (std::ostream & _os)
{
  ConstTarget::printExtra (_os);

  _os << "REFERENCED " << references << std::endl;
  int old_prec = _os.precision (2);
  _os << "PROPER MOTION RA " <<
    (propMotions.ra * 360000.0)
    << " DEC " << (propMotions.dec * 360000.0) << " (mas/y)";
  _os << "TYPE " << simbadType << std::endl;
  _os << "B MAG " << simbadBMag << std::endl;
  _os.precision (old_prec);

  for (std::list < std::string >::iterator alias = aliases.begin ();
       alias != aliases.end (); alias++)
    {
      _os << "ALIAS " << (*alias) << std::endl;
    }
}

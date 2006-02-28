#include "telmodel.h"

#include <fstream>

Rts2TelModel::Rts2TelModel (Rts2DevTelescope * in_telescope,
			    const char *in_modelFile)
{
  cond = new Rts2ObsConditions (in_telescope);
  modelFile = in_modelFile;
}

Rts2TelModel::~Rts2TelModel (void)
{
  delete cond;
}

int
Rts2TelModel::load ()
{
  std::ifstream is (modelFile);
  is >> this;
  if (is.fail ())
    return -1;
  return 0;
}

int
Rts2TelModel::apply (struct ln_equ_posn *pos)
{
  return apply (pos, ln_get_mean_sidereal_time (ln_get_julian_from_sys ()));
}

int
Rts2TelModel::apply (struct ln_equ_posn *pos, double lst)
{
  for (std::vector < Rts2ModelTerm * >::iterator iter = terms.begin ();
       iter != terms.end (); iter++)
    {
      (*iter)->apply (pos, cond);
    }
  return 0;
}

int
Rts2TelModel::reverse (struct ln_equ_posn *pos)
{
  return reverse (pos, ln_get_mean_sidereal_time (ln_get_julian_from_sys ()));
}

int
Rts2TelModel::reverse (struct ln_equ_posn *pos, double sid)
{
  for (std::vector < Rts2ModelTerm * >::iterator iter = terms.begin ();
       iter != terms.end (); iter++)
    {
      (*iter)->reverse (pos, cond);
    }
  return 0;
}

std::istream & operator >> (std::istream & is, Rts2TelModel * model)
{
  std::string name;
  char eq;
  double corr;
  Rts2ModelTerm *term;
  // load line..
  while (!is.eof ())
    {
      is >> name;
      // ignore comment lines
      if (name.at (0) == '#')
	{
	  // read till end of line
	  is.ignore (2000);
	  continue;
	}
      // get corr parameter
      is >> eq >> corr;
      if (eq != '=' || is.fail ())
	{
	  return is;
	}

      if (name == "ME")
	{
	  term = new Rts2TermME (corr);
	}
      else if (name == "MA")
	{
	  term = new Rts2TermMA (corr);
	}
      else if (name == "IH")
	{
	  term = new Rts2TermIH (corr);
	}
      else if (name == "ID")
	{
	  term = new Rts2TermID (corr);
	}
      else if (name == "CH")
	{
	  term = new Rts2TermCH (corr);
	}
      else if (name == "NP")
	{
	  term = new Rts2TermNP (corr);
	}
      else if (name == "PHH")
	{
	  term = new Rts2TermPHH (corr);
	}
      else if (name == "PDD")
	{
	  term = new Rts2TermPDD (corr);
	}
      else if (name == "A1H")
	{
	  term = new Rts2TermA1H (corr);
	}
      else if (name == "A1D")
	{
	  term = new Rts2TermA1D (corr);
	}
      else if (name == "TF")
	{
	  term = new Rts2TermTF (corr);
	}
      else if (name == "TX")
	{
	  term = new Rts2TermTX (corr);
	}
      else
	{
	  return is;
	}
      model->terms.push_back (term);
    }
  return is;
}

std::ostream & operator << (std::ostream & os, Rts2TelModel * model)
{
  for (std::vector < Rts2ModelTerm * >::iterator iter = model->terms.begin ();
       iter != model->terms.end (); iter++)
    {
      os << (*iter);
    }
  return os;
}

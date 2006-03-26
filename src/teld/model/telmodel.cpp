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
//      std::cout << (*iter) << "Before: " << pos->ra << " " << pos->dec << std::endl;
      (*iter)->apply (pos, cond);
//      std::cout << "After: " << pos->ra << " " << pos->dec << std::endl;
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

  double corr;
  double sigma;
  Rts2ModelTerm *term;
  // first line
  is.getline (model->caption, 80);
  // second line - method, number, refA, refB
  is >> model->method >> model->num >> model->rms >> model->refA >> model->
    refB;
  is.ignore (2000, is.widen ('\n'));
  if (is.fail ())
    return is;
  while (!is.eof ())
    {
      is >> name;
      if (name == "END")
	{
	  return is;
	}
      // get corr parameter
      is >> corr >> sigma;
      if (is.fail ())
	{
	  return is;
	}
      // correction is in degrees to speed up a calculation
      corr /= 3600.0;
      if (name == "ME")
	{
	  term = new Rts2TermME (corr, sigma);
	}
      else if (name == "MA")
	{
	  term = new Rts2TermMA (corr, sigma);
	}
      else if (name == "IH")
	{
	  term = new Rts2TermIH (corr, sigma);
	}
      else if (name == "ID")
	{
	  term = new Rts2TermID (corr, sigma);
	}
      else if (name == "CH")
	{
	  term = new Rts2TermCH (corr, sigma);
	}
      else if (name == "NP")
	{
	  term = new Rts2TermNP (corr, sigma);
	}
      else if (name == "PHH")
	{
	  term = new Rts2TermPHH (corr, sigma);
	}
      else if (name == "PDD")
	{
	  term = new Rts2TermPDD (corr, sigma);
	}
      else if (name == "A1H")
	{
	  term = new Rts2TermA1H (corr, sigma);
	}
      else if (name == "A1D")
	{
	  term = new Rts2TermA1D (corr, sigma);
	}
      else if (name == "TF")
	{
	  term = new Rts2TermTF (corr, sigma);
	}
      else if (name == "TX")
	{
	  term = new Rts2TermTX (corr, sigma);
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

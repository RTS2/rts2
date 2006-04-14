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
  for (std::vector < Rts2ModelTerm * >::iterator iter = terms.begin ();
       iter != terms.end (); iter++)
    {
      (*iter)->apply (pos, cond);
    }
  return 0;
}

int
Rts2TelModel::applyVerbose (struct ln_equ_posn *pos)
{
  for (std::vector < Rts2ModelTerm * >::iterator iter = terms.begin ();
       iter != terms.end (); iter++)
    {
      std::cout << (*iter) << "Before: " << pos->ra << " " << pos->
	dec << std::endl;
      (*iter)->apply (pos, cond);
      std::cout << "After: " << pos->ra << " " << pos->
	dec << std::endl << std::endl;
    }
  return 0;
}


int
Rts2TelModel::reverse (struct ln_equ_posn *pos)
{
  struct ln_equ_posn pos2;

  for (std::vector < Rts2ModelTerm * >::iterator iter = terms.begin ();
       iter != terms.end (); iter++)
    {
      pos2.ra = pos->ra;
      pos2.dec = pos->dec;

      (*iter)->apply (pos, cond);

      pos->ra = ln_range_degrees (2 * pos2.ra - pos->ra);
      pos->dec = 2 * pos2.dec - pos->dec;
    }
  return 0;
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
      else if (name == "HCEC")
	{
	  term = new Rts2TermHCEC (corr, sigma);
	}
      else if (name == "HCES")
	{
	  term = new Rts2TermHCES (corr, sigma);
	}
      else if (name == "DCEC")
	{
	  term = new Rts2TermDCEC (corr, sigma);
	}
      else if (name == "DCES")
	{
	  term = new Rts2TermDCES (corr, sigma);
	}
      // generic harmonics term
      else if (name[0] == 'H')
	{
	  term = new Rts2TermHarmonics (corr, sigma, name.c_str ());
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

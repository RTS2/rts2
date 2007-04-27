#include "rts2sehex.h"

Rts2Path::~Rts2Path (void)
{
  for (top = begin (); top != end (); top++)
    {
      delete *top;
    }
  clear ();
}

void
Rts2Path::addRaDec (double in_ra, double in_dec)
{
  struct ln_equ_posn *newPos = new struct ln_equ_posn;
  newPos->ra = in_ra;
  newPos->dec = in_dec;
  push_back (newPos);
}

double
Rts2SEHex::getRa ()
{
  return path.getRa () * ra_size;
}

double
Rts2SEHex::getDec ()
{
  return path.getDec () * dec_size;
}

bool Rts2SEHex::endLoop ()
{
  return !path.haveNext ();
}

bool
Rts2SEHex::getNextLoop ()
{
  if (path.getNext ())
    {
      changeEl->setChangeRaDec (getRa (), getDec ());
      return false;
    }
  return true;
}

void
Rts2SEHex::constructPath ()
{
  // construct path
  path.addRaDec (1, 1);
  path.addRaDec (1, 0);
  path.addRaDec (1, -1);
  path.addRaDec (0, -1);
  path.addRaDec (-1, -1);
  path.addRaDec (-1, 0);
  path.addRaDec (-1, 1);
  path.addRaDec (0, 1);
  path.endPath ();
}


Rts2SEHex::Rts2SEHex (Rts2Script * in_script, double in_ra_size,
		      double in_dec_size):
Rts2ScriptElementBlock (in_script)
{
  ra_size = in_ra_size;
  dec_size = in_dec_size;
  changeEl = NULL;
}

Rts2SEHex::~Rts2SEHex (void)
{
  changeEl = NULL;
}

void
Rts2SEHex::beforeExecuting ()
{
  if (!path.haveNext ())
    constructPath ();

  if (path.haveNext ())
    {
      changeEl = new Rts2ScriptElementChange (script, getRa (), getDec ());
      addElement (changeEl);
    }
}

void
Rts2SEFF::constructPath ()
{
  path.addRaDec (-2, -2);
  path.addRaDec (1, 0);
  path.addRaDec (1, 0);
  path.addRaDec (1, 0);
  path.addRaDec (1, 0);
  path.addRaDec (0, 1);
  path.addRaDec (-1, 0);
  path.addRaDec (-1, 0);
  path.addRaDec (-1, 0);
  path.addRaDec (-1, 0);
  path.addRaDec (0, 1);
  path.addRaDec (1, 0);
  path.addRaDec (1, 0);
  path.addRaDec (1, 0);
  path.addRaDec (1, 0);
  path.addRaDec (0, 1);
  path.addRaDec (-1, 0);
  path.addRaDec (-1, 0);
  path.addRaDec (-1, 0);
  path.addRaDec (-1, 0);
  path.addRaDec (0, 1);
  path.addRaDec (1, 0);
  path.addRaDec (1, 0);
  path.addRaDec (1, 0);
  path.addRaDec (1, 0);
  path.addRaDec (-2, -2);
  path.endPath ();
}

Rts2SEFF::Rts2SEFF (Rts2Script * in_script, double in_ra_size,
		    double in_dec_size):
Rts2SEHex (in_script, in_ra_size, in_dec_size)
{
}

Rts2SEFF::~Rts2SEFF (void)
{

}

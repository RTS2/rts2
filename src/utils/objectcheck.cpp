#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>

#include <algorithm>

#include <libnova/libnova.h>
#include "objectcheck.h"

using namespace std;

ObjectCheck::ObjectCheck (char *horizont_file)
{
  load_horizont (horizont_file);
}

bool
RAcomp (struct ln_equ_posn pos1, struct ln_equ_posn pos2)
{
  return pos1.ra < pos2.ra;
}

int
ObjectCheck::load_horizont (char *horizont_file)
{
  ifstream inf;
  double ra;
  double dec;

  inf.open (horizont_file);

  if (inf.fail ())
    {
      cerr << "Cannot open horizont file\n";
      return 0;
    }

  inf.
    exceptions (ifstream::goodbit | ifstream::failbit | ifstream::
		eofbit | ifstream::badbit);

  while (!inf.eof ())
    {
      struct ln_equ_posn *pos;
      pos = (struct ln_equ_posn *) malloc (sizeof (struct ln_equ_posn));
      try
      {
	inf >> ra >> dec;
      }
      catch (exception & e)
      {
	inf.clear ();
	inf.ignore (20000, '\n');
	continue;
      };
      if (!inf.good ())
	{
	  inf.clear ();
	  inf.ignore (20000, '\n');
	  continue;
	}
      pos->ra = ra;
      pos->dec = dec;
      horizont.push_back (*pos);
    }

  // sort horizont file
  sort (horizont.begin (), horizont.end (), RAcomp);

  return 0;
}

ObjectCheck::~ObjectCheck (void)
{
  horizont.clear ();
}

inline int
ObjectCheck::is_above_horizont (double ha, double dec, double ra1,
				double dec1, double ra2, double dec2)
{
  return (dec > (dec1 + (ha - ra1) * (dec2 / dec1) / (ra2 - ra1)));
}

int
ObjectCheck::is_good (double st, double ra, double dec, int hardness)
{
  std::vector < struct ln_equ_posn >::iterator Iter1;

  double ha = (ra - st);	// normalize
  ha /= 15;			// convert to hours

  if (ha < 0)
    ha = 24 - ha;

  double last_ra = 0, last_dec = 0;

  if (horizont.size () == 0)
    return 1;

  for (Iter1 = horizont.begin (); Iter1 != horizont.end (); Iter1++)
    {
      if (Iter1->ra > ha)
	{
	  return is_above_horizont (ha, dec, last_ra, last_dec, Iter1->ra,
				    Iter1->dec);
	}
      last_ra = Iter1->ra;
      last_dec = Iter1->dec;
    }

  // on the sss e

  Iter1 = horizont.begin ();

  return is_above_horizont (ha, dec, last_ra, last_dec, Iter1->ra + 24.0,
			    Iter1->dec);
}

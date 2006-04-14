/**
 * Contains getHamOffset functions.
 *
 * As this function is special for FRAM/HAM combination only, I keep it in separate file.
 *
 * @author Petr Kubanek <pkubanek@asu.cas.cz>
 */

#include "rts2image.h"

#include <math.h>

static int
sdFluxCompare (const void *sr1, const void *sr2)
{
  return (((struct stardata *) sr1)->F >
	  ((struct stardata *) sr2)->F) ? -1 : 1;
}

int
Rts2Image::getHam (double &x, double &y)
{
  if (sexResultNum == 0)
    {
      return -1;
    }
  qsort (sexResults, sexResultNum, sizeof (struct stardata), sdFluxCompare);
  // we think that first source is HAM
  syslog (LOG_DEBUG, "Rts2Image::getHam flux0: %f", sexResults[0].F);
  if (sexResults[0].F > 100000)
    {
      float dist;
      if (sexResultNum > 1)
	{
	  // let's see if the second is close enough..airplane light
	  dist =
	    sqrt (((sexResults[0].X - sexResults[1].X) * (sexResults[0].X -
							  sexResults[1].X)) +
		  ((sexResults[0].Y - sexResults[1].Y) * (sexResults[0].Y -
							  sexResults[1].Y)));
	}
      else
	{
	  dist = 101;
	}
      // we are quite sure we find it..
      if (dist < 100 || sexResults[0].F > 150000)
	{
	  x = sexResults[0].X;
	  y = sexResults[0].Y;
	  return 0;
	}
      else
	syslog (LOG_DEBUG, "Rts2Image::getHam big sidt: %f", dist);
    }
  // HAM not found..
  return -1;
}

int
Rts2Image::getBrightestOffset (double &x, double &y, float &flux)
{
  if (sexResultNum == 0)
    {
      return -1;
    }
  qsort (sexResults, sexResultNum, sizeof (struct stardata), sdFluxCompare);
  // returns coordinates of brightest source
  syslog (LOG_DEBUG, "Rts2Image::getBrightestOffset flux0: %f flags: %i",
	  sexResults[0].F, sexResults[0].flags);
  x = sexResults[0].X;
  y = sexResults[0].Y;
  flux = sexResults[0].F;
  return 0;
}

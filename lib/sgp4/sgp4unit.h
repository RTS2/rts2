#ifndef _sgp4unit_
#define _sgp4unit_
/*     ----------------------------------------------------------------
*
*                                 sgp4unit.h
*
*    this file contains the sgp4 procedures for analytical propagation
*    of a satellite. the code was originally released in the 1980 and 1986
*    spacetrack papers. a detailed discussion of the theory and history
*    may be found in the 2006 aiaa paper by vallado, crawford, hujsak,
*    and kelso.
*
*                            companion code for
*               fundamentals of astrodynamics and applications
*                                    2007
*                              by david vallado
*
*       (w) 719-573-2600, email dvallado@agi.com
*
*    current :
*              30 Dec 11  david vallado
*                           consolidate updated code version
*              30 Aug 10  david vallado
*                           delete unused variables in initl
*                           replace pow inetger 2, 3 with multiplies for speed
*    changes :
*               3 Nov 08  david vallado
*                           put returns in for error codes
*              29 sep 08  david vallado
*                           fix atime for faster operation in dspace
*                           add operationmode for afspc (a) or improved (i)
*                           performance mode
*              20 apr 07  david vallado
*                           misc fixes for constants
*              11 aug 06  david vallado
*                           chg lyddane choice back to strn3, constants, misc doc
*              15 dec 05  david vallado
*                           misc fixes
*              26 jul 05  david vallado
*                           fixes for paper
*                           note that each fix is preceded by a
*                           comment with "sgp4fix" and an explanation of
*                           what was changed
*              10 aug 04  david vallado
*                           2nd printing baseline working
*              14 may 01  david vallado
*                           2nd edition baseline
*                     80  norad
*                           original baseline
*       ----------------------------------------------------------------      */

#include <math.h>
#include <stdio.h>
#include <libnova/libnova.h>

#include "sgp4.h"
#define SGP4Version  "SGP4 Version 2011-12-30"

#define pi 3.14159265358979323846

// -------------------------- structure declarations ----------------------------
typedef enum
{
  wgs72old,
  wgs72,
  wgs84
} gravconsttype;

// --------------------------- function declarations ----------------------------
bool sgp4init
     (
       gravconsttype whichconst,  char opsmode,  const int satn,     const double epoch,
       const double xbstar,  const double xecco, const double xargpo,
       const double xinclo,  const double xmo,   const double xno,
       const double xnodeo,  rts2sgp4::elsetrec& satrec
     );

bool sgp4
     (
       gravconsttype whichconst, rts2sgp4::elsetrec& satrec,  double tsince,
       struct ln_rect_posn *r, struct ln_rect_posn *v
     );

double  gstime
        (
          double jdut1
        );

void getgravconst
     (
      gravconsttype whichconst,
      double& tumin,
      double& mu,
      double& radiusearthkm,
      double& xke,
      double& j2,
      double& j3,
      double& j4,
      double& j3oj2
     );

#endif


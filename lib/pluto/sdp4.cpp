#include <math.h>
#include "norad.h"
#include "norad_in.h"

#define c1           params[2]
#define c4           params[3]
#define xnodcf       params[4]
#define t2cof        params[5]
#define deep_arg     ((deep_arg_t *)( params + 10))

void DLL_FUNC SDP4_init( double *params, const tle_t *tle)
{
   init_t init;

   sxpx_common_init( params, tle, &init, deep_arg);
   deep_arg->sing = sin(tle->omegao);
   deep_arg->cosg = cos(tle->omegao);

   /* initialize Deep() */
   Deep_dpinit( tle, deep_arg);
#ifdef RETAIN_PERTURBATION_VALUES_AT_EPOCH
   /* initialize lunisolar perturbations: */
   deep_arg->t = 0.;                            /* added 30 Dec 2003 */
   deep_arg->solar_lunar_init_flag = 1;
   Deep_dpper( tle, deep_arg);
   deep_arg->solar_lunar_init_flag = 0;
#endif
} /*End of SDP4() initialization */

int DLL_FUNC SDP4( const double tsince, const tle_t *tle, const double *params,
                                         double *pos, double *vel)
{
  double
      a, tempa, tsince_squared,
      xl, xnoddf;

  /* Update for secular gravity and atmospheric drag */
  deep_arg->omgadf = tle->omegao + deep_arg->omgdot * tsince;
  xnoddf = tle->xnodeo + deep_arg->xnodot * tsince;
  tsince_squared = tsince*tsince;
  deep_arg->xnode = xnoddf + xnodcf * tsince_squared;
  deep_arg->xn = deep_arg->xnodp;

  /* Update for deep-space secular effects */
  deep_arg->xll = tle->xmo + deep_arg->xmdot * tsince;
  deep_arg->t = tsince;

  Deep_dpsec( tle, deep_arg);

  tempa = 1-c1*tsince;
  if( deep_arg->xn < 0.)
     return( SXPX_ERR_NEGATIVE_XN);
  a = pow(xke/deep_arg->xn,two_thirds)*tempa*tempa;
  deep_arg->em -= tle->bstar*c4*tsince;

  /* Update for deep-space periodic effects */
  deep_arg->xll += deep_arg->xnodp * t2cof * tsince_squared;

  Deep_dpper( tle, deep_arg);

            /* Keeping xinc positive is not really necessary,  unless        */
            /* you're displaying elements and dislike negative inclinations. */
#ifdef KEEP_INCLINATION_POSITIVE
  if (deep_arg->xinc < 0.)       /* Begin April 1983 errata correction: */
     {
     deep_arg->xinc = -deep_arg->xinc;
     deep_arg->sinio = -deep_arg->sinio;
     deep_arg->xnode += pi;
     deep_arg->omgadf -= pi;
     }                          /* End April 1983 errata correction. */
#endif

  xl = deep_arg->xll + deep_arg->omgadf + deep_arg->xnode;
               /* Dundee change:  Reset cosio,  sinio for new xinc: */
  deep_arg->cosio = cos( deep_arg->xinc);
  deep_arg->sinio = sin( deep_arg->xinc);

  return( sxpx_posn_vel( deep_arg->xnode, a, deep_arg->em, params, deep_arg->cosio,
                deep_arg->sinio, deep_arg->xinc, deep_arg->omgadf,
                xl, pos, vel));
} /* SDP4 */

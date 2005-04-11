/* Processed by ecpg (3.1.1) */
/* These include files are added by the preprocessor */
#include <ecpgtype.h>
#include <ecpglib.h>
#include <ecpgerrno.h>
#include <sqlca.h>
#line 1 "sub_targets.ec"
/* End of automatic include section */
#include <libnova/libnova.h>
#include "target.h"


#line 1 "/usr/include/postgresql/sqlca.h"
#ifndef POSTGRES_SQLCA_H
#define POSTGRES_SQLCA_H

#ifndef DLLIMPORT
#if defined(__CYGWIN__) || defined(WIN32)
#define DLLIMPORT __declspec (dllimport)
#else
#define DLLIMPORT
#endif /* __CYGWIN__ */
#endif /* DLLIMPORT */

#define SQLERRMC_LEN	70

#ifdef __cplusplus
extern "C"
{
#endif

  struct sqlca_t
  {
    char sqlcaid[8];
    long sqlabc;
    long sqlcode;
    struct
    {
      int sqlerrml;
      char sqlerrmc[SQLERRMC_LEN];
    } sqlerrm;
    char sqlerrp[8];
    long sqlerrd[6];
    /* Element 0: empty                                             */
    /* 1: OID of processed tuple if applicable                      */
    /* 2: number of rows processed                          */
    /* after an INSERT, UPDATE or                           */
    /* DELETE statement                                     */
    /* 3: empty                                             */
    /* 4: empty                                             */
    /* 5: empty                                             */
    char sqlwarn[8];
    /* Element 0: set to 'W' if at least one other is 'W'   */
    /* 1: if 'W' at least one character string              */
    /* value was truncated when it was                      */
    /* stored into a host variable.                         */

    /*
     * 2: if 'W' a (hopefully) non-fatal notice occurred
     *//* 3: empty */
    /* 4: empty                                             */
    /* 5: empty                                             */
    /* 6: empty                                             */
    /* 7: empty                                             */

    char sqlstate[5];
  };

  struct sqlca_t *ECPGget_sqlca (void);

#ifndef POSTGRES_ECPG_INTERNAL
#define sqlca (*ECPGget_sqlca())
#endif

#ifdef __cplusplus
}
#endif

#endif

#line 4 "sub_targets.ec"


// ConstTarget
ConstTarget::ConstTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):
Target (in_tar_id, in_obs)
{
  /* exec sql begin declare section */




#line 11 "sub_targets.ec"
  double ra;

#line 12 "sub_targets.ec"
  double dec;

#line 13 "sub_targets.ec"
  int db_tar_id = in_tar_id;
/* exec sql end declare section */
#line 14 "sub_targets.ec"


  {
    ECPGdo (__LINE__, 0, 1, NULL,
	    "select  tar_ra  , tar_dec   from targets where tar_id  =  ?  ",
	    ECPGt_int, &(db_tar_id), (long) 1, (long) 1, sizeof (int),
	    ECPGt_NO_INDICATOR, NULL, 0L, 0L, 0L, ECPGt_EOIT, ECPGt_double,
	    &(ra), (long) 1, (long) 1, sizeof (double), ECPGt_NO_INDICATOR,
	    NULL, 0L, 0L, 0L, ECPGt_double, &(dec), (long) 1, (long) 1,
	    sizeof (double), ECPGt_NO_INDICATOR, NULL, 0L, 0L, 0L,
	    ECPGt_EORT);
  }
#line 24 "sub_targets.ec"

  if (sqlca.sqlcode)
    {
      throw sqlca.sqlcode;
    }
  position.ra = ra;
  position.dec = dec;
}

int
ConstTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  *pos = position;
  return 0;
}

int
ConstTarget::getRST (struct ln_rst_time *rst, double JD)
{
  return ln_get_object_rst (JD, observer, &position, rst);
}

// EllTarget - good for commets and so on
EllTarget::EllTarget (int in_tar_id, struct ln_lnlat_posn * in_obs):Target (in_tar_id,
	in_obs)
{
  /* exec sql begin declare section */








  // minimal magnitude


#line 50 "sub_targets.ec"
  double
    ell_minpause;

#line 51 "sub_targets.ec"
  double
    ell_a;

#line 52 "sub_targets.ec"
  double
    ell_e;

#line 53 "sub_targets.ec"
  double
    ell_i;

#line 54 "sub_targets.ec"
  double
    ell_w;

#line 55 "sub_targets.ec"
  double
    ell_omega;

#line 56 "sub_targets.ec"
  double
    ell_n;

#line 57 "sub_targets.ec"
  double
    ell_JD;

#line 58 "sub_targets.ec"
  double
    min_m;

#line 59 "sub_targets.ec"
  int
    db_tar_id = in_tar_id;
/* exec sql end declare section */
#line 60 "sub_targets.ec"


  {
    ECPGdo (__LINE__, 0, 1, NULL,
	    "select  extract( EPOCH from ell_minpause  ) , ell_a  , ell_e  , ell_i  , ell_w  , ell_omega  , ell_n  , ell_JD   from ell where ell . tar_id  =  ?  ",
	    ECPGt_int, &(db_tar_id), (long) 1, (long) 1, sizeof (int),
	    ECPGt_NO_INDICATOR, NULL, 0L, 0L, 0L, ECPGt_EOIT, ECPGt_double,
	    &(ell_minpause), (long) 1, (long) 1, sizeof (double),
	    ECPGt_NO_INDICATOR, NULL, 0L, 0L, 0L, ECPGt_double, &(ell_a),
	    (long) 1, (long) 1, sizeof (double), ECPGt_NO_INDICATOR, NULL, 0L,
	    0L, 0L, ECPGt_double, &(ell_e), (long) 1, (long) 1,
	    sizeof (double), ECPGt_NO_INDICATOR, NULL, 0L, 0L, 0L,
	    ECPGt_double, &(ell_i), (long) 1, (long) 1, sizeof (double),
	    ECPGt_NO_INDICATOR, NULL, 0L, 0L, 0L, ECPGt_double, &(ell_w),
	    (long) 1, (long) 1, sizeof (double), ECPGt_NO_INDICATOR, NULL, 0L,
	    0L, 0L, ECPGt_double, &(ell_omega), (long) 1, (long) 1,
	    sizeof (double), ECPGt_NO_INDICATOR, NULL, 0L, 0L, 0L,
	    ECPGt_double, &(ell_n), (long) 1, (long) 1, sizeof (double),
	    ECPGt_NO_INDICATOR, NULL, 0L, 0L, 0L, ECPGt_double, &(ell_JD),
	    (long) 1, (long) 1, sizeof (double), ECPGt_NO_INDICATOR, NULL, 0L,
	    0L, 0L, ECPGt_EORT);
  }
#line 83 "sub_targets.ec"

  if (sqlca.sqlcode)
    throw
      sqlca.
      sqlcode;
  orbit.a = ell_a;
  orbit.e = ell_e;
  orbit.i = ell_i;
  orbit.w = ell_w;
  orbit.omega = ell_omega;
  orbit.n = ell_n;
  orbit.JD = ell_JD;
}

int
EllTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  if (orbit.e == 1.0)
    {
      struct ln_par_orbit par_orbit;
      par_orbit.q = orbit.a;
      par_orbit.i = orbit.i;
      par_orbit.w = orbit.w;
      par_orbit.omega = orbit.omega;
      par_orbit.JD = orbit.JD;
      ln_get_par_body_equ_coords (JD, &par_orbit, pos);
      return 0;
    }
  else if (orbit.e > 1.0)
    {
      struct ln_hyp_orbit hyp_orbit;
      hyp_orbit.q = orbit.a;
      hyp_orbit.e = orbit.e;
      hyp_orbit.i = orbit.i;
      hyp_orbit.w = orbit.w;
      hyp_orbit.omega = orbit.omega;
      hyp_orbit.JD = orbit.JD;
      ln_get_hyp_body_equ_coords (JD, &hyp_orbit, pos);
      return 0;
    }
  ln_get_ell_body_equ_coords (JD, &orbit, pos);
  return 0;
}

int
EllTarget::getRST (struct ln_rst_time *rst, double JD)
{
  return ln_get_ell_body_rst (JD, observer, &orbit, rst);
}

// will pickup the Moon
LunarTarget::LunarTarget (int in_tar_id, struct ln_lnlat_posn * in_obs):Target (in_tar_id,
	in_obs)
{
}

int
LunarTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  ln_get_lunar_equ_coords (JD, pos, 0);
  return 0;
}

int
LunarTarget::getRST (struct ln_rst_time *rst, double JD)
{
  return ln_get_lunar_rst (JD, observer, rst);
}

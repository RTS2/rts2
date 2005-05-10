/* Processed by ecpg (3.1.1) */
/* These include files are added by the preprocessor */
#include <ecpgtype.h>
#include <ecpglib.h>
#include <ecpgerrno.h>
#include <sqlca.h>
#line 1 "target.ec"
/* End of automatic include section */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "target.h"
#include "../utils/config.h"

#include <syslog.h>


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

#line 10 "target.ec"


void
Target::logMsg (const char *message)
{
  printf ("%s\n", message);
}

void
Target::logMsg (const char *message, int num)
{
  printf ("%s %i\n", message, num);
}

void
Target::logMsg (const char *message, long num)
{
  printf ("%s %li\n", message, num);
}

void
Target::logMsg (const char *message, double num)
{
  printf ("%s %f\n", message, num);
}

void
Target::logMsg (const char *message, const char *val)
{
  printf ("%s %s\n", message, val);
}

Target::Target (int in_tar_id, struct ln_lnlat_posn *in_obs)
{
  observer = in_obs;

  obs_id = -1;
  target_id = in_tar_id;
}

int
Target::move ()
{
  return 0;
}

/**
 * Move to designated target, get astrometry, proof that target was acquired with given margin.
 *
 * @return 0 if I can observe futher, -1 if observation was canceled
 */
int
Target::acquire ()
{

}

int
Target::observe ()
{

}

/**
 * Return script for camera exposure.
 *
 * @param target        target id
 * @param camera_name   name of the camera
 * @param script        script
 * 
 * return -1 on error, 0 on success
 */
int
Target::getDBScript (int target, const char *camera_name, char *script)
{
  /* exec sql begin declare section */





#line 86 "target.ec"
  int tar_id = target;

#line 87 "target.ec"
  const char *cam_name = camera_name;

#line 88 "target.ec"
  struct varchar_sc_script
  {
    int len;
    char arr[2000];
  } sc_script;

#line 89 "target.ec"
  int sc_indicator;
/* exec sql end declare section */
#line 90 "target.ec"

  {
    ECPGdo (__LINE__, 0, 1, NULL,
	    "select  script   from scripts where tar_id  =  ? and camera_name  =  ?  ",
	    ECPGt_int, &(tar_id), (long) 1, (long) 1, sizeof (int),
	    ECPGt_NO_INDICATOR, NULL, 0L, 0L, 0L, ECPGt_char, &(cam_name),
	    (long) 0, (long) 1, 1 * sizeof (char), ECPGt_NO_INDICATOR, NULL,
	    0L, 0L, 0L, ECPGt_EOIT, ECPGt_varchar, &(sc_script), (long) 2000,
	    (long) 1, sizeof (struct varchar_sc_script), ECPGt_int,
	    &(sc_indicator), (long) 1, (long) 1, sizeof (int), ECPGt_EORT);
  }
#line 100 "target.ec"

  if (sqlca.sqlcode)
    goto err;
  if (sc_indicator < 0)
    goto err;
  strncpy (script, sc_script.arr, sc_script.len);
  script[sc_script.len] = '\0';
  return 0;
#undef test_sql
err:
  printf ("err db_get_script: %li %s\n", sqlca.sqlcode,
	  sqlca.sqlerrm.sqlerrmc);
  script[0] = '\0';
  return -1;
}


/**
 * Return script for camera exposure.
 *
 * @param device_name	camera device for script
 * @param buf		buffer for script
 *
 * @return 0 on success, < 0 on error
 */
int
Target::getScript (const char *device_name, char *buf)
{
  char obs_type_str[2];
  char *s;
  int ret;
  obs_type_str[0] = obs_type;
  obs_type_str[1] = 0;

  ret = getDBScript (target_id, device_name, buf);
  if (!ret)
    return 0;

  s = get_sub_device_string_default (device_name, "script", obs_type_str,
				     "E 10");
  strncpy (buf, s, MAX_COMMAND_LENGTH);
  return 0;
}

int
Target::postprocess ()
{

}

/****
 * 
 *   Return -1 if target is not suitable for observing,
 *   othrwise return 0.
 */
int
Target::considerForObserving (ObjectCheck * checker, double JD)
{
  // horizont constrain..
  struct ln_equ_posn curr_position;
  double gst = ln_get_mean_sidereal_time (JD);
  if (getPosition (&curr_position, JD))
    {
      changePriority (-100, JD + 1);
      return -1;
    }
  if (!checker->is_good (gst, curr_position.ra, curr_position.dec))
    {
      struct ln_rst_time rst;
      int ret;
      ret = getRST (&rst, JD);
      if (ret == -1)
	{
	  // object doesn't rise, let's hope tomorrow it will rise
	  changePriority (-100, JD + 1);
	  return -1;
	}
      // object is above horizont, but checker reject it..let's see what
      // will hapens in 10 minutes 
      if (rst.rise < JD)
	{
	  changePriority (-100, JD + 1.0 / 8640.0);
	  return -1;
	}
      changePriority (-100, rst.rise);
      return -1;
    }
  // target was selected for observation
  return 0;
}

int
Target::dropBonus ()
{
  /* exec sql begin declare section */


#line 195 "target.ec"
  int db_tar_id;
/* exec sql end declare section */
#line 196 "target.ec"


  {
    ECPGdo (__LINE__, 0, 1, NULL,
	    "update targets set tar_bonus  = null , tar_bonus_time  = null  where tar_id  =  ?",
	    ECPGt_int, &(db_tar_id), (long) 1, (long) 1, sizeof (int),
	    ECPGt_NO_INDICATOR, NULL, 0L, 0L, 0L, ECPGt_EOIT, ECPGt_EORT);
  }
#line 201 "target.ec"


  return !!sqlca.sqlcode;
}

int
Target::changePriority (int pri_change, double validJD)
{
  /* exec sql begin declare section */




#line 210 "target.ec"
  int db_tar_id = target_id;

#line 211 "target.ec"
  int db_priority_change = pri_change;

#line 212 "target.ec"
  long db_next_t;
/* exec sql end declare section */
#line 213 "target.ec"

  ln_get_timet_from_julian (validJD, &db_next_t);
  {
    ECPGdo (__LINE__, 0, 1, NULL,
	    "update targets set tar_bonus  = tar_bonus  +  ? , tar_bonus_time  = abstime (  ? )  where tar_id  = db_tar_id ",
	    ECPGt_int, &(db_priority_change), (long) 1, (long) 1,
	    sizeof (int), ECPGt_NO_INDICATOR, NULL, 0L, 0L, 0L, ECPGt_long,
	    &(db_next_t), (long) 1, (long) 1, sizeof (long),
	    ECPGt_NO_INDICATOR, NULL, 0L, 0L, 0L, ECPGt_EOIT, ECPGt_EORT);
  }
#line 221 "target.ec"

  if (!sqlca.sqlcode)
    return -1;
  return 0;
}

Target *
createTarget (int in_tar_id, struct ln_lnlat_posn * in_obs)
{
  /* exec sql begin declare section */



#line 230 "target.ec"
  int db_tar_id = in_tar_id;

#line 231 "target.ec"
  char db_type_id;
/* exec sql end declare section */
#line 232 "target.ec"


  {
    ECPGconnect (__LINE__, 0, "stars", NULL, NULL, NULL, 0);
  }
#line 234 "target.ec"


  try
  {
    {
      ECPGdo (__LINE__, 0, 1, NULL,
	      "select  type_id   from targets where tar_id  =  ?  ",
	      ECPGt_int, &(db_tar_id), (long) 1, (long) 1, sizeof (int),
	      ECPGt_NO_INDICATOR, NULL, 0L, 0L, 0L, ECPGt_EOIT, ECPGt_char,
	      &(db_type_id), (long) 1, (long) 1, 1 * sizeof (char),
	      ECPGt_NO_INDICATOR, NULL, 0L, 0L, 0L, ECPGt_EORT);
    }
#line 246 "target.ec"


    if (sqlca.sqlcode)
      throw & sqlca;

    // get more informations about target..
    switch (db_type_id)
      {
      case TYPE_ELLIPTICAL:
	return new EllTarget (in_tar_id, in_obs);
      case TYPE_GRB:
	return new TargetGRB (in_tar_id, in_obs);
      default:
	return new ConstTarget (in_tar_id, in_obs);
      }
  }
  catch (struct sqlca_t * sqlerr)
  {
    syslog (LOG_ERR, "Cannot create target: %i sqlcode: %i %s",
	    db_tar_id, sqlerr->sqlcode, sqlerr->sqlerrm.sqlerrmc);
  }
  return NULL;
}

#ifndef __RTS_TARGET__
#define __RTS_TARGET__
#include <libnova/libnova.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#include "image_info.h"

#define MAX_READOUT_TIME		120
#define EXPOSURE_TIMEOUT		50

#define EXPOSURE_TIME		60

#define TYPE_OPORTUNITY         'O'
#define TYPE_GRB                'G'
#define TYPE_GRB_TEST		'g'
#define TYPE_SKY_SURVEY         'S'
#define TYPE_GPS                'P'
#define TYPE_ELLIPTICAL         'E'
#define TYPE_HETE               'H'
#define TYPE_PHOTOMETRIC	'M'
#define TYPE_TECHNICAL          't'
#define TYPE_TERESTIAL		'T'
#define TYPE_CALIBRATION	'c'

#define COMMAND_EXPOSURE	'E'
#define COMMAND_FILTER		'F'
#define COMMAND_PHOTOMETER      'P'
#define COMMAND_CHANGE		'C'
#define COMMAND_WAIT            'W'
#define COMMAND_SLEEP		'S'
#define COMMAND_UTC_SLEEP	'U'
#define COMMAND_CENTERING	'c'
#define	COMMAND_FOCUSING	'f'

#define TARGET_FOCUSING		11

struct thread_list
{
  pthread_t thread;
  struct thread_list *next;
};

class Target;

struct ex_info
{
  struct device *camera;
  Target *last;
  int move_count;
};

/**
 * Class for one observation.
 *
 * It is created when it's possible to observer such target, and when such
 * observation have some scientific value.
 * 
 * It's put on the observation que inside planc. It's then executed.
 *
 * During execution it can do centering (acquiring images to better know scope position),
 * it can compute importance of future observations.
 *
 * After execution planc select new target for execution, run image processing
 * on first observation target in the que.
 *
 * After all images are processed, new priority can be computed, some results
 * (e.g. light curves) can be put to the database.
 *
 * Target is mainly backed by observation entry in the DB. It uses obs_state
 * field to store states of observation.
 */
class Target
{
private:
  struct device *telescope;
  struct ln_lnlat_posn *observer;
  struct thread_list thread_l;

  int get_info (struct device *cam,
		float exposure, hi_precision_t * hi_precision);

  virtual int runScript (struct ex_info *exinfo);	// script for each device
  virtual int get_script (struct device *camera, char *buf);
  int dec_script_thread_count (void);
  int join_script_thread ();

  pthread_mutex_t move_count_mutex;
  pthread_cond_t move_count_cond;
  int move_count;

  int precission_count;

  pthread_mutex_t script_thread_count_mutex;
  pthread_cond_t script_thread_count_cond;
  int script_thread_count;

  int running_script_count;	// number of running scripts (not in W)
public:
    Target::Target (struct device *tel, struct ln_lnlat_posn *obs)
  {
    printf ("tel: %p\n", tel);
    telescope = tel;
    observer = obs;

    pthread_mutex_init (&move_count_mutex, NULL);
    pthread_cond_init (&move_count_cond, NULL);
    move_count = 0;		// number of es 

    precission_count = 0;

    pthread_mutex_init (&script_thread_count_mutex, NULL);
    pthread_cond_init (&script_thread_count_cond, NULL);
    script_thread_count = 0;	// number of running scripts...

    running_script_count = 0;	// number of running scripts (not in W)

    thread_l.next = NULL;
    thread_l.thread = 0;
  }
  int getPosition (struct ln_equ_posn *pos)
  {
    return getPosition (pos, ln_get_julian_from_sys ());
  };
  virtual int getPosition (struct ln_equ_posn *pos, double JD)
  {
    return -1;
  };
  virtual int getRST (struct ln_equ_posn *pos, struct ln_rst_time *rst,
		      double jd)
  {
    return -1;
  };
  int move ();			// change position
  static void *runStart (void *exinfo);	// entry point for camera threads
  virtual int observe (Target * last);
  virtual int acquire ();	// one extra thread
  virtual int run ();		// start per device threads
  virtual int postprocess ();	// run in extra thread
  int type;			// light, dark, flat, flat_dark
  int id;
  int obs_id;
  char obs_type;		// SKY_SURVEY, GBR, .. 
  time_t start_time;
  int moved;
  int tolerance;
  int hi_precision;		// when 1, image will get imediate astrometry and telescope status will be updated
  Target *next;
};

class ConstTarget:public Target
{
private:
  struct ln_equ_posn position;
public:
    ConstTarget (struct device *tel, struct ln_lnlat_posn *obs,
		 struct ln_equ_posn *in_pos);
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int getRST (struct ln_lnlat_posn *observer, struct ln_rst_time *rst,
		      double jd);
};

class EllTarget:public Target
{
private:
  struct ln_ell_orbit orbit;
public:
    EllTarget (struct device *tel, struct ln_lnlat_posn *obs,
	       struct ln_ell_orbit *in_orbit);
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int getRST (struct ln_lnlat_posn *observer, struct ln_rst_time *rst,
		      double jd);
};

class ParTarget:public Target
{
private:
  struct ln_par_orbit orbit;
public:
    ParTarget (struct device *tel, struct ln_lnlat_posn *obs,
	       struct ln_par_orbit *in_orbit);
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int getRST (struct ln_lnlat_posn *observer, struct ln_rst_time *rst,
		      double jd);
};

class LunarTarget:public Target
{
public:
  LunarTarget (struct device *tel, struct ln_lnlat_posn *obs);
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int getRST (struct ln_lnlat_posn *observer, struct ln_rst_time *rst,
		      double jd);
};

#endif /*! __RTS_TARGET__ */

#ifndef __RTS2_SELECTOR__
#define __RTS2_SELECTOR__
#include "target.h"
#include "../utils/objectcheck.h"
#include <libnova/ln_types.h>
#include <time.h>

typedef enum
{ SELECTOR_AIRMASS, SELECTOR_ALTITUDE, SELECTOR_HETE,
  SELECTOR_GPS, SELECTOR_ELL, SELECTOR_PHOTOMETRY
} selector_t;


class Selector
{
private:
  ObjectCheck * checker;
  struct device *telescope;
  struct ln_lnlat_posn *observer;
  int find_plan (int id, time_t c_start);
  Target *add_target (Target * plan, int type, int id, double ra,
		      double dec, time_t obs_time, int tolerance,
		      char obs_type);
  Target *add_target_ell (Target * plan, int type, int id,
			  ln_ell_orbit * orbit, time_t obs_time,
			  int tolerance, char obs_type);
  int select_next_alt (time_t c_start, Target * plan, float lon, float lat);
  int select_next_gps (time_t c_start, Target * plan, float lon, float lat);
  int
    select_next_airmass (time_t c_start, Target * plan,
			 float target_airmass, float az_end, float az_start,
			 float lon, float lat);
  int select_next_grb (time_t c_start, Target * plan, float lon, float lat);
  int
    select_next_to (time_t * c_start, Target * plan, float az_end,
		    float az_start, float lon, float lat);
  int
    select_next_ell (time_t * c_start, Target * plan, float az_end,
		     float az_start, float lon, float lat);
  int
    select_next_photometry (time_t * c_start, Target * plan, float lon,
			    float lat);

  int
    select_next_terestial (time_t * c_start, Target * plan, float lon,
			   float lat);

  int
    flat_field (Target * plan, time_t * obs_start, int number, float lon,
		float lat);

  int hete_mosaic (Target * plan, double jd, time_t * obs_start, int number);

public:
    Selector (ObjectCheck * check, struct device *tel,
	      struct ln_lnlat_posn *obs)
  {
    checker = check;
    telescope = tel;
    observer = obs;
  };

  ~Selector (void)
  {

  };

  int get_next_plan (Target * plan, int selector_type,
		     time_t * obs_time, int number, float exposure, int state,
		     float lon, float lat, int ignore_astro);

  void free_plan (Target * plan);
  int get_obs_id ();
};

int createTarget (Target ** in_target, int tar_id, struct device *telescope,
		  struct ln_lnlat_posn *observer);

#endif /* __RTS2_SELECTOR__ */

#include "rts2targetplanet.h"
#include "../utils/infoval.h"
#include "../utils/libnova_cpp.h"

#define PLANETS		10

planet_info_t planets[PLANETS] = 
{
  {
    "Sun",
    ln_get_solar_equ_coords,
    ln_get_earth_solar_dist,
    NULL,
    NULL,
    ln_get_solar_sdiam,
    NULL,
    NULL
  },
  {
    "Mercury",
    ln_get_mercury_equ_coords,
    ln_get_mercury_earth_dist,
    ln_get_mercury_solar_dist,
    ln_get_mercury_magnitude,
    ln_get_mercury_sdiam,
    ln_get_mercury_phase,
    ln_get_mercury_disk
  },
  {
    "Venus",
    ln_get_venus_equ_coords,
    ln_get_venus_earth_dist,
    ln_get_venus_solar_dist,
    ln_get_venus_magnitude,
    ln_get_venus_sdiam,
    ln_get_venus_phase,
    ln_get_venus_disk
  },
  {
    "Moon",
    ln_get_lunar_equ_coords,
    ln_get_lunar_earth_dist,
    ln_get_earth_solar_dist,
    NULL,
    ln_get_lunar_sdiam,
    ln_get_lunar_phase,
    ln_get_lunar_disk
  },
  {
    "Mars",
    ln_get_mars_equ_coords,
    ln_get_mars_earth_dist,
    ln_get_mars_solar_dist,
    ln_get_mars_magnitude,
    ln_get_mars_sdiam,
    ln_get_mars_phase,
    ln_get_mars_disk
  },
  {
    "Jupiter",
    ln_get_jupiter_equ_coords,
    ln_get_jupiter_earth_dist,
    ln_get_jupiter_solar_dist,
    ln_get_jupiter_magnitude,
    ln_get_jupiter_equ_sdiam,
    ln_get_jupiter_phase,
    ln_get_jupiter_disk
  },
  {
    "Saturn",
    ln_get_saturn_equ_coords,
    ln_get_saturn_earth_dist,
    ln_get_saturn_solar_dist,
    ln_get_saturn_magnitude,
    ln_get_saturn_equ_sdiam,
    ln_get_saturn_phase,
    ln_get_saturn_disk
  },
  {
    "Uranus",
    ln_get_uranus_equ_coords,
    ln_get_uranus_earth_dist,
    ln_get_uranus_solar_dist,
    ln_get_uranus_magnitude,
    ln_get_uranus_sdiam,
    ln_get_uranus_phase,
    ln_get_uranus_disk
  },
  {
    "Neptune",
    ln_get_neptune_equ_coords,
    ln_get_neptune_earth_dist,
    ln_get_neptune_solar_dist,
    ln_get_neptune_magnitude,
    ln_get_neptune_sdiam,
    ln_get_neptune_phase,
    ln_get_neptune_disk
  },
  {
    "Pluto",
    ln_get_pluto_equ_coords,
    ln_get_pluto_earth_dist,
    ln_get_pluto_solar_dist,
    ln_get_pluto_magnitude,
    ln_get_pluto_sdiam,
    ln_get_pluto_phase,
    ln_get_pluto_disk
  }
};

TargetPlanet::TargetPlanet (int tar_id, struct ln_lnlat_posn *in_obs):Target (tar_id, in_obs)
{
  planet_info = NULL;
}

TargetPlanet::~TargetPlanet (void)
{
}

int
TargetPlanet::load ()
{
  int ret;
  ret = Target::load ();
  if (ret)
    return ret;

  planet_info = NULL;

  for (int i = 0; i < PLANETS; i++)
  {
    if (!strcmp (getTargetName (), planets[i].name))
    {
      planet_info = &planets[i];
      return 0;
    }
  }
  return -1;
}

int
TargetPlanet::getPosition (struct ln_equ_posn *pos, double JD, struct ln_equ_posn *parallax)
{
  planet_info->rst_func (JD, pos);

  ln_get_parallax (pos, getEarthDistance (JD), observer, 1706, JD, parallax);

  pos->ra += parallax->ra;
  pos->dec += parallax->dec;
  
  return 0;
}

int
TargetPlanet::getPosition (struct ln_equ_posn *pos, double JD)
{
  struct ln_equ_posn parallax;

  return getPosition (pos, JD, &parallax);
}

int
TargetPlanet::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
  ln_get_body_rst_horizon (JD, observer, planet_info->rst_func, horizon, rst);
  return 0;
}

int
TargetPlanet::isContinues ()
{
  return 0;
}

void
TargetPlanet::printExtra (std::ostream & _os, double JD)
{
  struct ln_equ_posn pos, parallax;
  getPosition (&pos, JD, &parallax);
  _os
    << std::endl
    << InfoVal<double> ("EARTH DISTANCE", getEarthDistance (JD))
    << InfoVal<double> ("SOLAR DISTANCE", getSolarDistance (JD))
    << InfoVal<double> ("MAGNITUDE", getMagnitude (JD))
    << InfoVal<LibnovaDegDist> ("SDIAM", LibnovaDegDist (getSDiam (JD)))
    << InfoVal<double> ("PHASE", getPhase (JD))
    << InfoVal<double> ("DISK", getDisk (JD))
    << InfoVal<LibnovaDegDist> ("PARALLAX RA", LibnovaDegDist (parallax.ra))
    << InfoVal<LibnovaDegDist> ("PARALLAX DEC", LibnovaDegDist (parallax.dec))
    << std::endl;
  Target::printExtra (_os, JD);
}

double
TargetPlanet::getEarthDistance (double JD)
{
  double ret;
  ret = planet_info->earth_func (JD);
  // moon distance is in km:(
  if (planet_info == &(planets[3]))
    return ret / 149.598E6;
  return ret;
}

double
TargetPlanet::getSolarDistance (double JD)
{
  if (planet_info->sun_func)
    return planet_info->sun_func (JD);
  return 0;
}

double
TargetPlanet::getMagnitude (double JD)
{
  if (planet_info->mag_func)
    return planet_info->mag_func (JD);
  return nan("f");
}

double
TargetPlanet::getSDiam (double JD)
{
  if (planet_info->sdiam_func)
    // return value is in arcsec..
    return planet_info->sdiam_func (JD) / 3600.0;
  return nan("f");
}

double
TargetPlanet::getPhase (double JD)
{
  if (planet_info->phase_func)
    return planet_info->phase_func (JD);
  return nan("f");
}

double
TargetPlanet::getDisk (double JD)
{
  if (planet_info->disk_func)
    return planet_info->disk_func (JD);
  return nan("f");
}

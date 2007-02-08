#include "rts2targetplanet.h"

#define PLANETS		10

planet_info_t planets[PLANETS] = 
{
  {"Sun", ln_get_solar_equ_coords},
  {"Mercury", ln_get_mercury_equ_coords},
  {"Venus", ln_get_venus_equ_coords},
  {"Moon", ln_get_lunar_equ_coords},
  {"Mars", ln_get_mars_equ_coords},
  {"Jupiter", ln_get_jupiter_equ_coords},
  {"Saturn", ln_get_saturn_equ_coords},
  {"Uranus", ln_get_uranus_equ_coords},
  {"Neptune", ln_get_neptune_equ_coords},
  {"Pluto", ln_get_pluto_equ_coords}
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
TargetPlanet::getPosition (struct ln_equ_posn *pos, double JD)
{
  planet_info->function (JD, pos);
  return 0;
}

int
TargetPlanet::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
  ln_get_body_rst_horizon (JD, observer, planet_info->function, horizon, rst);
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

}

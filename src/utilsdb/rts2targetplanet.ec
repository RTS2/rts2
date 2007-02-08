#include "rts2targetplanet.h"

TargetPlanet::TargetPlanet (int tar_id, struct ln_lnlat_posn *in_obs):Target (tar_id, in_obs)
{
  planet_id = -1;
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

  planet_id = -1;

  if (!strcmp (getTargetName (), "Sun"))
    planet_id = 0;
  if (!strcmp (getTargetName (), "Mercury"))
    planet_id = 1;
  if (!strcmp (getTargetName (), "Venus"))
    planet_id = 2;
  if (!strcmp (getTargetName (), "Moon"))
    planet_id = 3;
  if (!strcmp (getTargetName (), "Mars"))
    planet_id = 4;
  if (!strcmp (getTargetName (), "Jupiter"))
    planet_id = 5;
  if (!strcmp (getTargetName (), "Saturn"))
    planet_id = 6;
  if (!strcmp (getTargetName (), "Urans"))
    planet_id = 7;
  if (!strcmp (getTargetName (), "Neptune"))
    planet_id = 8;
  if (!strcmp (getTargetName (), "Pluto"))
    planet_id = 9;
  if (planet_id == -1)
    return -1;
  return 0;
}

int
TargetPlanet::getPosition (struct ln_equ_posn *pos, double JD)
{
  switch (planet_id)
  {
    case 0:
      ln_get_solar_equ_coords (JD, pos);
      break;
    case 1:
      ln_get_mercury_equ_coords (JD, pos);
      break;
    case 2:
      ln_get_venus_equ_coords (JD, pos);
      break;
    case 3:
      ln_get_lunar_equ_coords (JD, pos);
      break;
    case 4:
      ln_get_mars_equ_coords (JD, pos);
      break;
    case 5:
      ln_get_jupiter_equ_coords (JD, pos);
      break;
    case 6:
      ln_get_saturn_equ_coords (JD, pos);
      break;
    case 7:
      ln_get_uranus_equ_coords (JD, pos);
      break;
    case 8:
      ln_get_neptune_equ_coords (JD, pos);
      break;
    case 9:
      ln_get_pluto_equ_coords (JD, pos);
      break;
  }
  return 0;
}

int
TargetPlanet::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
  switch (planet_id)
  {
    case 0:
      ln_get_solar_rst_horizon (JD, observer, horizon, rst);
      break;
    case 1:
      ln_get_body_rst_horizon (JD, observer,
      ln_get_mercury_equ_coords, horizon, rst);
      break;
    case 2:
      ln_get_body_rst_horizon (JD, observer,
      ln_get_venus_equ_coords, horizon, rst);
      break;
    case 3:
      ln_get_body_rst_horizon (JD, observer,
      ln_get_lunar_equ_coords, horizon, rst);
      break;
    case 4:
      ln_get_body_rst_horizon (JD, observer,
      ln_get_mars_equ_coords, horizon, rst);
      break;
    case 5:
      ln_get_body_rst_horizon (JD, observer,
      ln_get_jupiter_equ_coords, horizon, rst);
      break;
    case 6:
      ln_get_body_rst_horizon (JD, observer,
      ln_get_saturn_equ_coords, horizon, rst);
      break;
    case 7:
      ln_get_body_rst_horizon (JD, observer,
      ln_get_uranus_equ_coords, horizon, rst);
      break;
    case 8:
      ln_get_body_rst_horizon (JD, observer,
      ln_get_neptune_equ_coords, horizon, rst);
      break;
    case 9:
      ln_get_body_rst_horizon (JD, observer,
      ln_get_pluto_equ_coords, horizon, rst);
      break;
  }
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

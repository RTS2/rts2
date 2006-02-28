#include "modelterm.h"
#include <math.h>

double
in180 (double x)
{
  for (; x > 180; x -= 360);
  for (; x < -180; x += 360);
  return x;
}

std::ostream & operator << (std::ostream & os, Rts2ModelTerm * term)
{
  os << term->name << " = " << term->corr << std::endl;
  return os;
}

// status OK
void
Rts2TermME::apply (struct ln_equ_posn *pos,
		   Rts2ObsConditions * obs_conditions)
{
  double h0, d0, h1, d1, M, N;

  h0 = ln_deg_to_rad (in180 (pos->ra));
  d0 = ln_deg_to_rad (in180 (pos->dec));

  N = asin (sin (h0) * cos (d0));
  M = asin (sin (d0) / cos (N));

  if ((h0 > (M_PI / 2)) || (h0 < (-M_PI / 2)))
    {
      if (M > 0)
	M = M_PI - M;
      else
	M = -M_PI + M;
    }

  M = M - ln_deg_to_rad (corr);

  if (M > M_PI)
    M -= 2 * M_PI;
  if (M < -M_PI)
    M += 2 * M_PI;

  d1 = asin (sin (M) * cos (N));
  h1 = asin (sin (N) / cos (d1));

  if (M > (M_PI / 2))
    h1 = M_PI - h1;
  if (M < (-M_PI / 2))
    h1 = -M_PI + h1;

  if (h1 > M_PI)
    h1 -= 2 * M_PI;
  if (h1 < -M_PI)
    h1 += 2 * M_PI;

  pos->ra = ln_rad_to_deg (h1);
  pos->dec = ln_rad_to_deg (d1);
}

// status OK
void
Rts2TermMA::apply (struct ln_equ_posn *pos,
		   Rts2ObsConditions * obs_conditions)
{
  double d, h;

  d = pos->dec - (corr) * sin (ln_deg_to_rad (pos->ra));
  h =
    pos->ra +
    (corr) * cos (ln_deg_to_rad (pos->ra)) * tan (ln_deg_to_rad (pos->dec));

  pos->ra = h;
  pos->dec = d;
}

// status: OK
void
Rts2TermIH::apply (struct ln_equ_posn *pos,
		   Rts2ObsConditions * obs_conditions)
{
  // Add a zero point to the hour angle
  pos->ra = in180 (pos->ra - corr);
  // No change for declination
}

// status: OK
void
Rts2TermID::apply (struct ln_equ_posn *pos,
		   Rts2ObsConditions * obs_conditions)
{
  // Add a zero point to the declination
  pos->dec = in180 (pos->dec - corr);
  // No change for hour angle
}

// status: OK
void
Rts2TermCH::apply (struct ln_equ_posn *pos,
		   Rts2ObsConditions * obs_conditions)
{
  pos->ra = pos->ra - (corr) / cos (ln_deg_to_rad (pos->dec));
}

// status: error?
void
Rts2TermNP::apply (struct ln_equ_posn *pos,
		   Rts2ObsConditions * obs_conditions)
{
  pos->ra = pos->ra - (corr) * tan (ln_deg_to_rad (pos->ra));
  pos->dec = pos->dec;
}

// status: ok
void
Rts2TermPHH::apply (struct ln_equ_posn *pos,
		    Rts2ObsConditions * obs_conditions)
{
  pos->ra =
    pos->ra - ln_rad_to_deg (ln_deg_to_rad (corr) * ln_deg_to_rad (pos->ra));
}

// status: ok
void
Rts2TermPDD::apply (struct ln_equ_posn *pos,
		    Rts2ObsConditions * obs_conditions)
{
  pos->dec =
    pos->dec -
    ln_rad_to_deg (ln_deg_to_rad (corr) * ln_deg_to_rad (pos->dec));
}

// status: testing
void
Rts2TermA1H::apply (struct ln_equ_posn *pos,
		    Rts2ObsConditions * obs_conditions)
{
  pos->ra =
    pos->ra -
    ln_rad_to_deg (ln_deg_to_rad (corr) * obs_conditions->getFlip ());
}

// status: testing
void
Rts2TermA1D::apply (struct ln_equ_posn *pos,
		    Rts2ObsConditions * obs_conditions)
{
  pos->dec =
    pos->dec -
    ln_rad_to_deg (ln_deg_to_rad (corr) * obs_conditions->getFlip ());
}

void
Rts2TermTF::apply (struct ln_equ_posn *pos,
		   Rts2ObsConditions * obs_conditions)
{
  double d, h, f;
  d = ln_deg_to_rad (pos->dec);
  h = ln_deg_to_rad (pos->ra);
  f = ln_deg_to_rad (obs_conditions->getLatitude ());

  pos->ra = pos->ra + (corr) * cos (f) * sin (h) / cos (d);
  pos->dec =
    pos->dec + (corr) * (cos (f) * cos (h) * sin (d) - sin (f) * cos (d));
}

void
Rts2TermTX::apply (struct ln_equ_posn *pos,
		   Rts2ObsConditions * obs_conditions)
{
  double d, h, f;
  d = ln_deg_to_rad (pos->dec);
  h = ln_deg_to_rad (pos->ra);
  f = ln_deg_to_rad (obs_conditions->getLatitude ());

  pos->ra =
    pos->ra +
    (corr) * cos (f) * sin (h) / cos (d) /
    (cos (d) * sin (f) + cos (d) * cos (h) * cos (f));
  pos->dec =
    pos->dec +
    (corr) * (cos (f) * cos (h) * sin (d) - sin (f) * cos (d)) /
    (cos (d) * sin (f) + cos (d) * cos (h) * cos (f));
}

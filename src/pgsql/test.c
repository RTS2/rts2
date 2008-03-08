#include <math.h>
#include <stdio.h>

struct kwcs2
{
  int naxis1;			/* Number of pixels along x-axis */
  int naxis2;			/* Number of pixels along y-axis */
  // we assume type RA---TAN and DEC--TAN
  double crpix1;		/* Reference pixel coordinates */
  double crpix2;		/* Reference pixel coordinates */
  double crval1;		/* Coordinate at reference pixel in degrees */
  double crval2;		/* Coordinate at reference pixel in degrees */
  double cd1_1;			/* Rotation matrix */
  double cd1_2;
  double cd2_1;
  double cd2_2;
  double equinox;		/* Epoch of coordinates, for FK4/FK5 conversion */
};

double
deg2rad (double deg)
{
  return deg * M_PI / 180.0;
}

double
rad2deg (double rad)
{
  return rad * 180.0 / M_PI;
}

/*!
 * Calculate word coordinates of given pixel.
 */
void
RTS2pix2wcs (struct kwcs2 *arg, double x, double y, double *ra, double *dec)
{
  double ra0, dec0;
  double dx, dy, tx;
  double phi, theta, r_theta;

  ra0 = deg2rad (arg->crval1);
  dec0 = deg2rad (arg->crval2);

  // Offset from ref pixel
  dx = x - arg->crpix1;
  dy = y - arg->crpix2;

  // Scale and rotate using CD matrix
  tx = arg->cd1_1 * dx + arg->cd1_2 * dy;
  dy = arg->cd2_1 * dx + arg->cd2_2 * dy;
  dx = tx;

  dx = deg2rad (dx);
  dy = deg2rad (dy);

  phi = atan2 (dx, -dy);	// (14), p. 1085
  r_theta = sqrt (dx * dx + dy * dy);	// (15), p. 1085
  theta = atan2 (1, r_theta);	// (17), p. 1085

  printf ("dx: %f dy: %f phi: %f r_theta: %f theta: %f\n", dx, dy, phi,
	  r_theta, theta);

  printf ("atan2: %f\n",
	  sin (theta) * cos (dec0) - cos (theta) * sin (dec0) * cos (phi -
								     M_PI));

  *ra = ra0 + atan2 (-cos (theta) * sin (phi - M_PI), sin (theta) * cos (dec0) - cos (theta) * sin (dec0) * cos (phi - M_PI));	// (2), p. 1079
  *dec = asin (sin (theta) * sin (dec0) + cos (theta) * cos (dec0) * cos (phi - M_PI));	// (2), p. 1079

  *ra = rad2deg (*ra);
  *dec = rad2deg (*dec);
}

int
main (int argc, char **argv)
{
  struct kwcs2 wcs;

  double ra, dec;

  wcs.crpix1 = 400;
  wcs.crpix2 = 400;
  wcs.crval1 = 253;
  wcs.crval2 = -43;
  wcs.cd1_1 = -0.08;
  wcs.cd1_2 = 0;
  wcs.cd2_1 = 0;
  wcs.cd2_2 = 0.08;
  wcs.equinox = 2000.0;

  RTS2pix2wcs (&wcs, 0, 0, &ra, &dec);
  printf ("ra: %f dec : %f\n", ra, dec);

  RTS2pix2wcs (&wcs, 400, 400, &ra, &dec);
  printf ("ra: %f dec : %f\n", ra, dec);

  RTS2pix2wcs (&wcs, 800, 800, &ra, &dec);
  printf ("ra: %f dec : %f\n", ra, dec);
}

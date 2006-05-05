#include <libnova/libnova.h>
#include <iostream>
#include <iomanip>

#include "rts2image.h"
#include "../utils/libnova_cpp.h"

void
printOffset (double x, double y, Rts2Image * image)
{
  double sep;
  double x_out;
  double y_out;
  double ra, dec;

  image->getOffset (x, y, x_out, y_out, sep);
  image->getRaDec (x, y, ra, dec);

  std::ios_base::fmtflags old_settings =
    std::cout.setf (std::ios_base::fixed, std::ios_base::floatfield);

  int old_p = std::cout.precision (2);

  std::cout << "Rts2Image::getOffset ("
    << std::setw (10) << x << ", "
    << std::setw (10) << y << "): "
    << "RA " << LibnovaRa (ra) << " "
    << LibnovaDegArcMin (x_out)
    << " DEC " << LibnovaDec (dec) << " "
    << LibnovaDegArcMin (y_out) << " ("
    << LibnovaDegArcMin (sep) << ")" << std::endl;

  std::cout.precision (old_p);
  std::cout.setf (old_settings);
}

int
main (int argc, char **argv)
{
  double ra;
  double dec;
  double x;
  double y;
  Rts2Image *image;
  image = new Rts2Image (argv[1]);
  std::cout << image << std::endl;
  std::cout << "Image XoA and Yoa: [" << image->getXoA () << ":" << image->
    getYoA () << "]" << std::endl;
  std::cout << "[XoA:YoA] RA: " << image->
    getCenterRa () << " DEC: " << image->getCenterDec () << std::endl;
  std::cout << "FLIP: " << image->getFlip () << std::endl;
  image->getRaDec (image->getXoA (), image->getYoA (), ra, dec);
  std::cout << "ROTANG: " << ln_rad_to_deg (image->
					    getRotang ()) << " (deg) XPLATE: "
    << image->getXPlate () << " YPLATE: " << image->getYPlate () << std::endl;
  std::cout << "RA and DEC of [XoA:YoA]: " << ra << ", " << dec << std::endl;
  image->getRaDec (0, 0, ra, dec);
  std::cout << "RA and DEC of [0:0]: " << ra << ", " << dec << std::endl;
  image->getRaDec (image->getWidth (), 0, ra, dec);
  std::cout << "RA and DEC of [W:0]: " << ra << ", " << dec << std::endl;
  image->getRaDec (0, image->getHeight (), ra, dec);
  std::cout << "RA and DEC of [0:H]: " << ra << ", " << dec << std::endl;
  image->getRaDec (image->getWidth (), image->getHeight (), ra, dec);
  std::cout << "RA and DEC of [W:H]: " << ra << ", " << dec << std::endl;
  std::cout << "Rts2Image::getCenterRow " << image->getCenter (x, y,
							       3) << " " << x
    << ":" << y << std::endl;

  printOffset (image->getXoA () + 50, image->getYoA (), image);
  printOffset (image->getXoA (), image->getYoA () + 50, image);
  printOffset (image->getXoA () - 50, image->getYoA (), image);
  printOffset (image->getXoA (), image->getYoA () - 50, image);

  return 0;
}

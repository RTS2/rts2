#include <libnova/libnova.h>
#include <iostream>

#include "rts2image.h"

int
main (int argc, char **argv)
{
  double ra;
  double dec;
  double x;
  double y;
  Rts2Image *image;
  image = new Rts2Image (argv[1]);
  std::cout << "Image dimension: " << image->getWidth () << "x" << image->
    getHeight () << " pixels" << std::endl;
  std::cout << "Image XoA and Yoa: [" << image->getXoA () << ":" << image->
    getYoA () << "]" << std::endl;
  std::cout << "[XoA:YoA] RA: " << image->
    getCenterRa () << " DEC: " << image->getCenterDec () << std::endl;
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
  std::
    cout << "Rts2Image::getOffset (659.433000, 94.990000, x, y) " << image->
    getOffset (659.433000, 94.990000, x,
	       y) << " " << x << " " << y << std::endl;
  return 0;
}

#include "rts2appdbimage.h"
#include "../utils/rts2config.h"

#include <signal.h>
#include <iostream>
#include <iomanip>

#include <list>

#define IMAGEOP_NOOP		0x00
#define IMAGEOP_ADDDATE		0x01
#define IMAGEOP_INSERT		0x02
#define IMAGEOP_TEST		0x04

class Rts2AppImageManip:public Rts2AppDbImage
{
private:
  int operation;

  void printOffset (double x, double y, Rts2Image * image);

  int addDate (Rts2Image * image);
  int insert (Rts2ImageDb * image);
  int testImage (Rts2Image * image);
protected:
    virtual int processOption (int in_opt);
  virtual int processImage (Rts2ImageDb * image);
public:
    Rts2AppImageManip (int in_argc, char **in_argv);
};

void
Rts2AppImageManip::printOffset (double x, double y, Rts2Image * image)
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
Rts2AppImageManip::addDate (Rts2Image * image)
{
  int ret;
  time_t t;
  std::cout << "Adding date " << image->getImageName () << "..";
  t = image->getExposureSec ();
  image->setValue ("DATE-OBS", &t, image->getExposureUsec (),
		   "date of observation");
  ret = image->saveImage ();
  std::cout << (ret ? "failed" : "OK") << std::endl;
  return ret;
}

int
Rts2AppImageManip::insert (Rts2ImageDb * image)
{
  return image->saveImage ();
}

int
Rts2AppImageManip::testImage (Rts2Image * image)
{
  double ra, dec, x, y;
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

  printOffset (152, 150, image);

  return 0;
}

int
Rts2AppImageManip::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'a':
      operation &= IMAGEOP_ADDDATE;
      break;
    case 'i':
      operation &= IMAGEOP_INSERT;
      break;
    case 't':
      operation &= IMAGEOP_TEST;
      break;
    default:
      return Rts2AppDbImage::processOption (in_opt);
    }
  return 0;
}

int
Rts2AppImageManip::processImage (Rts2ImageDb * image)
{
  if (operation & IMAGEOP_ADDDATE)
    addDate (image);
  if (operation & IMAGEOP_INSERT)
    insert (image);
  if (operation & IMAGEOP_TEST)
    testImage (image);
  return 0;
}

Rts2AppImageManip::Rts2AppImageManip (int in_argc, char **in_argv):Rts2AppDbImage (in_argc,
		in_argv)
{
  Rts2Config *
    config;
  config = Rts2Config::instance ();

  operation = IMAGEOP_NOOP;
  addOption ('d', "add-date", 0, "add DATE-OBS to image header");
  addOption ('i', "insert", 0, "insert/update images in the database");
}

Rts2AppImageManip *
  app;

void
killSignal (int sig)
{
  if (app)
    delete
      app;
}

int
main (int argc, char **argv)
{
  int
    ret;
  app = new Rts2AppImageManip (argc, argv);
  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);
  ret = app->init ();
  if (ret)
    {
      delete
	app;
      return 0;
    }
  ret = app->run ();
  delete
    app;
  return ret;
}

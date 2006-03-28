#include <string>
#include <vector>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <libnova/libnova.h>

#include <ostream>
#include <iostream>

#include "../utils/rts2app.h"
#include "../utils/rts2config.h"
#include "../writers/rts2image.h"

class TPM:public Rts2App
{
private:
  std::vector < std::string > filenames;
  int headline (Rts2Image * image, std::ostream & _os);
  int printImage (Rts2Image * image, std::ostream & _os);

  struct ln_lnlat_posn obs;
protected:
    virtual int processOption (int in_opt);
  virtual int processArgs (const char *arg);
public:
    TPM (int argc, char **argv);
    virtual ~ TPM (void);

  virtual int init ();
  virtual int run ();

  virtual void help ();
};

TPM::TPM (int in_argc, char **in_argv):
Rts2App (in_argc, in_argv)
{
}

TPM::~TPM (void)
{
  filenames.clear ();
}

int
TPM::processOption (int in_opt)
{
  return Rts2App::processOption (in_opt);
}

int
TPM::processArgs (const char *arg)
{
  filenames.push_back (arg);
  return 0;
}

int
TPM::init ()
{
  int ret;
  ret = Rts2App::init ();
  if (ret)
    return ret;
  if (filenames.empty ())
    {
      std::cerr << "Need to have at least one filename!" << std::endl;
      help ();
      return -1;
    }
  return 0;
}

int
TPM::run ()
{
  bool firstLine = false;
  for (std::vector < std::string >::iterator iter = filenames.begin ();
       iter != filenames.end (); iter++)
    {
      Rts2Image *image = new Rts2Image ((*iter).c_str ());
      if (!firstLine)
	{
	  headline (image, std::cout);
	  firstLine = true;
	}
      printImage (image, std::cout);
      delete image;
    }
  return 0;
}

void
TPM::help ()
{
  std::
    cout <<
    "Process list of images with astrometry, and creates file which you can feed to TPoint"
    << std::endl;
  std::
    cout << "Without any switch, prouduce file with J2000 mean coordinates."
    << std::endl;
  std::
    cout <<
    "Option proudced should be sufficient to run it throught TPOINT and get model"
    << std::endl;
  Rts2App::help ();
}

int
TPM::headline (Rts2Image * image, std::ostream & _os)
{
  obs.lat = Rts2Config::instance ()->getObserver ()->lat;
  obs.lng = Rts2Config::instance ()->getObserver ()->lng;
  // try to get latitude from image
  image->getValue ("LAT", obs.lat);
  image->getValue ("LONG", obs.lng);
  // standart header
  _os << "RTS2 model from astrometry" << std::endl << ":EQUAT" << std::endl	// we are observing on equatorial mount
    << ":ALLSKY" << std::endl	// we have images from all sky
    << ":NODA" << std::endl	// don't know
    << " " << LibnovaDeg90 (obs.lat) << " 2000 1 01" << std::endl;	// we have J2000, not refracted coordinates from mount
  return 0;
}

int
TPM::printImage (Rts2Image * image, std::ostream & _os)
{
  LibnovaRaDec actual;
  LibnovaRaDec target;
  time_t ct;
  double JD;
  double mean_sidereal;
  float expo;
  int flip;
  double aux1;

  int ret;

  ret = image->getCoordTarget (actual);
  if (ret)
    return ret;
  ret = image->getCoordTarget (target);
  if (ret)
    return ret;

  ret = image->getValue ("CTIME", ct);
  if (ret)
    return ret;

  ret = image->getValue ("EXPOSURE", expo);
  if (ret)
    return ret;

  flip = image->getMountFlip ();
  ret = image->getValue ("MNT_AX1", aux1);
  if (ret)
    return ret;

  ct = (time_t) (ct + expo / 2);

  JD = ln_get_julian_from_timet (&ct);
  mean_sidereal =
    ln_range_degrees (15 * ln_get_apparent_sidereal_time (JD) - obs.lng);

  LibnovaHaM lst (mean_sidereal);

  _os << actual << " 0 0 2000.0 " << target << " " << lst << " " << flip <<
    " " << aux1 << std::endl;
  return 0;
}

int
main (int argc, char **argv)
{
  int ret;

  TPM *app = new TPM (argc, argv);
  ret = app->init ();
  if (ret)
    return ret;
  ret = app->run ();
  delete app;
  return ret;
}

#include <string>
#include <vector>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <libnova/libnova.h>

#include <ostream>
#include <iostream>

#include "../utils/rts2cliapp.h"
#include "../utils/rts2config.h"
#include "../writers/rts2image.h"

class TPM:public Rts2CliApp
{
private:
  std::vector < std::string > filenames;
  int headline (Rts2Image * image, std::ostream & _os);
  int printImage (Rts2Image * image, std::ostream & _os);
  // select images with given flip; -1 for all flip, 0 or 1 for given flip
  int selFlip;

  struct ln_lnlat_posn obs;

  double ra_step;
  double ra_offset;
  double dec_step;
  double dec_offset;

  enum
  { TARGET, BEST, MOUNT } tarCorType;
protected:
    virtual int processOption (int in_opt);
  virtual int processArgs (const char *arg);
  virtual int init ();
public:
    TPM (int argc, char **argv);
    virtual ~ TPM (void);

  virtual int doProcessing ();

  virtual void help ();
};

TPM::TPM (int in_argc, char **in_argv):
Rts2CliApp (in_argc, in_argv)
{
  tarCorType = BEST;
  selFlip = -1;
  ra_step = nan ("f");
  ra_offset = 0;
  dec_step = nan ("f");
  dec_offset = 0;
  addOption ('t', NULL, 1,
	     "target coordinates type (t for TAR_RA and TAR_DEC, b for RASC and DECL)");
  addOption ('f', NULL, 1, "select images with given flip (0 or 1)");
  addOption ('r', NULL, 1,
	     "step size for mnt_ax0; if specified, HA value is taken from mnt_ax0");
  addOption ('R', NULL, 1, "ra offset in raw counts");
  addOption ('d', NULL, 1,
	     "step size for mnt_ax1; if specified, DEC value is taken from mnt_ax1");
  addOption ('D', NULL, 1, "dec offset in raw counts");
}

TPM::~TPM (void)
{
  filenames.clear ();
}

int
TPM::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 't':
      switch (*optarg)
	{
	case 't':
	  tarCorType = TARGET;
	  break;
	case 'b':
	  tarCorType = BEST;
	  break;
	case 'm':
	  tarCorType = MOUNT;
	  break;
	default:
	  std::
	    cerr << "Invalit coordinates type (" << *optarg <<
	    "), expected t or b" << std::endl;
	  return -1;
	}
      break;
    case 'f':
      if (!strcmp (optarg, "1"))
	selFlip = 1;
      else if (!strcmp (optarg, "0"))
	selFlip = 0;
      else
	{
	  std::
	    cout << "You entered invalid flip, please select either 0 or 1" <<
	    std::endl;
	  help ();
	  return -1;
	}
      break;
    case 'r':
      ra_step = atof (optarg);
      break;
    case 'R':
      ra_offset = atof (optarg);
      break;
    case 'd':
      dec_step = atof (optarg);
      break;
    case 'D':
      dec_offset = atof (optarg);
      break;
    default:
      return Rts2CliApp::processOption (in_opt);
    }
  return 0;
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
  ret = Rts2CliApp::init ();
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
TPM::doProcessing ()
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
  std::cout << "END" << std::endl;
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
  Rts2CliApp::help ();
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
  _os << "RTS2 model from astrometry" << std::endl << ":EQUAT" << std::endl;	// we are observing on equatorial mount
  switch (tarCorType)
    {
    case TARGET:
    case BEST:
      _os << ":J2000" << std::endl;
      break;
    case MOUNT:
      // mount is geocentric
      break;
    }
  _os << ":NODA" << std::endl;	// don't know
  switch (tarCorType)
    {
    case TARGET:
    case BEST:
      _os << " " << LibnovaDeg90 (obs.lat) << " 2000 1 01" << std::endl;	// we have J2000, not refracted coordinates from mount
      break;
    case MOUNT:
      _os << " " << LibnovaDeg90 (obs.lat)
	<< " " << image->getStartYearString ()
	<< " " << image->getStartMonthString ()
	<< " " << image->getStartDayString () << " 20 1000 60" << std::endl;
      break;
    }
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
  int imageFlip;
  double aux0;
  double aux1;

  int ret;

  ret = image->getCoordAstrometry (actual);
  if (ret)
    return ret;
  switch (tarCorType)
    {
    case TARGET:
      ret = image->getCoordTarget (target);
      break;
    case BEST:
      ret = image->getCoordBest (target);
      break;
    case MOUNT:
      ret = image->getCoordMount (target);
      break;
    }
  if (ret)
    return ret;

  ret = image->getValue ("CTIME", ct);
  if (ret)
    return ret;

  ret = image->getValue ("EXPOSURE", expo);
  if (ret)
    return ret;

  imageFlip = image->getMountFlip ();
  // don't process images with invalid flip
  if (selFlip != -1 && imageFlip != selFlip)
    return 0;
  ret = image->getValue ("MNT_AX1", aux1);
  if (ret)
    aux1 = -2;
  ct = (time_t) (ct + expo / 2);

  JD = ln_get_julian_from_timet (&ct);
  mean_sidereal =
    ln_range_degrees (15 * ln_get_apparent_sidereal_time (JD) + obs.lng);

  if (!isnan (ra_step))
    {
      ret = image->getValue ("MNT_AX0", aux0);
      if (ret)
	return ret;
      actual.
	setRa (ln_range_degrees
	       (mean_sidereal - (aux0 - ra_offset) / ra_step));
    }
  if (!isnan (dec_step))
    {
      ret = image->getValue ("MNT_AX1", aux1);
      if (ret)
	return ret;
      actual.setDec ((aux1 - dec_offset) / dec_step);
    }

  LibnovaHaM lst (mean_sidereal);

  _os << actual;
  switch (tarCorType)
    {
    case TARGET:
    case BEST:
      _os << " 0 0 2000.0 ";
      break;
    case MOUNT:
      _os << " ";
      // geocentric..
      break;
    }
  _os << target << " " << lst << " " << imageFlip << " " << aux1 << std::endl;
  return 0;
}

int
main (int argc, char **argv)
{
  TPM app = TPM (argc, argv);
  return app.run ();
}

#include "rts2appdbimage.h"
#include "../utils/rts2config.h"

#include <iostream>
#include <iomanip>

#include <list>

#define IMAGEOP_NOOP		0x00
#define IMAGEOP_ADDDATE		0x01
#define IMAGEOP_INSERT		0x02
#define IMAGEOP_TEST		0x04
#define IMAGEOP_COPY		0x08
#define IMAGEOP_MOVE		0x10
#define IMAGEOP_EVAL		0x20
#define IMAGEOP_CREATEWCS	0x40

class Rts2AppImageManip:public Rts2AppDbImage
{
private:
  int operation;

  void printOffset (double x, double y, Rts2Image * image);

  int addDate (Rts2Image * image);
  int insert (Rts2ImageDb * image);
  int testImage (Rts2Image * image);
  int testEval (Rts2Image * image);
  void createWCS (Rts2Image * image);

  double off_x, off_y;

    std::string copy_expr;
    std::string move_expr;
protected:
    virtual int processOption (int in_opt);
  virtual bool doInitDB ();
  virtual int processImage (Rts2ImageDb * image);

  virtual void help ();
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
  std::cout
    << image << std::endl
    << "average " << image->getAverage () << std::endl
    << "stdev " << image->getStdDev () << std::endl
    << "bg_stdev " << image->getBgStdDev () << std::endl
    << "Image XoA and Yoa: [" << image->getXoA ()
    << ":" << image->getYoA () << "]" << std::endl
    << "[XoA:YoA] RA: " << image->getCenterRa ()
    << " DEC: " << image->getCenterDec () << std::endl
    << "FLIP: " << image->getFlip () << std::endl
    << image->getRaDec (image->getXoA (), image->getYoA (), ra, dec)
    << "ROTANG: " << ln_rad_to_deg (image->getRotang ())
    << " (deg) XPLATE: " << image->getXPlate ()
    << " YPLATE: " << image->getYPlate () << std::endl
    << "RA and DEC of [XoA:YoA]: " << ra << ", " << dec << std::endl
    << image->getRaDec (0, 0, ra, dec)
    << "RA and DEC of [0:0]: " << ra << ", " << dec << std::endl
    << image->getRaDec (image->getWidth (), 0, ra, dec)
    << "RA and DEC of [W:0]: " << ra << ", " << dec << std::endl
    << image->getRaDec (0, image->getHeight (), ra, dec)
    << "RA and DEC of [0:H]: " << ra << ", " << dec << std::endl
    << image->getRaDec (image->getWidth (), image->getHeight (), ra, dec)
    << "RA and DEC of [W:H]: " << ra << ", " << dec << std::endl
    << "Rts2Image::getCenterRow " << image->getCenter (x, y, 3) << " " << x
    << ":" << y << std::endl
    << "Expression %b/%t/%i/%c/%f '" << image->
    expandPath (std::string ("%b/%t/%i/%c/%f")) << '\'' << std::
    endl <<
    "Expression $DATE-OBS$/%b/%e/%E/%f/%F/%t/%i/%y/%m/%d/%D/%H/%M/%S/%s.fits '"
    << image->
    expandPath (std::
		string
		("$DATE-OBS$/%b/%e/%E/%f/%F/%t/%i/%y/%m/%d/%D/%H/%M/%S/%s.fits"))
    << '\'' << std::endl;

  printOffset (image->getXoA () + 50, image->getYoA (), image);
  printOffset (image->getXoA (), image->getYoA () + 50, image);
  printOffset (image->getXoA () - 50, image->getYoA (), image);
  printOffset (image->getXoA (), image->getYoA () - 50, image);

  printOffset (152, 150, image);

  return 0;
}

int
Rts2AppImageManip::testEval (Rts2Image * image)
{
  float value, error;

  image->evalAF (&value, &error);

  std::cout << "value: " << value << " error: " << error << std::endl;

  return 0;
}

void
Rts2AppImageManip::createWCS (Rts2Image * image)
{
  int ret = image->createWCS (off_x, off_y);

  if (ret)
    std::cerr << "Create WCS returned with error " << ret << std::endl;
}

int
Rts2AppImageManip::processOption (int in_opt)
{
  char *off_sep;
  switch (in_opt)
    {
    case 'c':
      operation |= IMAGEOP_COPY;
      copy_expr = optarg;
      break;
    case 'd':
      operation |= IMAGEOP_ADDDATE;
      break;
    case 'i':
      operation |= IMAGEOP_INSERT;
      break;
    case 'm':
      operation |= IMAGEOP_MOVE;
      move_expr = optarg;
      break;
    case 't':
      operation |= IMAGEOP_TEST;
      break;
    case 'e':
      operation |= IMAGEOP_EVAL;
      break;
    case 'w':
      operation |= IMAGEOP_CREATEWCS;
      break;
    case 'o':
      off_sep = index (optarg, ':');
      if (off_sep)
	{
	  *off_sep = '\0';
	  off_sep++;
	  off_x = atof (optarg);
	  off_y = atof (off_sep);
	}
      else
	{
	  off_x = atof (optarg);
	  off_y = off_x;
	}
      break;
    default:
      return Rts2AppDbImage::processOption (in_opt);
    }
  return 0;
}

bool Rts2AppImageManip::doInitDB ()
{
  return (operation & IMAGEOP_MOVE) || (operation & IMAGEOP_INSERT);
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
  if (operation & IMAGEOP_COPY)
    image->copyImageExpand (copy_expr);
  if (operation & IMAGEOP_MOVE)
    image->renameImageExpand (move_expr);
  if (operation & IMAGEOP_EVAL)
    testEval (image);
  if (operation & IMAGEOP_CREATEWCS)
    createWCS (image);
  return 0;
}

void
Rts2AppImageManip::help ()
{
  Rts2AppDbImage::help ();
  std::cout << "Examples:" << std::endl
    <<
    "  rts2-image -w 123.fits                 .. write WCS to file 123, based on information stored by RTS2 in the file"
    << std::
    endl <<
    "  rts2-image -w -o 20.12:10.56 123.fits  .. same as above, but add X offset of 20.12 pixels and Y offset of 10.56 pixels to WCS"
    << std::endl;
}

Rts2AppImageManip::Rts2AppImageManip (int in_argc, char **in_argv):
Rts2AppDbImage (in_argc, in_argv)
{
  operation = IMAGEOP_NOOP;

  off_x = 0;
  off_y = 0;

  addOption ('c', "copy", 1,
	     "copy image(s) to path expression given as argument");
  addOption ('d', "add-date", 0, "add DATE-OBS to image header");
  addOption ('i', "insert", 0, "insert/update image(s) in the database");
  addOption ('m', "move", 1,
	     "move image(s) to path expression given as argument");
  addOption ('e', "eval", 0, "image evaluation for AF purpose");
  addOption ('t', "test", 0, "test various image routines");
  addOption ('w', "wcs", 0,
	     "write WCS to FITS file, based on the RTS2 informations recorded in fits header");
  addOption ('o', "offsets", 1,
	     "X and Y offsets in pixels aplied to WCS information before WCS is written to the file. X and Y offsets must be separated by ':'");
}

int
main (int argc, char **argv)
{
  Rts2AppImageManip app = Rts2AppImageManip (argc, argv);
  int ret = app.init ();
  if (ret)
    {
      return ret;
    }
  return app.run ();
}

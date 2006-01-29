#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/target.h"
#include "../utilsdb/rts2obsset.h"
#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"

#include "rts2simbadtarget.h"

#include <iostream>
#include <iomanip>
#include <list>
#include <stdlib.h>
#include <sstream>

class Rts2NewTarget:public Rts2AppDb
{
private:
  Target * target;
  struct ln_lnlat_posn *obs;
protected:
  int askForInt (const char *desc, int &val);
  int askForString (const char *desc, std::string & val);
  int askForObject (const char *desc, Target ** retTarget);
public:
    Rts2NewTarget (int argc, char **argv);
    virtual ~ Rts2NewTarget (void);

  virtual int processOption (int in_opt);

  virtual int init ();
  virtual int run ();
};

int
Rts2NewTarget::askForInt (const char *desc, int &val)
{
  char temp[200];
  while (1)
    {
      std::cout << desc << " [" << val << "]: ";
      std::cin.getline (temp, 200);
      std::string str_val (temp);
      if (str_val.empty ())
	break;
      std::istringstream is (str_val);
      is >> val;
      if (!is.fail ())
	break;
      std::cout << "Invalid number!" << std::endl;
      std::cin.clear ();
      std::cin.ignore (2000, '\n');
    }
  std::cout << desc << ": " << val << std::endl;
  return 0;
}

int
Rts2NewTarget::askForString (const char *desc, std::string & val)
{
  char temp[201];
  while (1)
    {
      std::cout << desc << " [" << val << "]: ";
      std::cin.getline (temp, 200);
      if (!std::cin.fail ())
	break;
      std::cout << "Invalid string!" << std::endl;
      std::cin.clear ();
      std::cin.ignore (2000, '\n');
    }
  val = std::string (temp);
  std::cout << desc << ": " << val << std::endl;
  return 0;
}

int
Rts2NewTarget::askForObject (const char *desc, Target ** retTarget)
{
  std::string obj_text ("");
  int ret;
  ret = askForString (desc, obj_text);
  if (ret)
    return ret;
  std::istringstream is (obj_text);
  // now try to parse it to ra dec..
  double ra = 0;
  double dec = 0;
  int dec_sign = 1;
  int step = 1;
  enum
  { NOT_GET, RA, DEC } state;
  bool ra_six = false;

  state = NOT_GET;
  while (1)
    {
      std::string next;
      is >> next;
      if (is.fail ())
	break;
      std::istringstream next_num (next);
      int next_c = next_num.peek ();
      if (next_c == '+' || next_c == '-')
	{
	  if (state != RA)
	    return -1;
	  state = DEC;
	  step = 1;
	  if (next_c == '-')
	    {
	      dec_sign = -1;
	      next_num.get ();
	    }
	}
      double val;
      next_num >> val;
      if (next_num.fail ())
	break;
      switch (state)
	{
	case NOT_GET:
	  state = RA;
	case RA:
	  ra += (val / step);
	  if (step > 1)
	    ra_six = true;
	  step *= 60;
	  break;
	case DEC:
	  dec += (val / step);
	  step *= 60;
	  break;
	}
    }
  if (state == DEC)
    {
      // convert ra from hours to degs
      // if ra is one number, then it's already in degrees
      double obj_ra = ra;
      if (ra_six)
	obj_ra *= 15.0;
      double obj_dec = dec_sign * dec;
      // set name..
      ConstTarget *constTarget = new ConstTarget ();
      constTarget->setPosition (obj_ra, obj_dec);
      std::ostringstream os;
      os << "S " << LibnovaRaComp (obj_ra) << LibnovaDeg90Comp (obj_dec);
      constTarget->setTargetName (os.str ().c_str ());
      *retTarget = constTarget;
      return 0;
    }
  // try to get target from SIMBAD
  *retTarget = new Rts2SimbadTarget (obj_text.c_str ());
  ret = (*retTarget)->load ();
  if (ret)
    {
      delete *retTarget;
      return -1;
    }
  return 0;
}

Rts2NewTarget::Rts2NewTarget (int in_argc, char **in_argv):
Rts2AppDb (in_argc, in_argv)
{
  target = NULL;
  obs = NULL;
}

Rts2NewTarget::~Rts2NewTarget ()
{
  delete target;
}

int
Rts2NewTarget::processOption (int in_opt)
{
  switch (in_opt)
    {
    default:
      return Rts2AppDb::processOption (in_opt);
    }
  return 0;
}

int
Rts2NewTarget::init ()
{
  int ret;

  ret = Rts2AppDb::init ();
  if (ret)
    return ret;

  Rts2Config *config;
  config = Rts2Config::instance ();

  if (!obs)
    {
      obs = config->getObserver ();
    }

  return 0;
}

int
Rts2NewTarget::run ()
{
  int n_tar_id = 0;
  int ret;
  Target *new_tar;
  std::string target_name;
  // ask for target ID..
  std::cout << "Default values are written at [].." << std::endl;
  ret = askForObject ("Target name, RA&DEC or anything else", &new_tar);
  if (ret)
    return ret;
  askForInt ("Target ID", n_tar_id);

  std::cout << new_tar;

  new_tar->setTargetType (TYPE_OPORTUNITY);
  if (n_tar_id > 0)
    ret = new_tar->save (n_tar_id);
  else
    ret = new_tar->save ();

  delete new_tar;
  if (ret)
    std::cerr << "Error when saving target." << std::endl;
  return 0;
}

int
main (int argc, char **argv)
{
  int ret;
  Rts2NewTarget *app;
  app = new Rts2NewTarget (argc, argv);
  ret = app->init ();
  if (ret)
    return 1;
  app->run ();
  delete app;
}

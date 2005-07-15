#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/target.h"
#include "../utils/rts2config.h"

#include <iostream>
#include <list>
#include <stdlib.h>

class Rts2TargetInfo:public Rts2AppDb
{
private:
  std::list < int >targets;
  Target *target;
  struct ln_lnlat_posn *obs;
  int printTargetInfo ();
public:
    Rts2TargetInfo (int argc, char **argv);
    virtual ~ Rts2TargetInfo (void);

  virtual int processArgs (const char *arg);
  virtual int run ();
};

Rts2TargetInfo::Rts2TargetInfo (int argc, char **argv):
Rts2AppDb (argc, argv)
{
  obs = NULL;
}

Rts2TargetInfo::~Rts2TargetInfo ()
{
}

int
Rts2TargetInfo::processArgs (const char *arg)
{
  // try to create that target..
  int tar_id;
  char *test;
  tar_id = strtol (arg, &test, 10);
  if (*test)
    {
      std::cerr << "Invalid target id: " << arg << std::endl;
      return -1;
    }
  targets.push_back (tar_id);
  return 0;
}

int
Rts2TargetInfo::printTargetInfo ()
{
  struct ln_hms hms;
  struct ln_dms dms, dms2;
  struct ln_equ_posn pos;
  struct ln_hrz_posn hrz;
  double ha;
  time_t now, last;
  std::cout << "======================" << std::endl;
  std::cout << "Target ID: " << target->getTargetID () << std::endl;
  target->getPosition (&pos);
  ln_deg_to_hms (pos.ra, &hms);
  ln_deg_to_dms (pos.dec, &dms);
  std::cout << "Actual position: RA " << hms.hours << ":" << hms.
    minutes << ":" << hms.seconds << "(" << pos.
    ra << ") (2000)" << " Dec: " << dms.degrees << ":" << dms.
    minutes << ":" << dms.seconds << "(" << pos.
    dec << ") (2000)" << std::endl;
  std::cout << "Rise after " << target->
    secToObjectRise () << " seconds" << std::endl;
  std::cout << "Transit after " << target->
    secToObjectMeridianPass () << " seconds" << std::endl;
  std::cout << "Set after " << target->
    secToObjectSet () << " seconds" << std::endl;
  ha = target->getHourAngle ();
  ln_deg_to_hms (ha * 15.0, &hms);
  std::cout << "Hour angle " << hms.hours << ":" << hms.minutes << ":" << hms.
    seconds << " (" << ha << ")" << std::endl;
  target->getAltAz (&hrz);
  ln_deg_to_dms (hrz.alt, &dms);
  ln_deg_to_dms (hrz.az, &dms2);
  std::cout << "Alt: " << dms.degrees << ":" << dms.minutes << ":"
    << dms.seconds << " (" << hrz.alt << ") Azimut: "
    << dms2.degrees << ":" << dms2.minutes << ":" << dms2.seconds
    << " (" << hrz.az << ")" << std::endl;
  time (&now);
  std::cout << "Zenit distance " << target->getAzDistance () << std::endl;
  last = now - 86400;
  std::cout << "Observations in last few hours: " << target->getNumObs (&last,
									&now)
    << std::endl;
  std::cout << "Bonus: " << target->getBonus () << std::endl;
}

int
Rts2TargetInfo::run ()
{
  std::list < int >::iterator tar_iter;

  if (!obs)
    {
      Rts2Config *config;
      config = Rts2Config::instance ();
      obs = config->getObserver ();
    }

  for (tar_iter = targets.begin (); tar_iter != targets.end (); tar_iter++)
    {
      int tar_id;
      tar_id = *tar_iter;
      target = createTarget (tar_id, obs);
      if (!target)
	{
	  std::cerr << "Cannot find target with id: " << tar_id << std::endl;
	  continue;
	}
      printTargetInfo ();
      delete target;
    }
}

int
main (int argc, char **argv)
{
  int ret;
  Rts2TargetInfo *app;
  app = new Rts2TargetInfo (argc, argv);
  ret = app->init ();
  if (ret)
    return 1;
  app->run ();
  delete app;
}

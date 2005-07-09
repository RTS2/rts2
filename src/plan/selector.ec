#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
/*!
 * Used for generating new plan entries.
 *
 * @author petr
 */

#include "../utils/rts2devclient.h"
#include "../utilsdb/rts2devicedb.h"
#include "../utils/rts2event.h"
#include "../utils/rts2command.h"
#include "../utils/objectcheck.h"
#include "../utilsdb/target.h"
#include "status.h"

#include <signal.h>
#include <libnova/libnova.h>

class Rts2Selector:public Rts2DeviceDb
{
private:
  std::list < Target *> possibleTargets;
  void considerTarget (int consider_tar_id);
  void findNewTargets ();
  int selectNextNight ();
  int selectFlats ();
  int selectDarks ();
  ObjectCheck *checker;
  struct ln_lnlat_posn *observer;
public:
  Rts2Selector (int argc, char **argv);
    virtual ~ Rts2Selector (void);
  virtual int init ();
  virtual void postEvent (Rts2Event *event);
  int updateNext ();
  int selectNext (); // return next observation..
  virtual int changeMasterState (int new_state);
};

Rts2Selector::Rts2Selector (int argc, char **argv):Rts2DeviceDb (argc, argv, DEVICE_TYPE_SELECTOR, 5562,
	    "SEL")
{
  // add read config..when we will get config
  checker = new ObjectCheck ("/etc/rts2/horizont");
}

Rts2Selector::~Rts2Selector (void)
{
  std::list <Target *>::iterator target_list;
  for (target_list = possibleTargets.begin (); target_list !=
  possibleTargets.end (); target_list++)
  {
    Target *tar = *target_list;
    delete tar;
  }
  possibleTargets.clear ();
  delete checker;
}

int
Rts2Selector::init ()
{
  int ret;
  ret = Rts2DeviceDb::init ();
  if (ret)
    return ret;

  Rts2Config *config;
  config = Rts2Config::instance ();
  observer = config->getObserver ();
}

void
Rts2Selector::postEvent (Rts2Event *event)
{
  switch (event->getType ())
  {
    case EVENT_IMAGE_OK:
      updateNext ();
      break;
  }
  Rts2Device::postEvent (event);
}

int
Rts2Selector::updateNext ()
{
  int next_id;
  Rts2Conn * exec;
  next_id = selectNext ();
  if (next_id > 0)
  {
    exec = getOpenConnection ("EXEC");
    if (exec)
    {
      exec->queCommand (new Rts2CommandExecNext (this, next_id));
    }
    return 0;
  }
  return -1;
}

int
Rts2Selector::selectNext ()
{
  int masterState = getMasterState ();
  // take care of state - select to make darks when we are able to
  // make darks.
  switch (masterState)
  {
    case SERVERD_NIGHT:
      return selectNextNight ();
      break;
    case SERVERD_DUSK:
      return selectDarks ();
      break;
    case SERVERD_DAWN:
      return selectFlats ();
      break;
    case SERVERD_MORNING:
      return selectDarks ();
      break;
  }
  return -1; // we don't have any target to take observation..
}

void
Rts2Selector::considerTarget (int consider_tar_id)
{
  std::list < Target* >::iterator target_list;
  Target *newTar;
  int ret;

  for (target_list = possibleTargets.begin (); target_list != possibleTargets.end ();
  target_list++)
  {
    Target *tar = *target_list;
    if (tar->getTargetID () == consider_tar_id)
    {
      // we already have that target amongs possible one..
      return;
    }
  }
  // add us..
  newTar = createTarget (consider_tar_id, observer);
  ret = newTar->considerForObserving (checker, ln_get_julian_from_sys ());
  if (ret)
    return;
  // add to possible targets..
  possibleTargets.push_back (newTar);
}

void
Rts2Selector::findNewTargets ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int consider_tar_id;
  EXEC SQL END DECLARE SECTION;
  
  EXEC SQL DECLARE findNewTargets CURSOR FOR
    SELECT
      tar_id
    FROM
      targets
    WHERE
      tar_priority + tar_bonus > 0;
  EXEC SQL OPEN findNewTargets;
  while (sqlca.sqlcode == 0)
  {
    EXEC SQL FETCH next FROM findNewTargets INTO :consider_tar_id;
    // try to find us in considered targets..
    considerTarget (consider_tar_id);
  }
  if (sqlca.sqlcode != ECPG_NOT_FOUND)
  {
    // some DB error..strange, let's get out
    syslog (LOG_DEBUG, "findNewTargets DB error: %s", sqlca.sqlerrm);
    exit (1);
  }
  EXEC SQL CLOSE findNewTargets;
};

int
Rts2Selector::selectNextNight ()
{
  std::list < Target *>::iterator target_list;
  // search for new observation targets..
  int maxId;
  int maxBonus = -1;
  int tar_bonus;
  findNewTargets ();
  // find target with highest bonus and move us to it..
  for (target_list = possibleTargets.begin (); target_list !=
  possibleTargets.end (); target_list++)
  {
    Target *tar = *target_list;
    tar_bonus = tar->getBonus ();
    if (tar_bonus > maxBonus)
    {
      maxId = tar->getTargetID ();
      maxBonus = tar_bonus;
    }
  }
  if (maxBonus < 0)
  {
    // we don't get a targets..so take some darks..
    return selectDarks ();
  }
}

int
Rts2Selector::selectFlats ()
{
  return TARGET_FLAT;
}

int
Rts2Selector::selectDarks ()
{
  return TARGET_DARK;
}

int
Rts2Selector::changeMasterState (int new_state)
{
  return updateNext ();
}

Rts2Selector *selector;

void
killSignal (int sig)
{
  if (selector)
    delete selector;
  exit (0);
}

int
main (int argc, char **argv)
{
  int ret;
  selector = new Rts2Selector (argc, argv);
  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);
  ret = selector->init ();
  if (ret)
    return ret;
  selector->run ();
  delete selector;
  return 0;
}

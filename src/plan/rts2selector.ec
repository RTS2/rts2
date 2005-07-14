#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
/*!
 * Used for generating new plan entries.
 *
 * @author petr
 */
 
#include "rts2selector.h"
#include "status.h"

#include <libnova/libnova.h>

class Rts2DevClientTelescopeSel: public Rts2DevClientTelescope
{
public:
  Rts2DevClientTelescopeSel (Rts2Conn * in_connection);
  virtual void stateChanged (Rts2ServerState * state);
};

Rts2DevClientTelescopeSel::Rts2DevClientTelescopeSel (Rts2Conn * in_connection) : Rts2DevClientTelescope (in_connection)
{
}

void
Rts2DevClientTelescopeSel::stateChanged (Rts2ServerState * state)
{
  if (state->isName ("telescope"))
  {
    if ((state->value & TEL_MASK_MOVING) == TEL_OBSERVING
      || (state->value & TEL_MASK_MOVING) == TEL_PARKING)
      {
        connection->getMaster ()->postEvent (new Rts2Event (EVENT_IMAGE_OK));
      }
  }
  Rts2DevClientTelescope::stateChanged (state);
}

Rts2Selector::Rts2Selector (int argc, char **argv):Rts2DeviceDb (argc, argv, DEVICE_TYPE_SELECTOR, 5562,
	    "SEL")
{
  checker = NULL;
  time (&last_selected);
  next_id = -1;
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
  if (checker)
    delete checker;
}

int
Rts2Selector::init ()
{
  int ret;
  char horizontFile[250];

  ret = Rts2DeviceDb::init ();
  if (ret)
    return ret;

  Rts2Config *config;
  config = Rts2Config::instance ();
  observer = config->getObserver ();

  // add read config..when we will get config
  config->getString ("observatory", "horizont", horizontFile, 250);
  checker = new ObjectCheck (horizontFile);
  return 0;
}

int
Rts2Selector::idle ()
{
  time_t now;
  time (&now);
  if (now > last_selected + 500)
  {
    updateNext ();
    time (&last_selected);
  }
  return Rts2DeviceDb::idle ();
}

Rts2DevClient *
Rts2Selector::createOtherType (Rts2Conn * conn, int other_device_type)
{
  Rts2DevClient *ret;
  switch (other_device_type)
  {
    case DEVICE_TYPE_MOUNT:
      return new Rts2DevClientTelescopeSel (conn);
    case DEVICE_TYPE_EXECUTOR:
      ret = Rts2DeviceDb::createOtherType (conn, other_device_type);
      updateNext ();
      if (next_id > 0)
        conn->queCommand (new Rts2CommandExecNext (this, next_id));
      return ret;
    default:
      return Rts2DeviceDb::createOtherType (conn, other_device_type);
  }
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
  return TARGET_FLAT; // we don't have any target to take observation..
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
  syslog (LOG_DEBUG, "considerForObserving tar_id: %i ret: %i", newTar->getTargetID (), ret);
  if (ret)
  {
    delete newTar;
    return;
  }
  // add to possible targets..
  possibleTargets.push_back (newTar);
}

void
Rts2Selector::findNewTargets ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int consider_tar_id;
  EXEC SQL END DECLARE SECTION;

  // drop old priorities..

  EXEC SQL
  UPDATE
    targets
  SET
    tar_bonus = 0,
    tar_bonus_time = NULL
  WHERE
    tar_bonus_time > now ();
  EXEC SQL COMMIT;

  EXEC SQL DECLARE findnewtargets CURSOR WITH HOLD FOR
    SELECT
      tar_id
    FROM
      targets
    WHERE
        tar_enabled = true
      AND tar_priority + tar_bonus > 0;
  if (sqlca.sqlcode)
  {
    syslog (LOG_ERR, "findNewTargets: %s", sqlca.sqlerrm.sqlerrmc);
    return;
  }

  EXEC SQL OPEN findnewtargets;
  if (sqlca.sqlcode)
  {
    syslog (LOG_ERR, "findNewTargets: %s", sqlca.sqlerrm.sqlerrmc);
    EXEC SQL ROLLBACK;
    return;
  }
  while (1)
  {
    EXEC SQL FETCH next FROM findnewtargets INTO :consider_tar_id;
    if (sqlca.sqlcode)
      break;
    // try to find us in considered targets..
    considerTarget (consider_tar_id);
  }
  if (sqlca.sqlcode != ECPG_NOT_FOUND)
  {
    // some DB error..strange, let's get out
    syslog (LOG_DEBUG, "findNewTargets DB error: %s", sqlca.sqlerrm.sqlerrmc);
    exit (1);
  }
  EXEC SQL CLOSE findnewtargets;
};

int
Rts2Selector::selectNextNight ()
{
  std::list < Target *>::iterator target_list;
  // search for new observation targets..
  int maxId;
  float maxBonus = -1;
  float tar_bonus;
  findNewTargets ();
  // find target with highest bonus and move us to it..
  for (target_list = possibleTargets.begin (); target_list !=
    possibleTargets.end (); target_list++)
  {
    Target *tar = *target_list;
    tar_bonus = tar->getBonus ();
    syslog (LOG_DEBUG, "bonus: %i %f", tar->getTargetID (), tar_bonus);
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
  return maxId;
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
  int ret;
  return updateNext ();
}

#include "rts2targetscr.h"

Rts2TargetScr::Rts2TargetScr (Rts2ScriptExec * in_master):Rts2Target ()
{
  master = in_master;
}

Rts2TargetScr::~Rts2TargetScr (void)
{
}

int
Rts2TargetScr::getScript (const char *device_name, char *buf)
{
  Rts2ScriptForDevice *script =
    master->findScript (std::string (device_name));
  if (script)
    {
      strncpy (buf, script->getScript (), MAX_COMMAND_LENGTH);
      return 0;
    }

  *buf = '\0';
  return -1;
}

int
Rts2TargetScr::getPosition (struct ln_equ_posn *pos, double JD)
{
  return master->getPosition (pos, JD);
}

int
Rts2TargetScr::getObsTargetID ()
{
  return -1;
}

void
Rts2TargetScr::acqusitionStart ()
{
}

int
Rts2TargetScr::isAcquired ()
{
  return 1;
}

int
Rts2TargetScr::setNextObservable (time_t * time_ch)
{
  return 0;
}

void
Rts2TargetScr::setTargetBonus (float new_bonus, time_t * new_time)
{

}

int
Rts2TargetScr::save (bool overwrite)
{
  return 0;
}

int
Rts2TargetScr::save (bool overwrite, int tar_id)
{
  return 0;
}

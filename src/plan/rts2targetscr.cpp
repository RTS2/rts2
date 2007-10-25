#include "rts2targetscr.h"

Rts2TargetScr::Rts2TargetScr (Rts2ScriptExec * in_master):Rts2Target ()
{
	master = in_master;
	target_id = 1;
}


Rts2TargetScr::~Rts2TargetScr (void)
{
}


int
Rts2TargetScr::getScript (const char *device_name, std::string & buf)
{
	Rts2ScriptForDevice *script =
		master->findScript (std::string (device_name));
	if (script)
	{
		buf = std::string (script->getScript ());
		return 0;
	}

	buf = std::string ("");
	return -1;
}


int
Rts2TargetScr::getPosition (struct ln_equ_posn *pos, double JD)
{
	return master->getPosition (pos, JD);
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


moveType
Rts2TargetScr::startSlew (struct ln_equ_posn * position)
{
	position->ra = position->dec = 0;
	return OBS_MOVE_FAILED;
}


int
Rts2TargetScr::startObservation (Rts2Block * in_master)
{
	return -1;
}


void
Rts2TargetScr::writeToImage (Rts2Image * image, double JD)
{
}

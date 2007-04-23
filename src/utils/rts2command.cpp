#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <iostream>
#include <fstream>

#include "rts2command.h"

using namespace std;

Rts2Command::Rts2Command (Rts2Block * in_owner)
{
  owner = in_owner;
  text = NULL;
  commandCond = NO_COND;
}

Rts2Command::Rts2Command (Rts2Block * in_owner, char *in_text)
{
  owner = in_owner;
  setCommand (in_text);
  commandCond = NO_COND;
}

int
Rts2Command::setCommand (char *in_text)
{
  text = new char[strlen (in_text) + 1];
  strcpy (text, in_text);
  return 0;
}

Rts2Command::~Rts2Command (void)
{
  delete[]text;
}

int
Rts2Command::send ()
{
  return connection->send (text);
}

int
Rts2Command::commandReturn (int status)
{
  if (connection)
    connection->commandReturn (this, status);
  switch (status)
    {
    case 0:
      return commandReturnOK ();
    case 1:
      return commandReturnQued ();
    default:
      return commandReturnFailed (status);
    }
}

int
Rts2Command::commandReturnOK ()
{
  return -1;
}

int
Rts2Command::commandReturnQued ()
{
  return commandReturnOK ();
}

int
Rts2Command::commandReturnFailed (int status)
{
  return -1;
}

/**************************************************************
 *
 * Rts2CommandSendKey implementation.
 *
 *************************************************************/

Rts2CommandSendKey::Rts2CommandSendKey (Rts2Block * in_master, int in_key):Rts2Command
  (in_master)
{
  key = in_key;
}

int
Rts2CommandSendKey::send ()
{
  char *command;
  asprintf (&command, "auth %i %i",
	    owner->getCentraldConn ()->getCentraldId (), key);
  setCommand (command);
  free (command);
  return Rts2Command::send ();
}

/**************************************************************
 * 
 * Rts2CommandAuthorize command
 *
 *************************************************************/

Rts2CommandAuthorize::Rts2CommandAuthorize (Rts2Block * in_master, const char *device_name):Rts2Command
  (in_master)
{
  char *
    command;
  asprintf (&command, "key %s", device_name);
  setCommand (command);
  free (command);
}

/**************************************************************
 *
 * Rts2 device commands
 * 
 *************************************************************/

Rts2CommandCameraSettings::Rts2CommandCameraSettings (Rts2DevClientCamera * in_camera):Rts2Command (in_camera->
	     getMaster
	     ())
{
  camera = in_camera;
}

int
Rts2CommandCameraSettings::commandReturnOK ()
{
  camera->settingsOK ();
  return Rts2Command::commandReturnOK ();
}

int
Rts2CommandCameraSettings::commandReturnFailed (int status)
{
  camera->settingsFailed (status);
  return Rts2Command::commandReturnFailed (status);
}


Rts2CommandBinning::Rts2CommandBinning (Rts2DevClientCamera * in_camera, int binning_v, int binning_h):
Rts2CommandCameraSettings (in_camera)
{
  char *command;
  asprintf (&command, "binning 0 %i %i", binning_v, binning_h);
  setCommand (command);
  free (command);
}

Rts2CommandExposure::Rts2CommandExposure (Rts2Block * in_master,
					  Rts2DevClientCamera * in_camera,
					  int chip,
					  exposureType exp_type,
					  float exp_time, bool readout):
Rts2Command (in_master)
{
  char *command;
  if (readout)
    asprintf (&command, "readout_exposure %i %i %f", chip,
	      (exp_type == EXP_LIGHT ? 1 : 0), exp_time);
  else
    asprintf (&command, "expose %i %i %f", chip,
	      (exp_type == EXP_LIGHT ? 1 : 0), exp_time);
  setCommand (command);
  free (command);
  camera = in_camera;
  if (exp_type == EXP_LIGHT)
    commandCond = NO_EXPOSURE_NO_MOVE;
  else
    commandCond = NO_EXPOSURE_MOVE;
}

int
Rts2CommandExposure::commandReturnFailed (int status)
{
  camera->exposureFailed (status);
  return Rts2Command::commandReturnFailed (status);
}

Rts2CommandReadout::Rts2CommandReadout (Rts2Block * in_master, int chip):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "readout %i", chip);
  setCommand (command);
  free (command);
}

void
Rts2CommandFilter::setCommandFilter (int filter)
{
  char *command;
  asprintf (&command, "filter %i", filter);
  setCommand (command);
  free (command);
}

Rts2CommandFilter::Rts2CommandFilter (Rts2DevClientCamera * in_camera,
				      int filter):
Rts2Command (in_camera->getMaster ())
{
  camera = in_camera;
  phot = NULL;
  filterCli = NULL;
  setCommandFilter (filter);
}

Rts2CommandFilter::Rts2CommandFilter (Rts2DevClientPhot * in_phot,
				      int filter):
Rts2Command (in_phot->getMaster ())
{
  camera = NULL;
  phot = in_phot;
  filterCli = NULL;
  setCommandFilter (filter);
}

Rts2CommandFilter::Rts2CommandFilter (Rts2DevClientFilter * in_filter,
				      int filter):
Rts2Command (in_filter->getMaster ())
{
  camera = NULL;
  phot = NULL;
  filterCli = in_filter;
  setCommandFilter (filter);
}

int
Rts2CommandFilter::commandReturnOK ()
{
  if (camera)
    camera->filterOK ();
  if (phot)
    phot->filterOK ();
  if (filterCli)
    filterCli->filterOK ();
  return Rts2Command::commandReturnOK ();
}

int
Rts2CommandFilter::commandReturnFailed (int status)
{
  if (camera)
    camera->filterFailed (status);
  if (phot)
    phot->filterMoveFailed (status);
  if (filterCli)
    filterCli->filterMoveFailed (status);
  return Rts2Command::commandReturnFailed (status);
}

Rts2CommandBox::Rts2CommandBox (Rts2DevClientCamera * in_camera, int chip, int x, int y, int w, int h):Rts2CommandCameraSettings
  (in_camera)
{
  char *
    command;
  asprintf (&command, "box %i %i %i %i %i", chip, x, y, w, h);
  setCommand (command);
  free (command);
}

Rts2CommandCenter::Rts2CommandCenter (Rts2DevClientCamera * in_camera, int chip, int width = -1, int height = -1):Rts2CommandCameraSettings
  (in_camera)
{
  char *
    command;
  asprintf (&command, "center %i %i %i", chip, width, height);
  setCommand (command);
  free (command);
}

Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * in_client,
						std::string in_valName,
						char op,
						std::string in_operand):
Rts2Command (in_client->getMaster ())
{
  char *command;
  client = in_client;
  asprintf (&command, PROTO_SET_VALUE " %s %c %s", in_valName.c_str (), op,
	    in_operand.c_str ());
  setCommand (command);
  free (command);
}

int
Rts2CommandChangeValue::commandReturnOK ()
{
  client->settingsOK ();
  return Rts2Command::commandReturnOK ();
}

int
Rts2CommandChangeValue::commandReturnFailed (int status)
{
  client->settingsFailed (status);
  return Rts2Command::commandReturnFailed (status);
}

Rts2CommandMove::Rts2CommandMove (Rts2Block * in_master, Rts2DevClientTelescope * in_tel):
Rts2Command (in_master)
{
  tel = in_tel;
}

Rts2CommandMove::Rts2CommandMove (Rts2Block * in_master, Rts2DevClientTelescope * in_tel, double ra, double dec):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "move %lf %lf", ra, dec);
  setCommand (command);
  free (command);
  tel = in_tel;
}

int
Rts2CommandMove::commandReturnFailed (int status)
{
  tel->moveFailed (status);
  return Rts2Command::commandReturnFailed (status);
}

Rts2CommandMoveUnmodelled::Rts2CommandMoveUnmodelled (Rts2Block * in_master, Rts2DevClientTelescope * in_tel, double ra, double dec):
Rts2CommandMove (in_master, in_tel)
{
  char *command;
  asprintf (&command, "move_not_model %lf %lf", ra, dec);
  setCommand (command);
  free (command);
}

Rts2CommandMoveFixed::Rts2CommandMoveFixed (Rts2Block * in_master,
					    Rts2DevClientTelescope * in_tel,
					    double ra, double dec):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "fixed %lf %lf", ra, dec);
  setCommand (command);
  free (command);
  tel = in_tel;
}

int
Rts2CommandMoveFixed::commandReturnFailed (int status)
{
  tel->moveFailed (status);
  return Rts2Command::commandReturnFailed (status);
}

Rts2CommandResyncMove::Rts2CommandResyncMove (Rts2Block * in_master, Rts2DevClientTelescope * in_tel, double ra, double dec):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "resync %lf %lf", ra, dec);
  setCommand (command);
  free (command);
  tel = in_tel;
}

int
Rts2CommandResyncMove::commandReturnFailed (int status)
{
  tel->moveFailed (status);
  return Rts2Command::commandReturnFailed (status);
}

Rts2CommandSearch::Rts2CommandSearch (Rts2DevClientTelescope * in_tel, double searchRadius, double searchSpeed):Rts2Command (in_tel->
	     getMaster
	     ())
{
  char *
    command;
  asprintf (&command, "search %lf %lf", searchRadius, searchSpeed);
  setCommand (command);
  free (command);
  tel = in_tel;
}

int
Rts2CommandSearch::commandReturnFailed (int status)
{
  tel->searchFailed (status);
  return Rts2Command::commandReturnFailed (status);
}

Rts2CommandSearchStop::Rts2CommandSearchStop (Rts2DevClientTelescope * in_tel):Rts2Command (in_tel->getMaster (),
	     "searchstop")
{
}

Rts2CommandChange::Rts2CommandChange (Rts2Block * in_master, double ra, double dec):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "change %lf %lf", ra, dec);
  setCommand (command);
  free (command);
  tel = NULL;
  commandCond = IN_WAIT_STATE;
}

Rts2CommandChange::Rts2CommandChange (Rts2DevClientTelescope * in_tel,
				      double ra, double dec):
Rts2Command (in_tel->getMaster ())
{
  char *command;
  asprintf (&command, "change %lf %lf", ra, dec);
  setCommand (command);
  free (command);
  tel = in_tel;
  commandCond = IN_WAIT_STATE;
}

Rts2CommandChange::Rts2CommandChange (Rts2CommandChange * in_command, Rts2DevClientTelescope * in_tel):Rts2Command
  (in_command)
{
  tel = in_tel;
}

int
Rts2CommandChange::commandReturnFailed (int status)
{
  if (tel)
    tel->moveFailed (status);
  return Rts2Command::commandReturnFailed (status);
}


Rts2CommandCorrect::Rts2CommandCorrect (Rts2Block * in_master, int corr_mark, double ra, double dec, double ra_err, double dec_err):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "correct %i %lf %lf %lf %lf", corr_mark, ra_err,
	    dec_err, ra, dec);
  setCommand (command);
  free (command);
}

Rts2CommandStartGuide::Rts2CommandStartGuide (Rts2Block * in_master, char dir,
					      double dir_dist):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "start_guide %c %lf", dir, dir_dist);
  setCommand (command);
  free (command);
}

Rts2CommandStopGuide::Rts2CommandStopGuide (Rts2Block * in_master, char dir):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "stop_guide %c", dir);
  setCommand (command);
  free (command);
}

Rts2CommandCupolaMove::Rts2CommandCupolaMove (Rts2DevClientCupola * in_copula,
					      double ra, double dec):
Rts2Command (in_copula->getMaster ())
{
  char *msg;
  copula = in_copula;
  asprintf (&msg, "move %f %f", ra, dec);
  setCommand (msg);
  free (msg);
}

int
Rts2CommandCupolaMove::commandReturnFailed (int status)
{
  if (copula)
    copula->syncFailed (status);
  return Rts2Command::commandReturnFailed (status);
}

void
Rts2CommandChangeFocus::change (int in_steps)
{
  char *msg;
  asprintf (&msg, "step %i", in_steps);
  setCommand (msg);
  free (msg);
}

Rts2CommandChangeFocus::Rts2CommandChangeFocus (Rts2DevClientFocus *
						in_focuser, int in_steps):
Rts2Command (in_focuser->getMaster ())
{
  focuser = in_focuser;
  camera = NULL;
  change (in_steps);
}

Rts2CommandChangeFocus::Rts2CommandChangeFocus (Rts2DevClientCamera *
						in_camera, int in_steps):
Rts2Command (in_camera->getMaster ())
{
  focuser = NULL;
  camera = in_camera;
  change (in_steps);
}

int
Rts2CommandChangeFocus::commandReturnFailed (int status)
{
  if (focuser)
    focuser->focusingFailed (status);
  if (camera)
    camera->focuserFailed (status);
  return Rts2Command::commandReturnFailed (status);
}

void
Rts2CommandSetFocus::set (int in_steps)
{
  char *msg;
  asprintf (&msg, "set %i", in_steps);
  setCommand (msg);
  free (msg);
}

Rts2CommandSetFocus::Rts2CommandSetFocus (Rts2DevClientFocus * in_focuser,
					  int in_steps):
Rts2Command (in_focuser->getMaster ())
{
  focuser = in_focuser;
  camera = NULL;
  set (in_steps);
}

Rts2CommandSetFocus::Rts2CommandSetFocus (Rts2DevClientCamera * in_camera,
					  int in_steps):
Rts2Command (in_camera->getMaster ())
{
  focuser = NULL;
  camera = in_camera;
  set (in_steps);
}

int
Rts2CommandSetFocus::commandReturnFailed (int status)
{
  if (focuser)
    focuser->focusingFailed (status);
  if (camera)
    camera->focuserFailed (status);
  return Rts2Command::commandReturnFailed (status);
}

Rts2CommandMirror::Rts2CommandMirror (Rts2DevClientMirror * in_mirror, int in_pos):Rts2Command (in_mirror->
	     getMaster
	     ())
{
  mirror = in_mirror;
  char *
    msg;
  asprintf (&msg, "set %c", (in_pos == 0 ? 'A' : 'B'));
  setCommand (msg);
  free (msg);
}

int
Rts2CommandMirror::commandReturnFailed (int status)
{
  if (mirror)
    mirror->moveFailed (status);
  return Rts2Command::commandReturnFailed (status);
}

Rts2CommandIntegrate::Rts2CommandIntegrate (Rts2DevClientPhot * in_phot, int in_filter, float in_exp, int in_count):Rts2Command (in_phot->
	     getMaster
	     ())
{
  phot = in_phot;
  char *
    msg;
  asprintf (&msg, "intfil %i %f %i", in_filter, in_exp, in_count);
  setCommand (msg);
  free (msg);
}

Rts2CommandIntegrate::Rts2CommandIntegrate (Rts2DevClientPhot * in_phot,
					    float in_exp, int in_count):
Rts2Command (in_phot->getMaster ())
{
  phot = in_phot;
  char *msg;
  asprintf (&msg, "integrate %f %i", in_exp, in_count);
  setCommand (msg);
  free (msg);
}


int
Rts2CommandIntegrate::commandReturnFailed (int status)
{
  if (phot)
    phot->integrationFailed (status);
  return Rts2Command::commandReturnFailed (status);
}

Rts2CommandExecNext::Rts2CommandExecNext (Rts2Block * in_master, int next_id):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "next %i", next_id);
  setCommand (command);
  free (command);
}

Rts2CommandExecNow::Rts2CommandExecNow (Rts2Block * in_master, int now_id):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "now %i", now_id);
  setCommand (command);
  free (command);
}

Rts2CommandExecGrb::Rts2CommandExecGrb (Rts2Block * in_master, int grb_id):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "grb %i", grb_id);
  setCommand (command);
  free (command);
}

Rts2CommandExecShower::Rts2CommandExecShower (Rts2Block * in_master):
Rts2Command (in_master)
{
  setCommand ("shower");
}

Rts2CommandKillAll::Rts2CommandKillAll (Rts2Block * in_master):Rts2Command
  (in_master)
{
  setCommand ("killall");
}

Rts2CommandScriptEnds::Rts2CommandScriptEnds (Rts2Block * in_master):Rts2Command
  (in_master)
{
  setCommand ("script_ends");
}

Rts2CommandMessageMask::Rts2CommandMessageMask (Rts2Block * in_master,
						int in_mask):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "message_mask %i", in_mask);
  setCommand (command);
  free (command);
}

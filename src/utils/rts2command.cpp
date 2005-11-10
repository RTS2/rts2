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
  if (status)
    return commandReturnFailed (status);
  return commandReturnOK ();
}

int
Rts2Command::commandReturnFailed (int status)
{
  return -1;
}

int
Rts2Command::commandReturnOK ()
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
					  exposureType exp_type,
					  float exp_time):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "expose 0 %i %f", (exp_type == EXP_LIGHT ? 1 : 0),
	    exp_time);
  setCommand (command);
  free (command);
  camera = in_camera;
  commandCond = NO_EXPOSURE;
}

int
Rts2CommandExposure::commandReturnFailed (int status)
{
  camera->exposureFailed (status);
  return Rts2Command::commandReturnFailed (status);
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
  setCommandFilter (filter);
}

Rts2CommandFilter::Rts2CommandFilter (Rts2DevClientPhot * in_phot,
				      int filter):
Rts2Command (in_phot->getMaster ())
{
  camera = NULL;
  phot = in_phot;
  setCommandFilter (filter);
}

int
Rts2CommandFilter::commandReturnOK ()
{
  if (camera)
    camera->filterOK ();
  return Rts2Command::commandReturnOK ();
}

int
Rts2CommandFilter::commandReturnFailed (int status)
{
  if (camera)
    camera->filterFailed ();
  if (phot)
    phot->filterMoveFailed (status);
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

Rts2CommandMove::Rts2CommandMove (Rts2Block * in_master,
				  Rts2DevClientTelescope * in_tel, double ra,
				  double dec):
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

Rts2CommandMoveFixed::Rts2CommandMoveFixed (Rts2Block * in_master, Rts2DevClientTelescope * in_tel, double ra, double dec):
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

Rts2CommandCopulaMove::Rts2CommandCopulaMove (Rts2DevClientCopula * in_copula,
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
Rts2CommandCopulaMove::commandReturnFailed (int status)
{
  if (copula)
    copula->syncFailed (status);
  return Rts2Command::commandReturnFailed (status);
}

Rts2CommandChangeFocus::Rts2CommandChangeFocus (Rts2DevClientFocus * in_focuser, int in_steps):
Rts2Command (in_focuser->getMaster ())
{
  char *msg;
  focuser = in_focuser;
  asprintf (&msg, "step %i", in_steps);
  setCommand (msg);
  free (msg);
}

int
Rts2CommandChangeFocus::commandReturnFailed (int status)
{
  if (focuser)
    focuser->focusingFailed (status);
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

Rts2CommandExecGrb::Rts2CommandExecGrb (Rts2Block * in_master, int grb_id):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "grb %i", grb_id);
  setCommand (command);
  free (command);
}

Rts2CommandKillAll::Rts2CommandKillAll (Rts2Block * in_master):Rts2Command
  (in_master)
{
  setCommand ("killall");
}

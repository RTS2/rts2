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
}

Rts2Command::Rts2Command (Rts2Block * in_owner, char *in_text)
{
  owner = in_owner;
  setCommand (in_text);
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
  delete text;
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

Rts2CommandBinning::Rts2CommandBinning (Rts2Block * in_master, int binning_v,
					int binning_h):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "binning %i %i", binning_v, binning_h);
  setCommand (command);
  free (command);
}

Rts2CommandExposure::Rts2CommandExposure (Rts2Block * in_master,
					  exposureType exp_type,
					  float exp_time):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "expose 0 %i %f", (exp_type == EXP_LIGHT ? 1 : 0),
	    exp_time);
  setCommand (command);
  free (command);
}

Rts2CommandFilter::Rts2CommandFilter (Rts2Block * in_master, int filter):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "filter %i", filter);
  setCommand (command);
  free (command);
}

Rts2CommandMove::Rts2CommandMove (Rts2Block * in_master, double ra,
				  double dec):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "move %lf %lf", ra, dec);
  setCommand (command);
  free (command);
}

Rts2CommandCorrect::Rts2CommandCorrect (Rts2Block * in_master, int corr_mark,
					double ra, double dec, double ra_err,
					double dec_err):
Rts2Command (in_master)
{
  char *command;
  asprintf (&command, "correct %i %lf %lf %lf %lf", corr_mark, ra, dec,
	    ra_err, dec_err);
  setCommand (command);
  free (command);
}

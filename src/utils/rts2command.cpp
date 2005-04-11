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

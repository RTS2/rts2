#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <math.h>
#include <stdio.h>

#include "rts2block.h"
#include "rts2value.h"

Rts2Value::Rts2Value (char *in_val_name)
{
  valueName = in_val_name;
}

Rts2ValueString::Rts2ValueString (char *in_val_name):
Rts2Value (in_val_name)
{
  value = NULL;
}

void
Rts2ValueString::getValue (char *buf[])
{
  if (!value)
    {
      *buf = NULL;
    }
  else
    {
      *buf = new char[strlen (value) + 1];
      strcpy (*buf, value);
    }
}

int
Rts2ValueString::setValue (Rts2Conn * connection)
{
  char *new_value;
  if (connection->paramNextString (&new_value) || !connection->paramEnd ())
    return -3;
  delete value;
  value = new char[strlen (new_value) + 1];
  strcpy (value, new_value);
  return -1;
}

Rts2ValueInteger::Rts2ValueInteger (char *in_val_name):Rts2Value (in_val_name)
{
  value = 0;
}

void
Rts2ValueInteger::getValue (char *buf[])
{
  *buf = new char[20];
  sprintf (*buf, "%i", value);
}

int
Rts2ValueInteger::setValue (Rts2Conn * connection)
{
  int new_value;
  if (connection->paramNextInteger (&new_value) || !connection->paramEnd ())
    return -3;
  value = new_value;
  return -1;
}


Rts2ValueDouble::Rts2ValueDouble (char *in_val_name):Rts2Value (in_val_name)
{
  value = nan ("f");
}

void
Rts2ValueDouble::getValue (char *buf[])
{
  *buf = new char[20];
  sprintf (*buf, "%lf", value);
}

int
Rts2ValueDouble::setValue (Rts2Conn * connection)
{
  double new_value;
  if (connection->paramNextDouble (&new_value) || !connection->paramEnd ())
    return -3;
  value = new_value;
  return -1;
}

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

char *
Rts2ValueString::getValue (int width, int precision)
{
  if (!value)
    {
      return NULL;
    }
  else
    {
      strcpy (buf, value);
      return buf;
    }
}

int
Rts2ValueString::setValue (Rts2Conn * connection)
{
  char *new_value;
  if (connection->paramNextString (&new_value) || !connection->paramEnd ())
    return -3;
  delete[]value;
  value = new char[strlen (new_value) + 1];
  strcpy (value, new_value);
  return 0;
}

Rts2ValueInteger::Rts2ValueInteger (char *in_val_name):Rts2Value (in_val_name)
{
  value = 0;
}

char *
Rts2ValueInteger::getValue (int width, int precision)
{
  sprintf (buf, "%i", value);
  return buf;
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

Rts2ValueTime::Rts2ValueTime (char *in_val_name):Rts2Value (in_val_name)
{
  value = 0;
}

char *
Rts2ValueTime::getValue (int width, int precision)
{
  struct tm *t;
  t = localtime (&value);
  strftime (buf, 100, "%c", t);
  return buf;
}

int
Rts2ValueTime::setValue (Rts2Conn * connection)
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

char *
Rts2ValueDouble::getValue (int width, int precision)
{
  sprintf (buf, "%lf", value);
  return buf;
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

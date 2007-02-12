#include <math.h>
#include <stdio.h>
#include <sstream>

#include "rts2block.h"
#include "rts2value.h"

Rts2Value::Rts2Value (char *in_val_name)
{
  valueName = std::string (in_val_name);
  rts2Type = 0;
}

Rts2Value::Rts2Value (char *in_val_name, std::string in_description,
		      bool writeToFits)
{
  valueName = std::string (in_val_name);
  rts2Type = 0;
  description = in_description;
  if (writeToFits)
    setWriteToFits ();
}

int
Rts2Value::sendMetaInfo (Rts2Conn * connection)
{
  int ret;
  std::ostringstream _os;
  _os
    << PROTO_METAINFO << " "
    << rts2Type << " "
    << '"' << getName () << "\" " << '"' << description << "\" ";
  ret = connection->send (_os.str ());
  if (ret < 0)
    return ret;
  return send (connection);
}

int
Rts2Value::send (Rts2Conn * connection)
{
  return connection->sendValue (getName (), getValue ());
}

Rts2ValueString::Rts2ValueString (char *in_val_name):
Rts2Value (in_val_name)
{
  value = NULL;
  rts2Type |= RTS2_VALUE_STRING;
}

Rts2ValueString::Rts2ValueString (char *in_val_name,
				  std::string in_description,
				  bool writeToFits):
Rts2Value (in_val_name, in_description, writeToFits)
{
  value = NULL;
  rts2Type |= RTS2_VALUE_STRING;
}

char *
Rts2ValueString::getValue ()
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
  setValueString (new_value);
  return 0;
}

void
Rts2ValueString::setValueString (char *in_value)
{
  delete[]value;
  if (!in_value)
    {
      value = NULL;
      return;
    }
  value = new char[strlen (in_value) + 1];
  strcpy (value, in_value);
}

Rts2ValueInteger::Rts2ValueInteger (char *in_val_name):
Rts2Value (in_val_name)
{
  value = 0;
  rts2Type |= RTS2_VALUE_INTEGER;
}

Rts2ValueInteger::Rts2ValueInteger (char *in_val_name,
				    std::string in_description,
				    bool writeToFits):
Rts2Value (in_val_name, in_description, writeToFits)
{
  value = 0;
  rts2Type |= RTS2_VALUE_INTEGER;
}


char *
Rts2ValueInteger::getValue ()
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
  return 0;
}

Rts2ValueDouble::Rts2ValueDouble (char *in_val_name):Rts2Value (in_val_name)
{
  value = nan ("f");
  rts2Type |= RTS2_VALUE_DOUBLE;
}

Rts2ValueDouble::Rts2ValueDouble (char *in_val_name,
				  std::string in_description,
				  bool writeToFits):
Rts2Value (in_val_name, in_description, writeToFits)
{
  value = nan ("f");
  rts2Type |= RTS2_VALUE_DOUBLE;
}

char *
Rts2ValueDouble::getValue ()
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
  return 0;
}

Rts2ValueTime::Rts2ValueTime (char *in_val_name):Rts2ValueDouble (in_val_name)
{
  rts2Type = (~RTS2_VALUE_MASK & rts2Type) | RTS2_VALUE_TIME;
}

Rts2ValueTime::Rts2ValueTime (char *in_val_name, std::string in_description,
			      bool writeToFits):
Rts2ValueDouble (in_val_name, in_description, writeToFits)
{
  rts2Type = (~RTS2_VALUE_MASK & rts2Type) | RTS2_VALUE_TIME;
}

Rts2ValueFloat::Rts2ValueFloat (char *in_val_name):
Rts2Value (in_val_name)
{
  value = nan ("f");
  rts2Type |= RTS2_VALUE_FLOAT;
}

Rts2ValueFloat::Rts2ValueFloat (char *in_val_name, std::string in_description,
				bool writeToFits):
Rts2Value (in_val_name, in_description, writeToFits)
{
  value = nan ("f");
  rts2Type |= RTS2_VALUE_FLOAT;
}

char *
Rts2ValueFloat::getValue ()
{
  sprintf (buf, "%lf", value);
  return buf;
}

int
Rts2ValueFloat::setValue (Rts2Conn * connection)
{
  float new_value;
  if (connection->paramNextFloat (&new_value) || !connection->paramEnd ())
    return -3;
  value = new_value;
  return 0;
}

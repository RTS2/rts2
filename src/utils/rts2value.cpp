#include <math.h>
#include <stdio.h>
#include <sstream>

#include "rts2block.h"
#include "rts2value.h"

Rts2Value::Rts2Value (std::string in_val_name)
{
  valueName = in_val_name;
  rts2Type = 0;
}

Rts2Value::Rts2Value (std::string in_val_name, std::string in_description,
		      bool writeToFits, int32_t displayType)
{
  valueName = in_val_name;
  rts2Type = 0;
  description = in_description;
  if (writeToFits)
    setWriteToFits ();
  setValueDisplayType (displayType);
}

int
Rts2Value::doOpValue (char op, Rts2Value * old_value)
{
  switch (op)
    {
    case '=':
      return 0;
    default:
      logStream (MESSAGE_ERROR) << "unknow op '" << op << "' for type " <<
	getValueType () << sendLog;
      return -1;
    }
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

Rts2ValueString::Rts2ValueString (std::string in_val_name):
Rts2Value (in_val_name)
{
  value = NULL;
  rts2Type |= RTS2_VALUE_STRING;
}

Rts2ValueString::Rts2ValueString (std::string in_val_name, std::string in_description, bool writeToFits, int32_t displayType):
Rts2Value (in_val_name, in_description, writeToFits, displayType)
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

void
Rts2ValueString::setFromValue (Rts2Value * newValue)
{
  setValueString (newValue->getValue ());
}

Rts2ValueInteger::Rts2ValueInteger (std::string in_val_name):
Rts2Value (in_val_name)
{
  value = 0;
  rts2Type |= RTS2_VALUE_INTEGER;
}

Rts2ValueInteger::Rts2ValueInteger (std::string in_val_name, std::string in_description, bool writeToFits, int32_t displayType):
Rts2Value (in_val_name, in_description, writeToFits, displayType)
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

int
Rts2ValueInteger::doOpValue (char op, Rts2Value * old_value)
{
  switch (op)
    {
    case '+':
      setValueInteger (old_value->getValueInteger () + getValueInteger ());
      break;
    case '-':
      setValueInteger (old_value->getValueInteger () - getValueInteger ());
      break;
    default:
      return Rts2Value::doOpValue (op, old_value);
    }
  return 0;
}

void
Rts2ValueInteger::setFromValue (Rts2Value * newValue)
{
  setValueInteger (newValue->getValueInteger ());
}

Rts2ValueDouble::Rts2ValueDouble (std::string in_val_name):Rts2Value
  (in_val_name)
{
  value = nan ("f");
  rts2Type |= RTS2_VALUE_DOUBLE;
}

Rts2ValueDouble::Rts2ValueDouble (std::string in_val_name, std::string in_description, bool writeToFits, int32_t displayType):
Rts2Value (in_val_name, in_description, writeToFits, displayType)
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

int
Rts2ValueDouble::doOpValue (char op, Rts2Value * old_value)
{
  switch (op)
    {
    case '+':
      setValueDouble (old_value->getValueDouble () + getValueDouble ());
      break;
    case '-':
      setValueDouble (old_value->getValueDouble () - getValueDouble ());
      break;
    default:
      return Rts2Value::doOpValue (op, old_value);
    }
  return 0;
}

void
Rts2ValueDouble::setFromValue (Rts2Value * newValue)
{
  setValueDouble (newValue->getValueDouble ());
}

Rts2ValueTime::Rts2ValueTime (std::string in_val_name):Rts2ValueDouble
  (in_val_name)
{
  rts2Type = (~RTS2_VALUE_MASK & rts2Type) | RTS2_VALUE_TIME;
}

Rts2ValueTime::Rts2ValueTime (std::string in_val_name, std::string in_description, bool writeToFits, int32_t displayType):
Rts2ValueDouble (in_val_name, in_description, writeToFits, displayType)
{
  rts2Type = (~RTS2_VALUE_MASK & rts2Type) | RTS2_VALUE_TIME;
}

Rts2ValueFloat::Rts2ValueFloat (std::string in_val_name):
Rts2Value (in_val_name)
{
  value = nan ("f");
  rts2Type |= RTS2_VALUE_FLOAT;
}

Rts2ValueFloat::Rts2ValueFloat (std::string in_val_name, std::string in_description, bool writeToFits, int32_t displayType):
Rts2Value (in_val_name, in_description, writeToFits, displayType)
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

int
Rts2ValueFloat::doOpValue (char op, Rts2Value * old_value)
{
  switch (op)
    {
    case '+':
      setValueFloat (old_value->getValueFloat () + getValueFloat ());
      break;
    case '-':
      setValueFloat (old_value->getValueFloat () - getValueFloat ());
      break;
    default:
      return Rts2Value::doOpValue (op, old_value);
    }
  return 0;
}

void
Rts2ValueFloat::setFromValue (Rts2Value * newValue)
{
  setValueFloat (newValue->getValueFloat ());
}

Rts2ValueBool::Rts2ValueBool (std::string in_val_name):Rts2ValueInteger
  (in_val_name)
{
  rts2Type = (~RTS2_VALUE_MASK & rts2Type) | RTS2_VALUE_BOOL;
}

Rts2ValueBool::Rts2ValueBool (std::string in_val_name, std::string in_description, bool writeToFits, int32_t displayType):Rts2ValueInteger (in_val_name, in_description,
		  writeToFits,
		  displayType)
{
  rts2Type = (~RTS2_VALUE_MASK & rts2Type) | RTS2_VALUE_BOOL;
}

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <algorithm>

#include "rts2block.h"
#include "rts2devclient.h"

Rts2DevClient::Rts2DevClient (Rts2Conn * in_connection):Rts2Object ()
{
  connection = in_connection;
  processedBaseInfo = NOT_PROCESED;
  addValue (new Rts2ValueString ("type"));
  addValue (new Rts2ValueString ("serial"));
}

void
Rts2DevClient::addValue (Rts2Value * value)
{
  values.push_back (value);
}

char *
Rts2DevClient::getValue (char *value_name)
{
  std::vector < Rts2Value * >::iterator val_iter;
  for (val_iter = values.begin (); val_iter != values.end (); val_iter++)
    {


    }
  return NULL;
}

int
Rts2DevClient::command ()
{
  std::vector < Rts2Value * >::iterator val_iter;
  for (val_iter = values.begin (); val_iter != values.end (); val_iter++)
    {
      Rts2Value *value;
      value = (*val_iter);
      if (connection->isCommand (value->getName ()))
	return value->setValue (connection);
    }
  return -1;
}

Rts2DevClientCamera::Rts2DevClientCamera (Rts2Conn * in_connection):Rts2DevClient
  (in_connection)
{
  addValue (new Rts2ValueInteger ("chips"));
  addValue (new Rts2ValueInteger ("can_df"));
  addValue (new Rts2ValueInteger ("temperature_regulation"));
  addValue (new Rts2ValueDouble ("temperature_setpoint"));
  addValue (new Rts2ValueDouble ("air_temperature"));
  addValue (new Rts2ValueDouble ("ccd_temperature"));
  addValue (new Rts2ValueInteger ("cooling_power"));
  addValue (new Rts2ValueInteger ("fan"));
  addValue (new Rts2ValueInteger ("filter"));
}

Rts2DevClientTelescope::Rts2DevClientTelescope (Rts2Conn * in_connection):Rts2DevClient
  (in_connection)
{
  addValue (new Rts2ValueDouble ("longtitude"));
  addValue (new Rts2ValueDouble ("latitude"));
  addValue (new Rts2ValueDouble ("altitude"));
  addValue (new Rts2ValueDouble ("ra"));
  addValue (new Rts2ValueDouble ("dev"));
  addValue (new Rts2ValueDouble ("siderealtime"));
  addValue (new Rts2ValueDouble ("localtime"));
  addValue (new Rts2ValueInteger ("flip"));
  addValue (new Rts2ValueDouble ("axis0_counts"));
  addValue (new Rts2ValueDouble ("axis1_counts"));
}

Rts2DevClientDome::Rts2DevClientDome (Rts2Conn * in_connection):Rts2DevClient
  (in_connection)
{
  addValue (new Rts2ValueInteger ("dome"));
}

Rts2DevClientPhot::Rts2DevClientPhot (Rts2Conn * in_connection):Rts2DevClient
  (in_connection)
{
  addValue (new Rts2ValueDouble ("filter"));
  addValue (new Rts2ValueDouble ("filter_c"));
}

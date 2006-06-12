#include "rts2soapcli.h"

Rts2DevClientTelescopeSoap::Rts2DevClientTelescopeSoap (Rts2Conn * in_connection):Rts2DevClientTelescope
  (in_connection)
{
}

void
Rts2DevClientTelescopeSoap::postEvent (Rts2Event * event)
{
  struct ns1__getEquResponse *res;
  switch (event->getType ())
    {
    case EVENT_SOAP_TEL_GETEQU:
      res = (ns1__getEquResponse *) event->getArg ();
      res->radec->ra = getValueDouble ("ra");
      res->radec->dec = getValueDouble ("dec");
      break;
    }

  Rts2DevClientTelescope::postEvent (event);
}

Rts2DevClientExecutorSoap::Rts2DevClientExecutorSoap (Rts2Conn * in_connection):Rts2DevClientExecutor
  (in_connection)
{
}

void
Rts2DevClientExecutorSoap::postEvent (Rts2Event * event)
{
  struct ns1__getExecResponse *res;
  switch (event->getType ())
    {
    case EVENT_SOAP_EXEC_GETST:
      res = (ns1__getExecResponse *) event->getArg ();
      res->current = getValueInteger ("current_sel");
      // res->name = getValueChar ("current_info");
      res->next = getValueInteger ("obsid");
      break;
    }

  Rts2DevClientExecutor::postEvent (event);
}

Rts2DevClientDomeSoap::Rts2DevClientDomeSoap (Rts2Conn * in_connection):Rts2DevClientDome
  (in_connection)
{
}

void
Rts2DevClientDomeSoap::postEvent (Rts2Event * event)
{
  struct ns1__getDomeResponse *res;
  switch (event->getType ())
    {
    case EVENT_SOAP_DOME_GETST:
      res = (ns1__getDomeResponse *) event->getArg ();
      res->temp = getValueDouble ("temperature");
      res->humi = getValueDouble ("humidity");
      res->wind = getValueDouble ("windspeed");
      break;
    }
}

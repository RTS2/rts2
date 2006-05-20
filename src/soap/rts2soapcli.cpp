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
      res->ra = getValueDouble ("ra");
      res->dec = getValueDouble ("dec");
      break;
    }

  Rts2DevClientTelescope::postEvent (event);
}

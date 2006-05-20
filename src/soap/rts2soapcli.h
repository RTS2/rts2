#ifndef __RTS2_SOAPCLI__
#define __RTS2_SOAPCLI__

#include "../utils/rts2devclient.h"
#include "soapH.h"

#define EVENT_SOAP_TEL_GETEQU  RTS2_LOCAL_EVENT+1000

class Rts2DevClientTelescopeSoap:public Rts2DevClientTelescope
{
public:
  Rts2DevClientTelescopeSoap (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
};

#endif /*!__RTS2_SOAPCLI__ */

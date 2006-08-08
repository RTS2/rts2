#ifndef __RTS2_SOAPCLI__
#define __RTS2_SOAPCLI__

#include "../utils/rts2devclient.h"
#include "soapH.h"

#define EVENT_SOAP_TEL_GETEQU  RTS2_LOCAL_EVENT+1000
#define EVENT_SOAP_TEL_GET     RTS2_LOCAL_EVENT+1001
#define EVENT_SOAP_EXEC_GETST  RTS2_LOCAL_EVENT+1002
#define EVENT_SOAP_DOME_GETST  RTS2_LOCAL_EVENT+1003

class Rts2DevClientTelescopeSoap:public Rts2DevClientTelescope
{
public:
  Rts2DevClientTelescopeSoap (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
};

class Rts2DevClientExecutorSoap:public Rts2DevClientExecutor
{
private:
  void fillTarget (int in_tar_id, rts2__target * out_target);
public:
    Rts2DevClientExecutorSoap (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
};

class Rts2DevClientDomeSoap:public Rts2DevClientDome
{
public:
  Rts2DevClientDomeSoap (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
};

#endif /*!__RTS2_SOAPCLI__ */

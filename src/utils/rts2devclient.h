#ifndef __RTS2_DEVCLIENT__
#define __RTS2_DEVCLIENT__

#include <libnova/libnova.h>
#include <vector>

#include "rts2object.h"
#include "rts2block.h"
#include "rts2value.h"

class Rts2ClientTCPDataConn;
class Rts2ServerState;

/**************************************
 *
 * Defines default classes for handling device information.
 * 
 * Descendants of rts2client can (by overloading method getConnection)
 * specify devices classes, which e.g. allow X11 printout of device
 * information etc..
 * 
 *************************************/

class Rts2DevClient:public Rts2Object
{
protected:
  Rts2Conn * connection;
  enum
  { NOT_PROCESED, PROCESED } processedBaseInfo;
  Rts2Value *getValue (char *value_name);
public:
    Rts2DevClient (Rts2Conn * in_connection);
  void addValue (Rts2Value * value);
  char *getValueChar (char *value_name);
  double getValueDouble (char *value_name);
  int getValueInteger (char *value_name);

  int command ();

    std::vector < Rts2Value * >values;

  virtual void dataReceived (Rts2ClientTCPDataConn * dataConn)
  {
  }

  virtual void stateChanged (Rts2ServerState * state)
  {
  }
};

/**************************************
 *
 * Classes for devices connections.
 *
 * Please note that we cannot use descendants of Rts2ConnClient,
 * since we can connect to device as server.
 * 
 *************************************/

class Rts2DevClientCamera:public Rts2DevClient
{
public:
  Rts2DevClientCamera (Rts2Conn * in_connection);

};

class Rts2DevClientTelescope:public Rts2DevClient
{
protected:
  void getEqu (struct ln_equ_posn *tel);
  void getObs (struct ln_lnlat_posn *obs);

public:
    Rts2DevClientTelescope (Rts2Conn * in_connection);

};

class Rts2DevClientDome:public Rts2DevClient
{
public:
  Rts2DevClientDome (Rts2Conn * in_connection);
};

class Rts2DevClientMirror:public Rts2DevClient
{
public:
  Rts2DevClientMirror (Rts2Conn * in_connection);

};

class Rts2DevClientFocus:public Rts2DevClient
{
public:
  Rts2DevClientFocus (Rts2Conn * in_connection);

};

class Rts2DevClientPhot:public Rts2DevClient
{
public:
  Rts2DevClientPhot (Rts2Conn * in_connection);

};

/*class Rts2DevClientPlanc : public Rts2DevClient
{
public:
  Rts
};*/

#endif /* !__RTS2_DEVCLIENT__ */

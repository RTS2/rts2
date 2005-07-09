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
public:
    Rts2DevClient (Rts2Conn * in_connection);
  void addValue (Rts2Value * value);
  Rts2Value *getValue (char *value_name);
  char *getValueChar (char *value_name);
  double getValueDouble (char *value_name);
  int getValueInteger (char *value_name);

  virtual int command ();

    std::vector < Rts2Value * >values;

  virtual void dataReceived (Rts2ClientTCPDataConn * dataConn)
  {
  }

  virtual void stateChanged (Rts2ServerState * state)
  {
  }

  const char *getName ();
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
  double getLocalSiderealDeg ();

public:
    Rts2DevClientTelescope (Rts2Conn * in_connection);
  /*! gets calledn when move finished without success */
  virtual void moveFailed (int status)
  {
  }
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

class Rts2DevClientExecutor:public Rts2DevClient
{
public:
  Rts2DevClientExecutor (Rts2Conn * in_connection);
};

class Rts2DevClientSelector:public Rts2DevClient
{
public:
  Rts2DevClientSelector (Rts2Conn * in_connection);
};

class Rts2DevClientImgproc:public Rts2DevClient
{
public:
  Rts2DevClientImgproc (Rts2Conn * in_connection);
  virtual int command ();
};

class Rts2DevClientGrb:public Rts2DevClient
{
public:
  Rts2DevClientGrb (Rts2Conn * in_connection);
};

#endif /* !__RTS2_DEVCLIENT__ */

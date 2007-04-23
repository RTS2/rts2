#ifndef __RTS2_DEVCLIENT__
#define __RTS2_DEVCLIENT__

#include "rts2object.h"
#include "rts2block.h"
#include "rts2value.h"
#include "rts2valuelist.h"

class Rts2Command;
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

class Rts2Block;

class Rts2DevClient:public Rts2Object
{
private:
  int failedCount;
  Rts2ValueVector values;
protected:
    Rts2Conn * connection;
  enum
  { NOT_PROCESED, PROCESED } processedBaseInfo;
  virtual void getPriority ();
  virtual void lostPriority ();
  virtual void died ();
  enum
  { NOT_WAITING, WAIT_MOVE, WAIT_NOT_POSSIBLE } waiting;	// if we wait for something..
  virtual void blockWait ();
  virtual void unblockWait ();
  virtual void unsetWait ();
  virtual void clearWait ();
  virtual int isWaitMove ();
  virtual void setWaitMove ();

  void incFailedCount ()
  {
    failedCount++;
  }
  int getFailedCount ()
  {
    return failedCount;
  }
  void clearFailedCount ()
  {
    failedCount = 0;
  }
public:
  Rts2DevClient (Rts2Conn * in_connection);
  virtual ~ Rts2DevClient (void);
  virtual void postEvent (Rts2Event * event);
  void addValue (Rts2Value * value);
  int metaInfo (int rts2Type, std::string name, std::string desc);
  Rts2Value *getValue (const char *value_name);
  char *getValueChar (const char *value_name);
  double getValueDouble (const char *value_name);
  int getValueInteger (const char *value_name);

  virtual int commandValue (const char *name);
  virtual int command ();

  virtual void dataReceived (Rts2ClientTCPDataConn * dataConn)
  {
  }

  virtual void stateChanged (Rts2ServerState * state);
  void priorityInfo (bool have);

  const char *getName ();

  Rts2Block *getMaster ();

  int queCommand (Rts2Command * cmd);

  int commandReturn (Rts2Command * cmd, int status)
  {
    if (status)
      incFailedCount ();
    else
      clearFailedCount ();
    return 0;
  }

  int getStatus ();

  Rts2ValueVector::iterator valueBegin ()
  {
    return values.begin ();
  }
  Rts2ValueVector::iterator valueEnd ()
  {
    return values.end ();
  }
  Rts2Value *valueAt (int index)
  {
    return values[index];
  }
  int valueSize ()
  {
    return values.size ();
  }
  virtual void settingsOK ()
  {
  }
  virtual void settingsFailed (int status)
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
protected:
  virtual void exposureStarted ();
  virtual void exposureEnd ();
  virtual void readoutEnd ();
public:
    Rts2DevClientCamera (Rts2Conn * in_connection);
  // exposureFailed will get called even when we faild during readout
  virtual void exposureFailed (int status);
  virtual void stateChanged (Rts2ServerState * state);

  virtual void filterOK ()
  {
  }
  virtual void filterFailed (int status)
  {
  }
  virtual void focuserOK ()
  {
  }
  virtual void focuserFailed (int status)
  {
  }

  bool isIdle ();
};

class Rts2DevClientTelescope:public Rts2DevClient
{
protected:
  virtual void moveStart ();
  virtual void moveEnd ();
  virtual void searchStart ();
  virtual void searchEnd ();
public:
    Rts2DevClientTelescope (Rts2Conn * in_connection);
    virtual ~ Rts2DevClientTelescope (void);
  /*! gets calledn when move finished without success */
  virtual void moveFailed (int status)
  {
  }
  virtual void searchFailed (int status)
  {
  }
  virtual void stateChanged (Rts2ServerState * state);
  virtual void postEvent (Rts2Event * event);
};

class Rts2DevClientDome:public Rts2DevClient
{
public:
  Rts2DevClientDome (Rts2Conn * in_connection);
};

class Rts2DevClientCupola:public Rts2DevClientDome
{
protected:
  virtual void syncStarted ();
  virtual void syncEnded ();
public:
    Rts2DevClientCupola (Rts2Conn * in_connection);
  virtual void syncFailed (int status);
  virtual void stateChanged (Rts2ServerState * state);
};

class Rts2DevClientMirror:public Rts2DevClient
{
protected:
  virtual void mirrorA ();
  virtual void mirrorB ();
public:
    Rts2DevClientMirror (Rts2Conn * in_connection);
    virtual ~ Rts2DevClientMirror (void);
  virtual void moveFailed (int status);
  virtual void stateChanged (Rts2ServerState * state);
};

class Rts2DevClientFocus:public Rts2DevClient
{
protected:
  virtual void focusingStart ();
  virtual void focusingEnd ();
public:
    Rts2DevClientFocus (Rts2Conn * in_connection);
    virtual ~ Rts2DevClientFocus (void);
  virtual void focusingFailed (int status);
  virtual void stateChanged (Rts2ServerState * state);
};

class Rts2DevClientPhot:public Rts2DevClient
{
protected:
  virtual void filterMoveStart ();
  virtual void filterMoveEnd ();
  virtual void integrationStart ();
  virtual void integrationEnd ();
  virtual void addCount (int count, float exp, int is_ov);
  int lastCount;
  float lastExp;
  bool integrating;
public:
    Rts2DevClientPhot (Rts2Conn * in_connection);
    virtual ~ Rts2DevClientPhot (void);
  virtual int commandValue (const char *name);
  virtual void integrationFailed (int status);
  virtual void filterMoveFailed (int status);
  virtual void stateChanged (Rts2ServerState * state);

  virtual void filterOK ()
  {
  }

  bool isIntegrating ();
};

class Rts2DevClientFilter:public Rts2DevClient
{
protected:
  virtual void filterMoveStart ();
  virtual void filterMoveEnd ();
public:
    Rts2DevClientFilter (Rts2Conn * in_connection);
    virtual ~ Rts2DevClientFilter (void);
  virtual void filterMoveFailed (int status);
  virtual void stateChanged (Rts2ServerState * state);

  virtual void filterOK ()
  {
  }
};

class Rts2DevClientAugerShooter:public Rts2DevClient
{
public:
  Rts2DevClientAugerShooter (Rts2Conn * in_connection);
};

class Rts2DevClientExecutor:public Rts2DevClient
{
protected:
  virtual void lastReadout ();
public:
    Rts2DevClientExecutor (Rts2Conn * in_connection);
  virtual void stateChanged (Rts2ServerState * state);
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

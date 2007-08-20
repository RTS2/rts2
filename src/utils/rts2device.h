#ifndef __RTS2_DEVICE__
#define __RTS2_DEVICE__

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#include <malloc.h>
#include <getopt.h>
#include <sys/types.h>
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <vector>

#include "rts2daemon.h"
#include "rts2configraw.h"

#define CHECK_PRIORITY if (!conn->havePriority ()) { conn->sendCommandEnd (DEVDEM_E_PRIORITY, "haven't priority"); return -1; }

class Rts2Device;

/**
 * Device connection.
 *
 * Handles both connections which are created from clients to device, as well
 * as connections created from device to device. They are distinguished by
 * connType (set by setType, get by getType calls).
 */

class Rts2DevConn:public Rts2Conn
{
private:
  // in case we know address of other side..
  Rts2Address * address;

  Rts2Device *master;

  int doDeamonize ();
protected:
    virtual int command ();
public:
    Rts2DevConn (int in_sock, Rts2Device * in_master);

  virtual int init ();
  virtual int idle ();

  virtual int authorizationOK ();
  virtual int authorizationFailed ();
  void setHavePriority (int in_have_priority);

  virtual void setDeviceAddress (Rts2Address * in_addr);
  void setDeviceName (char *in_name);

  void connAuth ();

  virtual void setKey (int in_key);
  virtual void setConnState (conn_state_t new_conn_state);
};

class Rts2DevConnMaster:public Rts2Conn
{
  char *device_host;
  char master_host[HOST_NAME_MAX];
  int master_port;
  char device_name[DEVICE_NAME_SIZE];
  int device_type;
  int device_port;
  time_t nextTime;
protected:
    virtual int command ();
  virtual int priorityChange ();
  virtual int informations ();
  virtual int status ();
  virtual int connectionError (int last_data_size);
public:
    Rts2DevConnMaster (Rts2Block * in_master,
		       char *in_device_host, int in_device_port,
		       char *in_device_name, int in_device_type,
		       char *in_master_host, int in_master_port);
    virtual ~ Rts2DevConnMaster (void);
  int registerDevice ();
  virtual int init ();
  virtual int idle ();
  int authorize (Rts2DevConn * conn);
  void setHavePriority (int in_have_priority);
};

class Rts2DevConnData:public Rts2Conn
{
  int sendHeader ();
  Rts2Conn *dataConn;
protected:
  int command ()
  {
    logStream (MESSAGE_DEBUG) << "Rts2DevConnData::command badCommand " <<
      getCommand () << sendLog;
    // there isn't any command possible on data connection
    return -1;
  };
public:
Rts2DevConnData (Rts2Block * in_master, Rts2Conn * conn):Rts2Conn
    (in_master)
  {
    dataConn = conn;
  }
  virtual int init ();
  virtual int send (const char *msg);
  int send (char *data, size_t data_size);
};

class Rts2Device:public Rts2Daemon
{
private:
  Rts2DevConnMaster * conn_master;
  char *centrald_host;
  int centrald_port;

  // mode related variable
  char *modefile;
  Rts2ConfigRaw *modeconf;
  Rts2ValueSelection *modesel;

  int device_port;
  char *device_name;
  int device_type;

  int log_option;

  char *device_host;

  char *mailAddress;

  int setMode (int new_mode);

protected:
  /**
   * Process on option, when received from getopt () call.
   * 
   * @param in_opt  return value of getopt
   * @return 0 on success, -1 if option wasn't processed
   */
    virtual int processOption (int in_opt);
  virtual int init ();
  void clearStatesPriority ();

  virtual Rts2Conn *createClientConnection (char *in_deviceName);
  virtual Rts2Conn *createClientConnection (Rts2Address * in_addr);

  virtual void stateChanged (int new_state, int old_state,
			     const char *description);

  virtual void cancelPriorityOperations ();

  virtual bool queValueChange (Rts2CondValue * old_value)
  {
    return old_value->queValueChange (getState ());
  }

  virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
public:
  Rts2Device (int in_argc, char **in_argv, int in_device_type,
	      char *default_name);
  virtual ~ Rts2Device (void);
  virtual Rts2DevConn *createConnection (int in_sock);

  virtual int commandAuthorized (Rts2Conn * conn);

  int authorize (Rts2DevConn * conn);
  int sendMaster (char *msg)
  {
    return conn_master->send (msg);
  }

  // callback functions for device 
  virtual int ready ();

  virtual int ready (Rts2Conn * conn);

  int sendStateInfo (Rts2Conn * conn);

  // only devices can send messages
  virtual void sendMessage (messageType_t in_messageType,
			    const char *in_messageString);

  int sendMail (char *subject, char *text);

  int killAll ();
  virtual int scriptEnds ();

  virtual Rts2Conn *getCentraldConn ()
  {
    return conn_master;
  };
  char *getDeviceName ()
  {
    return device_name;
  };
  int getDeviceType ()
  {
    return device_type;
  };

  virtual void sigHUP (int sig);
};

#endif /* !__RTS2_DEVICE__ */

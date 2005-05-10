#ifndef __RTS2_DEVICE__
#define __RTS2_DEVICE__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#include <malloc.h>
#include <getopt.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>

#include "rts2block.h"

#define CHECK_PRIORITY if (!havePriority ()) { sendCommandEnd (DEVDEM_E_PRIORITY, "haven't priority"); return -1; }

class Rts2Device;

class Rts2Dev2DevConn:public Rts2Conn
{
private:
  Rts2Address * address;
public:
  Rts2Dev2DevConn (Rts2Block * in_master, char *in_name);
    Rts2Dev2DevConn (Rts2Block * in_master, Rts2Address * in_addr);
    virtual ~ Rts2Dev2DevConn (void);

  virtual int init ();
  virtual int idle ();

  virtual void setAddress (Rts2Address * in_addr);
  virtual void addressAdded (Rts2Address * in_addr);
  void connAuth ();

  virtual void setKey (int in_key);
};

class Rts2DevConn:public Rts2Conn
{
  Rts2Device *master;
  virtual int connectionError ();
protected:
    virtual int commandAuthorized ();
  int command ();
public:
    Rts2DevConn (int in_sock, Rts2Device * in_master);
  int add (fd_set * set);
  int authorizationOK ();
  int authorizationFailed ();
  void setHavePriority (int in_have_priority);
};

class Rts2DevConnMaster:public Rts2Conn
{
  char *device_host;
  char master_host[HOST_NAME_MAX];
  int master_port;
  char device_name[DEVICE_NAME_SIZE];
  int device_type;
  int device_port;
  Rts2DevConn *auth_conn;	// connection waiting for authorization
protected:
  int command ();
  int message ();
  int informations ();
  int status ();
public:
    Rts2DevConnMaster (Rts2Block * in_master,
		       char *in_device_host, int in_device_port,
		       char *in_device_name, int in_device_type,
		       char *in_master_host, int in_master_port);
  int registerDevice ();
  virtual int init ();
  int authorize (Rts2DevConn * conn);
  void setHavePriority (int in_have_priority);
};

class Rts2DevConnData:public Rts2Conn
{
  int sendHeader ();
  int acceptConn ();
  Rts2Conn *dataConn;
protected:
  int command ()
  {
    syslog (LOG_DEBUG, "Rts2DevConnData::command badCommand %s",
	    getCommand ());
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
  virtual int send (char *message);
  int send (char *data, size_t data_size);
};

class Rts2State
{
  int state;
  char *state_name;
  Rts2Device *master;
public:
    Rts2State (Rts2Device * in_master, char *in_state_name)
  {
    int len;
      master = in_master;
      len = strlen (in_state_name) + 1;
      state_name = (char *) malloc (len);
      strncpy (state_name, in_state_name, len);
      state = 0;
  };
  virtual ~ Rts2State (void)
  {
    free (state_name);
  }
  void setState (int new_state, char *description);
  void maskState (int state_mask, int new_state, char *description);
  int sendInfo (Rts2Conn * conn, int state_num);
  int getState ()
  {
    return state;
  };
};

class Rts2Device:public Rts2Block
{
  int lockf;

  int statesSize;
  Rts2State **states;
  Rts2DevConnMaster *conn_master;
  char *centrald_host;
  int centrald_port;
  int device_port;
  char *device_name;
  int device_type;

  int log_option;

  char *device_host;

protected:
  void setStateNames (int in_states_size, char **states_names);
  /**
   * Process on option, when received from getopt () call.
   * 
   * @param in_opt  return value of getopt
   * @return 0 on success, -1 if option wasn't processed
   */
  virtual int processOption (int in_opt);

  virtual Rts2Dev2DevConn *createClientConnection (char *in_deviceName);
  virtual Rts2Conn *createClientConnection (Rts2Address * in_addr);
public:
    Rts2Device (int in_argc, char **in_argv, int in_device_type,
		int default_port, char *default_name);
    virtual ~ Rts2Device (void);
  int changeState (int state_num, int new_state, char *description);
  int maskState (int state_num, int state_mask, int new_state,
		 char *description);
  int getState (int state_num)
  {
    return states[state_num]->getState ();
  };
  virtual int init ();
  virtual int idle ();
  int authorize (Rts2DevConn * conn);
  int sendStatusInfo (Rts2DevConn * conn);
  int sendMaster (char *msg)
  {
    return conn_master->send (msg);
  }

  // callback functions for device 
  virtual int ready ();
  virtual int info ();
  virtual int baseInfo ();

  virtual int ready (Rts2Conn * conn);
  virtual int info (Rts2Conn * conn);
  virtual int baseInfo (Rts2Conn * conn);

  virtual Rts2Conn *getCentraldConn ()
  {
    return conn_master;
  };
};

#endif /* !__RTS2_DEVICE__ */

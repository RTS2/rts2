#ifndef __RTS2_DEVICE__
#define __RTS2_DEVICE__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#include <malloc.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

#include "rts2block.h"

#define RTS2_CONN_AUTH_PENDING	1
#define RTS2_CONN_AUTH_OK	2
#define RTS2_CONN_AUTH_FAILED	3

#define CHECK_PRIORITY if (!havePriority ()) { sendCommandEnd (DEVDEM_E_PRIORITY, "haven't priority"); return -1; }

class Rts2Device;

class Rts2DevConn:public Rts2Conn
{
  Rts2Device *master;
  virtual int connectionError ();
protected:
    virtual int commandAuthorized ()
  {
    char *msg;
      asprintf (&msg, "devcon unknow command:'%s'", buf);
      sendCommandEnd (DEVDEM_E_SYSTEM, msg);
      free (msg);
      return -1;
  }
  int command ();
public:
  Rts2DevConn (int in_sock, Rts2Device * in_master):Rts2Conn (in_sock,
							      (Rts2Block *)
							      in_master)
  {
    master = in_master;
  };
  int add (fd_set * set);
  int authorizationOK ();
  int authorizationFailed ();
  void setHavePriority (int in_have_priority);
};

class Rts2DevConnMaster:public Rts2Conn
{
  char master_host[HOST_NAME_MAX];
  int master_port;
  char device_name[DEVICE_NAME_SIZE];
  int device_type;
  int device_port;
  Rts2DevConn *auth_conn;	// connection waiting for authorization
protected:
  int command ();
  int message ();
public:
    Rts2DevConnMaster (Rts2Block * in_master, int in_device_port,
		       char *in_device_name, int in_device_type,
		       char *in_master_host, int in_master_port);
  int registerDevice ();
  int init ();
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
  ~Rts2State (void)
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
  int statesSize;
  Rts2State **states;
  Rts2DevConnMaster *conn_master;
protected:
  void setStateNames (int in_states_size, char **states_names);
public:
    Rts2Device (int argc, char **argv, int device_type, int default_port,
		char *default_name);
   ~Rts2Device (void);
  void help (void);
  int changeState (int state_num, int new_state, char *description);
  int maskState (int state_num, int state_mask, int new_state,
		 char *description);
  int getState (int state_num)
  {
    return states[state_num]->getState ();
  };
  int init ();
  int idle ();
  int authorize (Rts2DevConn * conn);
  int sendStatusInfo (Rts2DevConn * conn);
};

#endif /* !__RTS2_DEVICE__ */

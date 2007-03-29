#ifndef __RTS2_CONN__
#define __RTS2_CONN__

#include <string>
#include <string.h>
#include <time.h>
#include <list>
#include <netinet/in.h>

#include <status.h>

#include "rts2object.h"
#include "rts2serverstate.h"
#include "rts2message.h"
#include "rts2logstream.h"

#define MAX_DATA		200

typedef enum conn_type_t
{ NOT_DEFINED_SERVER, CLIENT_SERVER, DEVICE_SERVER, DEVICE_DEVICE };

typedef enum
{ CONN_UNKNOW, CONN_RESOLVING_DEVICE, CONN_CONNECTING,
  CONN_CONNECTED, CONN_BROKEN, CONN_DELETE, CONN_AUTH_PENDING,
  CONN_AUTH_OK, CONN_AUTH_FAILED
} conn_state_t;

class Rts2Address;

class Rts2Block;

class Rts2Command;

class Rts2ClientTCPDataConn;

class Rts2DevClient;

class Rts2Event;

class Rts2Value;

class Rts2Conn:public Rts2Object
{
private:
  char buf[MAX_DATA + 2];
  char *buf_top;
  char *command_buf_top;

  conn_type_t type;
  char name[DEVICE_NAME_SIZE];	// name of device/client this connection goes to
  int key;
  int priority;			// priority - number
  int have_priority;		// priority - flag if we have priority
  int centrald_id;		// id of connection on central server
  in_addr addr;
  int port;			// local port & connection

    std::list < Rts2Command * >commandQue;
  Rts2Command *runningCommand;

  // used for monitoring of connection state..
  time_t lastGoodSend;
  time_t lastData;
  time_t lastSendReady;

  // connectionTimeout in seconds
  int connectionTimeout;
  conn_state_t conn_state;

  int message ();

protected:
    Rts2ServerState * serverState;

  Rts2Block *master;
  char *command_start;
  int sock;

  virtual int acceptConn ();

  virtual int setState (int in_value);

  void setOtherType (int other_device_type);

  Rts2DevClient *otherDevice;
  int otherType;

  void successfullSend ();
  void getSuccessSend (time_t * in_t);
  int reachedSendTimeout ();
  void successfullRead ();

  /**
   * Function called on connection error.
   *
   * @param last_data_size  < 0 when real error occurs, =0 when no more data on connection, >0 when there 
   * 	were sucessfully received data, but they were either not allowed or signaled end of connection
   */
  virtual int connectionError (int last_data_size);

public:
    Rts2Conn (Rts2Block * in_master);
    Rts2Conn (int in_sock, Rts2Block * in_master);
    virtual ~ Rts2Conn (void);

  virtual void postEvent (Rts2Event * event);

  virtual int add (fd_set * set);
  int getState ()
  {
    return serverState->getValue ();
  }
  int getErrorState ()
  {
    return getState () & DEVICE_ERROR_MASK;
  }
  /**
   * Get state without any error bits
   * 
   * \return State without error bits
   */
  int getRealState ()
  {
    return getState () & DEVICE_STATUS_MASK;
  }
  std::string getCameraChipState (int chipN);
  std::string getStateString ();
  virtual int init ()
  {
    return -1;
  }
  void postMaster (Rts2Event * event);
  virtual int idle ();

  virtual void endConnection ()
  {
    connectionError (-1);
  }

  virtual int authorizationOK ();
  virtual int authorizationFailed ();

  inline int isCommand (const char *cmd)
  {
    return !strcmp (cmd, getCommand ());
  }
  virtual int send (const char *msg);
  int send (std::string msg);
  virtual int sendMessage (Rts2Message & msg);
  int sendValue (std::string val_name, int value);
  int sendValue (std::string val_name, int val1, int val2);
  int sendValue (std::string val_name, int val1, double val2);
  int sendValue (std::string val_name, char *value);
  int sendValueRaw (std::string val_name, char *value);
  int sendValue (std::string val_name, double value);
  int sendValue (char *val_name, char *val1, int val2);
  int sendValue (char *val_name, int val1, int val2, double val3, double val4,
		 double val5, double val6);
  int sendValueTime (std::string val_name, time_t * value);
  int sendCommandEnd (int num, char *in_msg);
  virtual int processLine ();
  virtual int receive (fd_set * set);
  conn_type_t getType ()
  {
    return type;
  };
  void setType (conn_type_t in_type)
  {
    type = in_type;
  }
  int getOurAddress (struct sockaddr_in *addr);
  void setAddress (struct in_addr *in_address);
  void setPort (int in_port)
  {
    port = in_port;
  }
  void getAddress (char *addrBuf, int buf_size);
  int getLocalPort ()
  {
    return port;
  }
  const char *getName ()
  {
    return name;
  };
  int isName (const char *in_name)
  {
    return (!strcmp (getName (), in_name));
  }
  void setName (char *in_name)
  {
    strncpy (name, in_name, DEVICE_NAME_SIZE);
    name[DEVICE_NAME_SIZE - 1] = '\0';
  }
  int getKey ()
  {
    return key;
  };
  virtual void setKey (int in_key)
  {
    if (key == 0)
      key = in_key;
  }
  int havePriority ();
  void setHavePriority (int in_have_priority);
  int getHavePriority ()
  {
    return have_priority;
  };
  int getPriority ()
  {
    return priority;
  };
  void setPriority (int in_priority)
  {
    priority = in_priority;
  };
  int getCentraldId ()
  {
    return centrald_id;
  };
  void setCentraldId (int in_centrald_id);
  int sendPriorityInfo ();

  virtual int sendInfo (Rts2Conn * conn)
  {
    return -1;
  }

  int queCommand (Rts2Command * cmd);
  int queSend (Rts2Command * cmd);
  virtual int commandReturn (Rts2Command * cmd, int in_status);
  int queEmpty ()
  {
    return (runningCommand == NULL && commandQue.size () == 0);
  }
  void queClear ();

  virtual void addressUpdated (Rts2Address * in_addr)
  {
  }

  virtual void setConnState (conn_state_t new_conn_state);
  int isConnState (conn_state_t in_conn_state);
  conn_state_t getConnState ()
  {
    return conn_state;
  }

  int paramEnd ();
  int paramNextString (char **str);
  char *paramNextWholeString ();
  int paramNextStringNull (char **str);
  int paramNextInteger (int *num);
  int paramNextDouble (double *num);
  int paramNextFloat (float *num);
  int paramNextTimeval (struct timeval *tv);

  // called when some data were sucessfully received
  virtual void dataReceived (Rts2ClientTCPDataConn * dataConn);

  Rts2Block *getMaster ()
  {
    return master;
  }

  virtual void childReturned (pid_t child_pid)
  {
  }

  Rts2Value *getValue (const char *value_name);

  int getOtherType ();
  // set to -1 if we don't need timeout checks..
  void setConnTimeout (int new_connTimeout)
  {
    connectionTimeout = new_connTimeout;
  }
  int getConnTimeout ()
  {
    return connectionTimeout;
  }

  Rts2ServerState *getStateObject ()
  {
    return serverState;
  }

  Rts2DevClient *getOtherDevClient ()
  {
    return otherDevice;
  }

protected:
  virtual int command ();
  virtual void priorityChanged ();
  virtual int priorityChange ();
  int priorityInfo ();
  virtual int informations ();
  virtual int status ();
  int sendNextCommand ();

  int commandReturn ();
  inline char *getCommand ()
  {
    return command_start;
  }
  inline int isCommandReturn ()
  {
    return (*(getCommand ()) == '+' || *(getCommand ()) == '-');
  }
};

#endif /* ! __RTS2_CONN__ */

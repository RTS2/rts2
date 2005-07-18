#ifndef __RTS2_BLOCK__
#define __RTS2_BLOCK__

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>
#include <vector>
#include <list>
#include "status.h"

#include "rts2event.h"
#include "rts2object.h"
#include "rts2option.h"
#include "rts2address.h"
#include "rts2user.h"
#include "rts2devclient.h"
#include "rts2value.h"
#include "rts2app.h"

#define MSG_COMMAND             0x01
#define MSG_REPLY
#define MSG_DATA                0x04

#define MAX_CONN		20
#define MAX_DATA		200

#define MAX_STATE		5

#define USEC_SEC		1000000

typedef enum conn_type_t
{ NOT_DEFINED_SERVER, CLIENT_SERVER, DEVICE_SERVER, DEVICE_DEVICE };

typedef enum
{ CONN_UNKNOW, CONN_RESOLVING_DEVICE, CONN_CONNECTING,
  CONN_CONNECTED, CONN_BROKEN, CONN_DELETE, CONN_AUTH_PENDING,
  CONN_AUTH_OK, CONN_AUTH_FAILED
} conn_state_t;

class Rts2Block;

class Rts2Command;

class Rts2ClientTCPDataConn;

class Rts2DevClient;

class Rts2ServerState
{
public:
  char *name;
  int value;
    Rts2ServerState (char *in_name)
  {
    name = new char[strlen (in_name) + 1];
      strcpy (name, in_name);
  }
   ~Rts2ServerState (void)
  {
    delete[]name;
  }
  int isName (const char *in_name)
  {
    return !strcmp (name, in_name);
  }
  const char *getName ()
  {
    return name;
  }
};

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
  int acceptConn ();

    std::list < Rts2Command * >commandQue;
  Rts2Command *runningCommand;

  // used for monitoring of connection state..
  time_t lastGoodSend;
  time_t lastData;

  // connectionTimeout in seconds
  int connectionTimeout;
  conn_state_t conn_state;

protected:
    Rts2ServerState * serverState[MAX_STATE];

  Rts2Block *master;
  char *command_start;
  int sock;

  int setState (int in_state_num, char *in_state_name, int in_value);
  virtual int setState (char *in_state_name, int in_value);

  void setOtherType (int other_device_type);

  Rts2DevClient *otherDevice;
  int otherType;

  void successfullSend ();
  void successfullRead ();

  virtual int connectionError ();

public:
    Rts2Conn (Rts2Block * in_master);
    Rts2Conn (int in_sock, Rts2Block * in_master);
    virtual ~ Rts2Conn (void);

  virtual void postEvent (Rts2Event * event);

  int add (fd_set * set);
  virtual int getState (int state_num)
  {
    if (state_num < 0 || state_num >= MAX_STATE)
      return -1;
    if (serverState[state_num])
      return serverState[state_num]->value;
    return -1;
  }
  virtual int init ()
  {
    return -1;
  }
  void postMaster (Rts2Event * event);
  virtual int idle ();
  inline int isCommand (const char *cmd)
  {
    return !strcmp (cmd, getCommand ());
  }
  virtual int send (char *message);
  int sendValue (char *name, int value);
  int sendValue (char *name, int val1, int val2);
  int sendValue (char *name, int val1, double val2);
  int sendValue (char *name, char *value);
  int sendValue (char *name, double value);
  int sendValue (char *name, char *val1, int val2);
  int sendValue (char *name, int val1, int val2, double val3, double val4,
		 double val5, double val6);
  int sendValueTime (char *name, time_t * value);
  int sendCommandEnd (int num, char *message);
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
  void setHavePriority (int in_have_priority)
  {
    if (in_have_priority)
      send ("S priority 1 priority received");
    else
      send ("S priority 0 priority lost");
    have_priority = in_have_priority;
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
  int sendPriorityInfo (int number);

  virtual int sendInfo (Rts2Conn * conn)
  {
    return -1;
  }

  int queCommand (Rts2Command * command);
  int queSend (Rts2Command * command);
  virtual int commandReturn (Rts2Command * command, int status)
  {
    return 0;
  }
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
  int paramNextStringNull (char **str);
  int paramNextInteger (int *num);
  int paramNextDouble (double *num);
  int paramNextFloat (float *num);

  // called when some data were sucessfully received
  virtual void dataReceived (Rts2ClientTCPDataConn * dataConn);

  Rts2Block *getMaster ()
  {
    return master;
  }

  virtual void childReturned (pid_t child_pid)
  {
  }

  Rts2Value *getValue (char *value_name);

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

  void endConnection ()
  {
    connectionError ();		// maybe in future we will need to change that
  }
protected:
  virtual int command ();
  virtual int message ();
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

class Rts2Block:public Rts2App
{
  int sock;
  int port;
  long int idle_timeout;	// in msec
  int priority_client;
  int end_loop;

  // program options

  char *mailAddress;

    std::list < Rts2Address * >blockAddress;
    std::list < Rts2User * >blockUsers;

  int addConnection (int in_sock);
  int masterState;

protected:
  int deamonize;

  int addConnection (Rts2Conn * conn);
  virtual Rts2Conn *createClientConnection (char *in_deviceName) = 0;
  virtual Rts2Conn *createClientConnection (Rts2Address * in_addr) = 0;

  virtual int processOption (int in_opt);

  virtual void cancelPriorityOperations ();

  virtual void childReturned (pid_t child_pid);
  virtual int willConnect (Rts2Address * in_addr);	// determine if the device wants to connect to recently added device; returns 0 if we won't connect, 1 if we will connect

  /***
   * Address list related functions.
   **/
  virtual int addAddress (Rts2Address * in_addr);

public:
    Rts2Conn * connections[MAX_CONN];

    Rts2Block (int in_argc, char **in_argv);
    virtual ~ Rts2Block (void);
  void setPort (int in_port);
  int getPort (void);
  virtual int init ();
  void closeSockets ();

  virtual void postEvent (Rts2Event * event);

  /**
   * Used to create new connection - so childrens can
   * create childrens of Rts2Conn
   */
  virtual Rts2Conn *createConnection (int in_sock, int conn_num);
  Rts2Conn *addDataConnection (Rts2Conn * in_conn, char *in_hostname,
			       int in_port, int in_size);
  Rts2Conn *findName (const char *in_name);
  virtual int sendStatusMessage (char *state_name, int state);
  virtual int sendMessage (char *message);
  virtual int sendMessage (char *message, int val1, int val2);
  virtual int idle ();
  void setTimeout (long int new_timeout)
  {
    idle_timeout = new_timeout;
  }
  void setTimeoutMin (long int new_timeout)
  {
    if (new_timeout < idle_timeout)
      idle_timeout = new_timeout;
  }
  int endRunLoop ()
  {
    end_loop = 1;
  }
  int run ();
  virtual int deleteConnection (Rts2Conn * conn);
  int setPriorityClient (int in_priority_client, int timeout);
  void checkPriority (Rts2Conn * conn)
  {
    if (conn->getCentraldId () == priority_client)
      {
	conn->setHavePriority (1);
      }
  }
  virtual int changeMasterState (int new_state)
  {
    return 0;
  }
  int setMasterState (int new_state)
  {
    masterState = new_state;
    return changeMasterState (new_state);
  }
  int getMasterState ()
  {
    return masterState;
  }
  // status-mail related functions  
  int sendMail (char *subject, char *text);
  Rts2Address *findAddress (const char *blockName);

  void addAddress (const char *p_name, const char *p_host, int p_port,
		   int p_device_type);

  void deleteAddress (const char *p_name);

  virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
					  int other_device_type);
  void addUser (int p_centraldId, int p_priority, char p_priority_have,
		const char *p_login, const char *p_status_txt);
  int addUser (Rts2User * in_user);

  /***************************************************************
   * 
   * Return established connection to device with given name.
   *
   * Returns connection to device with deviceName. Device must be know to system.
   *
   ***************************************************************/

  Rts2Conn *getOpenConnection (char *deviceName);

  /***************************************************************
   *
   * Return connection to given device.
   * 
   * Create and return new connection if if device name isn't found
   * among connections, but is in address list.
   *
   * Cann return 'fake' client connection, which will not resolve 
   * to device name (even after 'info' call on master device).
   * For every command enqued to fake devices error handler will be
   * runned.
   *
   ***************************************************************/
  Rts2Conn *getConnection (char *deviceName);

  virtual Rts2Conn *getCentraldConn ()
  {
    return NULL;
  }

  int queAll (Rts2Command * command);
  int queAll (char *text);

  int allQuesEmpty ();

  // enables to grant priority for special device links
  virtual int grantPriority (Rts2Conn * conn)
  {
    return 0;
  }
};

#endif /*! __RTS2_NETBLOCK__ */

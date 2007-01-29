#ifndef __RTS2_BLOCK__
#define __RTS2_BLOCK__

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>
#include <list>
#include "status.h"

#include <sstream>

#include "rts2event.h"
#include "rts2object.h"
#include "rts2conn.h"
#include "rts2option.h"
#include "rts2address.h"
#include "rts2user.h"
#include "rts2devclient.h"
#include "rts2value.h"
#include "rts2app.h"
#include "rts2serverstate.h"

// protocol specific commands
#define PROTO_VALUE		"V"
#define PROTO_DATA		"D"
#define PROTO_AUTH		"A"
#define PROTO_PRIORITY		"P"
#define PROTO_PRIORITY_INFO	"Q"
#define PROTO_INFO		"I"
#define PROTO_STATUS		"S"
#define PROTO_TECHNICAL		"T"
#define PROTO_MESSAGE		"M"
#define PROTO_METAINFO		"E"

#define USEC_SEC		1000000

class Rts2Command;

class Rts2ClientTCPDataConn;

class Rts2DevClient;

class Rts2LogStream;

typedef
  std::list <
Rts2Conn * >
  connections_t;

class
  Rts2Block:
  public
  Rts2App
{
  int
    port;
  long int
    idle_timeout;		// in msec
  int
    priority_client;
  int
    end_loop;

  connections_t
    connections;

  std::list <
  Rts2Address * >
    blockAddress;
  std::list <
  Rts2User * >
    blockUsers;

  int
    masterState;

protected:

  virtual
    Rts2Conn *
  createClientConnection (char *in_deviceName) = 0;
  virtual Rts2Conn *
  createClientConnection (Rts2Address * in_addr) = 0;

  virtual void
  cancelPriorityOperations ();

  virtual void
  childReturned (pid_t child_pid);
  virtual int
  willConnect (Rts2Address * in_addr);	// determine if the device wants to connect to recently added device; returns 0 if we won't connect, 1 if we will connect

  /***
   * Address list related functions.
   **/
  virtual int
  addAddress (Rts2Address * in_addr);
  virtual void
  addSelectSocks (fd_set * read_set);
  virtual void
  selectSuccess (fd_set * read_set);
  void
  setMessageMask (int new_mask);
public:

  Rts2Block (int in_argc, char **in_argv);
  virtual ~
  Rts2Block (void);
  void
  setPort (int in_port);
  int
  getPort (void);

  void
  addConnection (Rts2Conn * conn);

  connections_t::iterator
  connectionBegin ()
  {
    return connections.begin ();
  }
  connections_t::iterator
  connectionEnd ()
  {
    return connections.end ();
  }
  int
  connectionSize ()
  {
    return connections.size ();
  }

  virtual void
  postEvent (Rts2Event * event);

  /**
   * Used to create new connection - so childrens can
   * create childrens of Rts2Conn
   */
  virtual Rts2Conn *
  createConnection (int in_sock);
  Rts2Conn *
  addDataConnection (Rts2Conn * in_conn, char *in_hostname,
		     int in_port, int in_size);
  Rts2Conn *
  findName (const char *in_name);
  Rts2Conn *
  findCentralId (int in_id);
  virtual int
  sendStatusMessage (int state);
  int
  sendAll (char *msg);
  void
  sendValueAll (char *val_name, char *value);
  int
  sendPriorityChange (int p_client, int timeout);
  // only used in centrald!
  void
  sendMessageAll (Rts2Message & msg);
  virtual int
  idle ();
  void
  setTimeout (long int new_timeout)
  {
    idle_timeout = new_timeout;
  }
  void
  setTimeoutMin (long int new_timeout)
  {
    if (new_timeout < idle_timeout)
      idle_timeout = new_timeout;
  }
  void
  endRunLoop ()
  {
    end_loop = 1;
  }
  int
  run ();
  virtual int
  deleteConnection (Rts2Conn * conn);
  int
  setPriorityClient (int in_priority_client, int timeout);
  void
  checkPriority (Rts2Conn * conn)
  {
    if (conn->getCentraldId () == priority_client)
      {
	conn->setHavePriority (1);
      }
  }
  virtual int
  changeMasterState (int new_state)
  {
    return 0;
  }
  int
  setMasterState (int new_state)
  {
    masterState = new_state;
    return changeMasterState (new_state);
  }
  int
  getMasterState ()
  {
    return masterState;
  }
  Rts2Address *
  findAddress (const char *blockName);

  void
  addAddress (const char *p_name, const char *p_host, int p_port,
	      int p_device_type);

  void
  deleteAddress (const char *p_name);

  virtual Rts2DevClient *
  createOtherType (Rts2Conn * conn, int other_device_type);
  void
  addUser (int p_centraldId, int p_priority, char p_priority_have,
	   const char *p_login);
  int
  addUser (Rts2User * in_user);

  /***************************************************************
   * 
   * Return established connection to device with given name.
   *
   * Returns connection to device with deviceName. Device must be know to system.
   *
   ***************************************************************/

  Rts2Conn *
  getOpenConnection (char *deviceName);

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
  Rts2Conn *
  getConnection (char *deviceName);

  virtual Rts2Conn *
  getCentraldConn ()
  {
    return NULL;
  }

  virtual void
  message (Rts2Message & msg);

  int
  queAll (Rts2Command * cmd);
  int
  queAll (char *text);

  int
  allQuesEmpty ();

  // enables to grant priority for special device links
  virtual int
  grantPriority (Rts2Conn * conn)
  {
    return 0;
  }

  /** 
   * Return connection with minimum (integer) value.
   */
  Rts2Conn *
  getMinConn (const char *valueName);

  virtual void
  centraldConnRunning ()
  {
  }

  virtual void
  centraldConnBroken ()
  {
  }
};

#endif /*! __RTS2_NETBLOCK__ */

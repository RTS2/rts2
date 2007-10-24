#ifndef __RTS2_CLIENT__
#define __RTS2_CLIENT__

#include "rts2block.h"
#include "rts2command.h"
#include "rts2devclient.h"

/**
 * @defgroup RTS2Client
 *
 * This module includes infrastructure needed for client applications.
 */

/**
 * Represents connection between device and client.
 *
 * This class is used in Rts2Client to represent connections
 * which client establish to other RTS2 components.
 *
 * @ingroup RTS2Client
 */
class Rts2ConnClient:public Rts2Conn
{
private:
  Rts2Address * address;
public:
  Rts2ConnClient (Rts2Block * in_master, char *in_name);
    virtual ~ Rts2ConnClient (void);

  virtual int init ();
  virtual int idle ();

  virtual void setAddress (Rts2Address * in_addr);
  void connLogin ();

  /**
   * Set client key.
   *
   * Keys are distributed from Rts2Centrald to client connection.
   *
   * @param in_key New client key.
   */
  virtual void setKey (int in_key);
};

/**
 * Represents connection between centrald and client.
 *
 * This class is used to represent conection between central
 * server and client. It is used to handle key management etc.
 *
 * @ingroup RTS2Client
 */
class Rts2ConnCentraldClient:public Rts2Conn
{
private:
  const char *master_host;
  const char *master_port;

  const char *login;
  const char *password;

public:
    Rts2ConnCentraldClient (Rts2Block * in_master, const char *in_login,
			    const char *in_password,
			    const char *in_master_host,
			    const char *in_master_port);
  virtual int init ();

  virtual int command ();
  virtual int status ();
};

/*class Rts2Thread:public Rts2Conn
{


};*/

class Rts2CommandLogin:public Rts2Command
{
private:
  const char *login;
  const char *password;
private:
  enum
  { LOGIN_SEND, PASSWORD_SEND, INFO_SEND } state;
public:
    Rts2CommandLogin (Rts2Block * master, const char *in_login,
		      const char *in_password);
  virtual int commandReturnOK (Rts2Conn * conn);
};

/**
 * Common class for client aplication.
 *
 * Connect to centrald, get names of all devices.
 * Works similary to Rts2Device
 *
 * @ingroup RTS2Block RTS2Client
 */
class Rts2Client:public Rts2Block
{
private:
  Rts2ConnCentraldClient * central_conn;
  char *central_host;
  char *central_port;
  char *login;
  char *password;

protected:
    virtual Rts2ConnClient * createClientConnection (char *in_deviceName);
  virtual Rts2Conn *createClientConnection (Rts2Address * in_addr);
  virtual int willConnect (Rts2Address * in_addr);

  virtual int processOption (int in_opt);
  virtual int init ();

  virtual Rts2ConnCentraldClient *createCentralConn ();

  const char *getCentralLogin ()
  {
    return login;
  }
  const char *getCentralPassword ()
  {
    return password;
  }
  const char *getCentralHost ()
  {
    return central_host;
  }
  const char *getCentralPort ()
  {
    return central_port;
  }
public:
  Rts2Client (int in_argc, char **in_argv);
  virtual ~ Rts2Client (void);

  virtual int run ();

  virtual Rts2Conn *getCentraldConn ()
  {
    return central_conn;
  }

  std::string getMasterStateString ();
};

#endif /* ! __RTS2_CLIENT__ */

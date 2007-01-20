#ifndef __RTS2_CLIENT__
#define __RTS2_CLIENT__

#include "rts2block.h"
#include "rts2command.h"
#include "rts2devclient.h"

/**
 * Defines client specific thinks (client connections, handle client
 * thread management, ..
 *
 */

class Rts2ConnClient:public Rts2Conn
{
private:
  Rts2Address * address;
protected:
/*  virtual int command ();
  virtual int message ();
  virtual int informations ();
  virtual int status ();
  virtual int commandReturn (); */
public:
  Rts2ConnClient (Rts2Block * in_master, char *in_name);
    virtual ~ Rts2ConnClient (void);

  virtual int init ();
  virtual int idle ();

  virtual void setAddress (Rts2Address * in_addr);
  void connLogin ();

  virtual void setKey (int in_key);
};

class Rts2ConnCentraldClient:public Rts2Conn
{
  char *master_host;
  char *master_port;

  char *login;
  char *password;

public:
    Rts2ConnCentraldClient (Rts2Block * in_master, char *in_login,
			    char *in_password, char *in_master_host,
			    char *in_master_port);
  virtual int init ();

  virtual int command ();
  virtual int informations ();
  virtual int status ();
};

/*class Rts2Thread:public Rts2Conn
{


};*/

class Rts2CommandLogin:public Rts2Command
{
  char *login;
  char *password;
private:
  enum
  { LOGIN_SEND, PASSWORD_SEND, INFO_SEND } state;
public:
    Rts2CommandLogin (Rts2Block * master, char *in_login, char *in_password);
  virtual int commandReturnOK ();
};

/**************************************************
 *
 * Common block for client aplication.
 *
 * Connect to centrald, get names of all devices.
 * Works similary to Rts2Device
 *
 *************************************************/

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
public:
    Rts2Client (int in_argc, char **in_argv);
    virtual ~ Rts2Client (void);

  virtual int processOption (int in_opt);
  virtual int init ();

  virtual Rts2Conn *getCentraldConn ()
  {
    return central_conn;
  }

  std::string getMasterStateString ();
};

#endif /* ! __RTS2_CLIENT__ */

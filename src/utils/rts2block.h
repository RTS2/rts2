#ifndef __RTS2_BLOCK__
#define __RTS2_BLOCK__

#include <string.h>
#include "status.h"

#define MSG_COMMAND             0x01
#define MSG_REPLY               0x02
#define MSG_DATA                0x04

#define MAX_CONN		20
#define MAX_DATA		200

typedef enum conn_type_t
{ NOT_DEFINED_SERVER, CLIENT_SERVER, DEVICE_SERVER };

class Rts2Conn
{
  int sock;
  int data_size;
  conn_type_t type;
  char name[DEVICE_NAME_SIZE];	// name of device/client this connection goes to
  int key;
  int priority;			// priority - number
  int have_priority;		// priority - flag if we have priority
  int centrald_id;		// id of connection on central server
protected:
  char buf[MAX_DATA + 1];
  char *buf_top;
public:
    Rts2Conn (int in_sock);
  int add (fd_set * set);
  int send (char *message, ...);
  int sendCommandEnd (int num, char *message, ...);
  int receive (fd_set * set);
  conn_type_t getType ()
  {
    return type;
  };
  void setType (conn_type_t in_type)
  {
    type = in_type;
  }
  const char *getName ()
  {
    return name;
  };
  void setName (char *in_name)
  {
    strncpy (name, in_name, DEVICE_NAME_SIZE);
  };
  int getKey ()
  {
    return key;
  };
  void setKey (int in_key)
  {
    key = in_key;
  };
  int havePriority ()
  {
    return have_priority;
  };
  void setHavePriority (int in_have_priority)
  {
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
  void setCentraldId (int in_centrald_id)
  {
    centrald_id = in_centrald_id;
  };
  virtual int sendInfo (Rts2Conn * conn)
  {
    return -1;
  };
private:
  virtual int command ();
protected:
  int paramEnd ();
  int paramNextString (char **str);
  int paramNextInteger (int *num);
  int paramNextDouble (double *num);
};

class Rts2Block
{
  int sock;
  int port;
  long int idle_timeout;	// in msec

public:
    Rts2Conn * connections[MAX_CONN];

    Rts2Block (int in_port);
   ~Rts2Block (void);
  int init ();
  virtual int addConnection (int in_sock);
  Rts2Conn *findName (char *in_name);
  int sendMessage (char *format, ...);
  virtual int idle ();
  int run ();
};

#endif /*! __RTS2_NETBLOCK__ */

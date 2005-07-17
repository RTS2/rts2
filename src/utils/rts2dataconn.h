#ifndef __RTS2_DATACONN__
#define __RTS2_DATACONN__

#include "rts2connnosend.h"
#include "imghdr.h"

/***************************************************
 *
 * Contains definitions of data connection.
 *
 * Data connections are used to transport data, mostly images, from
 * one part of RTS2 to the other.
 *
 * Those classes are not to be inherited from; if you need handle data,
 * overwrite dataAvailable method from connection instead.
 *
 * If you inherit from that class, you put yourself in danger when
 * some new way of transport will be added.
 *
 **************************************************/

class Rts2ClientTCPDataConn:public Rts2ConnNoSend
{
private:
  char *data;
  char *dataTop;
  int receivedSize;
  int totalSize;
  Rts2Conn *ownerConnection;
public:
    Rts2ClientTCPDataConn (Rts2Block * in_master, Rts2Conn * in_owner_conn,
			   char *hostname, int port, int in_totalSize);
    virtual ~ Rts2ClientTCPDataConn (void);

  virtual int receive (fd_set * set);
  virtual int idle ();

  virtual void dataReceived ();
  unsigned short *getData ()
  {
    return (unsigned short *) (data + sizeof (imghdr));
  }
  int getSize ()
  {
    return receivedSize - sizeof (imghdr);
  }
  unsigned short *getTop ()
  {
    return (unsigned short *) (data + receivedSize);
  }
  struct imghdr *getImageHeader ()
  {
    return (struct imghdr *) data;
  }
};

#endif /* !__RTS2_DATACONN__ */

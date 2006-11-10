#ifndef __RTS2_DAEMON__
#define __RTS2_DAEMON__

#include "rts2block.h"
#include "rts2logstream.h"

/**
 * Abstract class for centrald and all devices.
 *
 * This class contains functions which are common to components with one listening socket.
 */
class Rts2Daemon:public Rts2Block
{
private:
  bool daemonize;
  int listen_sock;
  int addConnection (int in_sock);
protected:
    virtual void addSelectSocks (fd_set * read_set);
  virtual void selectSuccess (fd_set * read_set);
public:
    Rts2Daemon (int in_argc, char **in_argv);
    virtual ~ Rts2Daemon (void);
  virtual int processOption (int in_opt);
  virtual int init ();
  virtual void forkedInstance ();
  int addConnection (Rts2Conn * conn)
  {
    return Rts2Block::addConnection (conn);
  }

  // only deamons can send messages
  virtual void sendMessage (messageType_t in_messageType,
			    const char *in_messageString) = 0;
  inline void sendMessage (messageType_t in_messageType,
			   std::ostringstream & _os);

  Rts2LogStream logStream (messageType_t in_messageType);
};

#endif /* ! __RTS2_DAEMON__ */

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
  // 0 - don't deamonize, 1 - do deamonize, 2 - is already deamonized, 3 - deamonized & centrald is running, don't print to stdout
  enum
  { DONT_DEAMONIZE, DO_DEAMONIZE, IS_DEAMONIZED, CENTRALD_OK } daemonize;
  int listen_sock;
  void addConnectionSock (int in_sock);
  int lockf;
protected:
  int checkLockFile (const char *lock_fname);
  int doDeamonize ();
  int lockFile ();
  virtual void addSelectSocks (fd_set * read_set);
  virtual void selectSuccess (fd_set * read_set);
public:
    Rts2Daemon (int in_argc, char **in_argv);
    virtual ~ Rts2Daemon (void);
  virtual int processOption (int in_opt);
  virtual int init ();
  void initDaemon ();
  virtual int run ();
  virtual void forkedInstance ();
  virtual void sendMessage (messageType_t in_messageType,
			    const char *in_messageString);
  virtual void centraldConnRunning ();
  virtual void centraldConnBroken ();
};

#endif /* ! __RTS2_DAEMON__ */

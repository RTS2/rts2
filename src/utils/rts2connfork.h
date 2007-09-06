#ifndef __RTS2_CONN_FORK__
#define __RTS2_CONN_FORK__

#include "rts2connnosend.h"

class Rts2ConnFork:public Rts2ConnNoSend
{
private:
  pid_t childPid;
  time_t forkedTimeout;

protected:
  char *exePath;
  virtual void connectionError (int last_data_size);
  virtual void beforeFork ();
  /**
   * Called when initialization of the connection fails at some point.
   */
  virtual void initFailed ();
public:
    Rts2ConnFork (Rts2Block * in_master, const char *in_exe, int in_timeout =
		  0);
    virtual ~ Rts2ConnFork (void);

  virtual int newProcess ();

  virtual int init ();
  virtual void stop ();
  void term ();

  virtual int idle ();

  virtual void childReturned (pid_t in_child_pid);

  virtual void childEnd ()
  {
    // insert here some post-processing
  }
};

#endif /* !__RTS2_CONN_FORK__ */

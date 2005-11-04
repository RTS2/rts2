#ifndef __RTS2_CONN_FORK__
#define __RTS2_CONN_FORK__

#include "rts2connnosend.h"

class Rts2ConnFork:public Rts2ConnNoSend
{
private:
  pid_t childPid;

protected:
  char *exePath;
  virtual int connectionError (int last_data_size);
public:
    Rts2ConnFork (Rts2Block * in_master, const char *in_exe);
    virtual ~ Rts2ConnFork (void);

  virtual int newProcess ();

  virtual int init ();
  virtual void stop ();

  virtual void childReturned (pid_t in_child_pid);

  virtual void childEnd ()
  {
    // insert here some post-processing
  }
};

#endif /* !__RTS2_CONN_FORK__ */

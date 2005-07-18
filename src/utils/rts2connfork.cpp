#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rts2connfork.h"

#include <signal.h>
#include <fcntl.h>

Rts2ConnFork::Rts2ConnFork (Rts2Block * in_master, const char *in_exe):
Rts2ConnNoSend (in_master)
{
  childPid = -1;
  exePath = new char[strlen (in_exe) + 1];
  strcpy (exePath, in_exe);
}

Rts2ConnFork::~Rts2ConnFork (void)
{
  if (childPid > 0)
    kill (childPid, SIGINT);
  if (exePath)
    delete[]exePath;
}


int
Rts2ConnFork::newProcess ()
{
  // create new process with execl..
  // now empty
  return -1;
}

int
Rts2ConnFork::init ()
{
  int ret;
  if (childPid > 0)
    {
      // continue
      kill (childPid, SIGCONT);
      return 1;
    }
  int filedes[2];
  ret = pipe (filedes);
  if (ret)
    {
      syslog (LOG_ERR,
	      "Rts2ConnImgProcess::run cannot create pipe for process: %m");
      return -1;
    }
  childPid = fork ();
  if (childPid == -1)
    {
      syslog (LOG_ERR, "Rts2ConnImgProcess::run cannot fork: %m");
      return -1;
    }
  else if (childPid)		// parent
    {
      sock = filedes[0];
      close (filedes[1]);
      fcntl (sock, F_SETFL, O_NONBLOCK);
      return 0;
    }
  // child
  close (filedes[0]);
  dup2 (filedes[1], 1);
  // close all sockets so when we crash, we don't get any dailing
  // sockets
  master->closeSockets ();
  ret = newProcess ();
  if (ret)
    syslog (LOG_ERR, "Rts2ConnFork::init newProcess return : %i %m", ret);
  exit (0);
}

void
Rts2ConnFork::stop ()
{
  if (childPid > 0)
    kill (childPid, SIGSTOP);
}

void
Rts2ConnFork::childReturned (pid_t in_child_pid)
{
  if (childPid == in_child_pid)
    {
      childEnd ();
      childPid = -1;
      endConnection ();
    }
}

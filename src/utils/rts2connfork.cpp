#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rts2connfork.h"

#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

Rts2ConnFork::Rts2ConnFork (Rts2Block * in_master, const char *in_exe,
			    int in_timeout):
Rts2ConnNoSend (in_master)
{
  childPid = -1;
  exePath = new char[strlen (in_exe) + 1];
  strcpy (exePath, in_exe);
  if (in_timeout > 0)
    {
      time (&forkedTimeout);
      forkedTimeout += in_timeout;
    }
  else
    {
      forkedTimeout = 0;
    }
}

Rts2ConnFork::~Rts2ConnFork (void)
{
  if (childPid > 0)
    kill (-childPid, SIGINT);
  delete[]exePath;
}

int
Rts2ConnFork::connectionError (int last_data_size)
{
  if (last_data_size < 0 && errno == EAGAIN)
    {
      logStream (MESSAGE_DEBUG) <<
	"Rts2ConnFork::connectionError reported EAGAIN - that should not happen, ignoring"
	<< sendLog;
      return 1;
    }
  return Rts2Conn::connectionError (last_data_size);
}

void
Rts2ConnFork::beforeFork ()
{
}

void
Rts2ConnFork::initFailed ()
{
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
      initFailed ();
      return 1;
    }
  int filedes[2];
  ret = pipe (filedes);
  if (ret)
    {
      logStream (MESSAGE_ERROR) <<
	"Rts2ConnImgProcess::run cannot create pipe for process: " <<
	strerror (errno) << sendLog;
      initFailed ();
      return -1;
    }
  // do everything that will be needed to done before forking
  beforeFork ();
  childPid = fork ();
  if (childPid == -1)
    {
      logStream (MESSAGE_ERROR) << "Rts2ConnImgProcess::run cannot fork: " <<
	strerror (errno) << sendLog;
      initFailed ();
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
  master->forkedInstance ();
  // start new group, so SIGTERM to group will kill all children
  setpgrp ();

  ret = newProcess ();
  if (ret)
    logStream (MESSAGE_ERROR) << "Rts2ConnFork::init newProcess return : " <<
      ret << " " << strerror (errno) << sendLog;
  exit (0);
}

void
Rts2ConnFork::stop ()
{
  if (childPid > 0)
    kill (-childPid, SIGSTOP);
}

void
Rts2ConnFork::term ()
{
  if (childPid > 0)
    kill (-childPid, SIGKILL);
  childPid = -1;
}

int
Rts2ConnFork::idle ()
{
  if (forkedTimeout > 0)
    {
      time_t now;
      time (&now);
      if (now > forkedTimeout)
	{
	  term ();
	  endConnection ();
	}
    }
  return 0;
}

void
Rts2ConnFork::childReturned (pid_t in_child_pid)
{
  if (childPid == in_child_pid)
    {
      childEnd ();
      childPid = -1;
      // endConnection will be called after read from pipe fails
    }
}

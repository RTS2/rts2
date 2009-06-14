/* 
 * Forking connection handling.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "connfork.h"

#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

using namespace rts2core;

ConnFork::ConnFork (Rts2Block *_master, const char *_exe, bool _fillConnEnvVars, int _timeout):
Rts2ConnNoSend (_master)
{
	childPid = -1;
	sockerr = -1;
	exePath = new char[strlen (_exe) + 1];
	strcpy (exePath, _exe);
	if (_timeout > 0)
	{
		time (&forkedTimeout);
		forkedTimeout += _timeout;
	}
	else
	{
		forkedTimeout = 0;
	}
	fillConnEnvVars = _fillConnEnvVars;
}


ConnFork::~ConnFork (void)
{
	if (childPid > 0)
		kill (-childPid, SIGINT);
	if (sockerr > 0)
		close (sockerr);
	delete[]exePath;
}


int
ConnFork::add (fd_set * readset, fd_set * writeset, fd_set * expset)
{
	if (sockerr > 0)
		FD_SET (sockerr, readset);
	return Rts2ConnNoSend::add (readset, writeset, expset);
}


int
ConnFork::receive (fd_set * readset)
{
	if (sockerr > 0 && FD_ISSET (sockerr, readset))
	{
		int data_size;
		char errbuf[5001];
		data_size = read (sockerr, errbuf, 5000);
		if (data_size > 0)
		{
			errbuf[data_size] = '\0';
			processErrorLine (errbuf);	
		}
		else if (data_size == 0)
		{
			close (sockerr);
			sockerr = -1;
		}
		else
		{
			logStream (MESSAGE_ERROR) << "From error pipe read error " << strerror (errno) << "." << sendLog;
		}
	}
	return Rts2ConnNoSend::receive (readset);
}


void
ConnFork::connectionError (int last_data_size)
{
	if (last_data_size < 0 && errno == EAGAIN)
	{
		logStream (MESSAGE_DEBUG) <<
			"ConnFork::connectionError reported EAGAIN - that should not happen, ignoring"
			<< sendLog;
		return;
	}
	Rts2Conn::connectionError (last_data_size);
}


void
ConnFork::beforeFork ()
{
}


void
ConnFork::initFailed ()
{
}


void
ConnFork::processErrorLine (char *errbuf)
{
	logStream (MESSAGE_ERROR) << "From error pipe received: " << errbuf << "." << sendLog;
}


int
ConnFork::newProcess ()
{
	return execl (exePath, NULL);
}


int
ConnFork::init ()
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
	int filedeserr[2];
	ret = pipe (filedes);
	if (ret)
	{
		logStream (MESSAGE_ERROR) <<
			"Rts2ConnImgProcess::run cannot create pipe for process: " <<
			strerror (errno) << sendLog;
		initFailed ();
		return -1;
	}
	ret = pipe (filedeserr);
	if (ret)
	{
		logStream (MESSAGE_ERROR) <<
			"Rts2ConnImgProcess::run cannot create error pipe for process: " <<
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
	else if (childPid)			 // parent
	{
		sock = filedes[0];
		close (filedes[1]);
		sockerr = filedeserr[0];
		close (filedeserr[1]);
		fcntl (sock, F_SETFL, O_NONBLOCK);
		fcntl (sockerr, F_SETFL, O_NONBLOCK);
		return 0;
	}
	// child
	close (1);
	close (2);

	close (filedes[0]);
	dup2 (filedes[1], 1);
	close (filedeserr[0]);
	dup2 (filedeserr[1], 2);

	// if required, pass environemnt values about connections..
	if (fillConnEnvVars)
	{
		connections_t *conns = getMaster () ->getConnections ();
		for (connections_t::iterator citer = conns->begin (); citer != conns->end (); citer++)
		{
			Rts2Conn *conn = *citer;
			for (Rts2ValueVector::iterator viter = conn->valueBegin (); viter != conn->valueEnd (); viter++)
			{
				Rts2Value *val = (*viter);
				char *envV = new char [strlen (conn->getName ()) + val->getName ().length () + strlen (val->getValue ()) + 3];
				sprintf (envV, "%s_%s=%s", conn->getName (), val->getName ().c_str (), val->getValue ());
				putenv (envV);
			}
		}
	}

	// close all sockets so when we crash, we don't get any dailing
	// sockets
	master->forkedInstance ();
	// start new group, so SIGTERM to group will kill all children
	setpgrp ();

	ret = newProcess ();
	if (ret)
		logStream (MESSAGE_ERROR) << "ConnFork::init newProcess return : " <<
			ret << " " << strerror (errno) << sendLog;
	exit (0);
}


void
ConnFork::stop ()
{
	if (childPid > 0)
		kill (-childPid, SIGSTOP);
}


void
ConnFork::term ()
{
	if (childPid > 0)
		kill (-childPid, SIGKILL);
	childPid = -1;
}


int
ConnFork::idle ()
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
ConnFork::childReturned (pid_t in_child_pid)
{
	if (childPid == in_child_pid)
	{
		childEnd ();
		childPid = -1;
		// endConnection will be called after read from pipe fails
	}
}

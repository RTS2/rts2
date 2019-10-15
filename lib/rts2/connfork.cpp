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

#include "connection/fork.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

using namespace rts2core;

ConnFork::ConnFork (rts2core::Block *_master, const char *_exe, bool _fillConnEnvVars, bool _openin, int _timeout):ConnNoSend (_master)
{
	childPid = -1;
	sockerr = -1;
	if (_openin)
		sockwrite = -2;
	else
		sockwrite = -1;

	// Parse command string into executable proper and arguments
	char *pos = (char *)_exe;
	bool finished = false;
	int count = 0;

	while (!finished)
	{
		char *start = pos; // Start of token
		char endChar = 0;

		while (isspace(*pos) && *pos)
		{
			pos++;
			start++;
		}

		if (*pos == '"' || *pos == '\'')
		{
			// Quoted string started
			endChar = *pos;
			pos++;
			start++;
		}

		while (((endChar == 0 && !isspace(*pos)) ||
				(endChar && *pos != endChar)) && *pos)
			pos++;

		if (!*pos)
			finished = true;
		else
		{
			*pos = '\0';
			pos++;
		}

		if (pos > start)
		{
			// Non-empty token
			if (count == 0)
			{
				exePath = new char[pos - start + 1];
				strcpy (exePath, start);
			}
			else
			{
				addArg (start);
			}
		}

		count ++;
	}

	connDebug = false;

	forkedTimeout = _timeout;
	time (&startTime);
	endTime = 0;

	fillConnEnvVars = _fillConnEnvVars;
}

ConnFork::~ConnFork ()
{
	if (childPid > 0)
		kill (-childPid, SIGTERM);
	if (sockerr > 0)
		close (sockerr);
	if (sockwrite > 0)
		close (sockwrite);
	delete[] exePath;
}

int ConnFork::writeToProcess (const char *msg)
{
	if (sockwrite < 0)
		return 0;

	std::string completeMessage = input + msg + "\n";

	int l = completeMessage.length ();
	int ret = write (sockwrite, completeMessage.c_str (), l);

	if (ret < 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot write to process " << getExePath () << ": " << strerror (errno) << sendLog;
		connectionError (-1);
		return -1;
	}
	else if (connDebug)
	{
		logStream (MESSAGE_DEBUG) << "wrote to process " << getExePath () << ":" << completeMessage.substr (0, ret) << sendLog;
	}
	setInput (completeMessage.substr (ret));
	return 0;
}

int ConnFork::writeToProcessInt (int msg)
{
	std::ostringstream os;
	os << msg;
	return writeToProcess (os.str ().c_str ());
}

void ConnFork::processLine ()
{
	if (connDebug)
	{
		logStream (MESSAGE_DEBUG) << "read from process: " << command_buf_top << sendLog;
	}
}

int ConnFork::add (Block *block)
{
	if (sock > 0)
		block->addPollFD (sock, POLLIN | POLLPRI | POLLHUP);
	if (sockerr > 0)
		block->addPollFD (sockerr, POLLIN | POLLPRI | POLLHUP);
	if (input.length () > 0)
	{
		if (sockwrite < 0)
		{
			logStream (MESSAGE_ERROR) << "asked to write data after write sock was closed" << sendLog;
			input = std::string ("");
		}
		else
		{
			block->addPollFD (sockwrite, POLLOUT);
		}
	}
	return 0;
}

int ConnFork::receive (Block *block)
{
	if (sockerr > 0)
	{
		if (block->isForRead (sockerr) || block->isHup (sockerr))
		{
			int data_size;
			char errbuf[5001];
			data_size = read (sockerr, errbuf, 5000);
			if (data_size > 0)
			{
				errbuf[data_size] = '\0';
				processErrorLine (errbuf);
				return 0;
			}
			else if (data_size == 0)
			{
				close (sockerr);
				sockerr = -1;
				return 0;
			}
			else
			{
				logStream (MESSAGE_ERROR) << "From error pipe read error " << strerror (errno) << "." << sendLog;
				connectionError (-1);
				return -1;
			}
		}
	}
	return ConnNoSend::receive (block);
}

int ConnFork::writable (Block *block)
{
	if (input.length () > 0)
	{
		if (sockwrite < 0)
		{
			connectionError (-1);
			return -1;
		}
	 	if (block->isForWrite (sockwrite))
		{
			int write_size = write (sockwrite, input.c_str (), input.length ());
			if (write_size < 0)
			{
				if (errno == EINTR)
				{
					logStream (MESSAGE_ERROR) << "rts2core::ConnFork while writing to sockwrite: " << strerror (errno) << sendLog;
					close (sockwrite);
					sockwrite = -1;
					return -1;
				}
				logStream (MESSAGE_WARNING) << "rts2core::ConnFork cannot write to process, will try again: " << strerror (errno) << sendLog;
				return 0;
			}
			input = input.substr (write_size);
			if (input.length () == 0)
			{
				write_size = close (sockwrite);
				if (write_size < 0)
					logStream (MESSAGE_ERROR) << "rts2core::ConnFork error while closing write descriptor: " << strerror (errno) << sendLog;
				sockwrite = -1;
			}
		}
	}
	return ConnNoSend::writable (block);
}

void ConnFork::connectionError (int last_data_size)
{
	if (last_data_size < 0 && errno == EAGAIN && sockwrite > 0)
	{
		logStream (MESSAGE_DEBUG) << "ConnFork::connectionError reported EAGAIN - that should not happen, ignoring it " << getpid () << sendLog;
		return;
	}

	Connection::connectionError (last_data_size);
}

void ConnFork::beforeFork ()
{
	time (&startTime);
	if (forkedTimeout > 0)
		endTime = startTime + forkedTimeout;
}

void ConnFork::initFailed ()
{
}

void ConnFork::processErrorLine (char *errbuf)
{
	logStream (MESSAGE_ERROR) << exePath << " error stream: " << errbuf << sendLog;
}

int ConnFork::newProcess ()
{
	char * args[argv.size () + 2];
	args[0] = exePath;
	for (size_t i = 0; i < argv.size (); i++)
		args[i + 1] = (char *) argv[i].c_str ();
	args[argv.size () + 1] = NULL;
	return execv (exePath, args);
}

void ConnFork::fillConnectionEnv (Connection *conn, const char *prefix)
{
	std::ostringstream _os;
	_os << conn->getState ();

	// put to environment device state
	char *envV = new char [strlen (prefix) + _os.str ().length () + 8];
	sprintf (envV, "%s_state=%s", prefix, _os.str ().c_str ());
	putenv (envV);

	for (rts2core::ValueVector::iterator viter = conn->valueBegin (); viter != conn->valueEnd (); viter++)
	{
		rts2core::Value *val = (*viter);
		// replace non-alpha characters
		std::string valn = val->getName ();
		for (std::string::iterator siter = valn.begin (); siter != valn.end (); siter++)
		{
			if (!isalnum (*siter))
				(*siter) = '_';
		}

		std::ostringstream os;
		os << prefix << "_" << valn << "=" << val->getDisplayValue ();
		envV = new char [os.str ().length () + 1];
		memcpy (envV, os.str ().c_str (), os.str ().length ());
		envV[os.str ().length ()] = '\0';
		putenv (envV);
	}
}

int ConnFork::init ()
{
	int ret;
	if (childPid > 0)
	{
		// continue
		kill (childPid, SIGCONT);
		initFailed ();
		return 1;
	}
	// first check if exePath exist and we have run permissions..
	if (access (exePath, X_OK))
	{
		logStream (MESSAGE_ERROR) << "execution of " << exePath << " failed with error: " << strerror (errno) << sendLog;
		return -1;
	}
	int filedes[2];
	int filedeserr[2];
	int filedeswrite[2];
	ret = pipe (filedes);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "ConnFork::init cannot create pipe for process: " << strerror (errno) << sendLog;
		initFailed ();
		return -1;
	}
	ret = pipe (filedeserr);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "ConnFork::init cannot create error pipe for process: " << strerror (errno) << sendLog;
		close (filedes[0]);
		close (filedes[1]);
		initFailed ();
		return -1;
	}

	if (sockwrite == -2)
	{
		ret = pipe (filedeswrite);
		if (ret)
		{
		  	logStream (MESSAGE_ERROR) << "ConnFork::init cannot create write pipe for process: " << strerror (errno) << sendLog;
			close (filedes[0]);
			close (filedes[1]);
			close (filedeserr[0]);
			close (filedeserr[1]);
			initFailed ();
			return -1;
		}
	}

	char *home = getenv ("HOME");
	// FIXME: this is a quite crude workaround that should probably be done in some other way?
	if ((!home || !*home) && (getuid () == 0))
		home = "/root"; // Fallback for starting from root environments not providing HOME, e.g. systemd

	// do everything that will be needed to done before forking
	beforeFork ();
	childPid = fork ();
	if (childPid == -1)
	{
		logStream (MESSAGE_ERROR) << "ConnFork::init cannot fork: " << strerror (errno) << sendLog;
		close (filedes[0]);
		close (filedes[1]);
		close (filedeserr[0]);
		close (filedeserr[1]);
		if (sockwrite > 0)
		{
			close (filedeswrite[0]);
			close (filedeswrite[1]);
		}

		initFailed ();
		return -1;
	}
	else if (childPid)			 // parent
	{
		sock = filedes[0];
		close (filedes[1]);
		sockerr = filedeserr[0];
		close (filedeserr[1]);

		if (sockwrite == -2)
		{
			sockwrite = filedeswrite[1];
			close (filedeswrite[0]);
			fcntl (sockwrite, F_SETFL, O_NONBLOCK);
		}
		fcntl (sock, F_SETFL, O_NONBLOCK);
		fcntl (sockerr, F_SETFL, O_NONBLOCK);

		return 0;
	}
	// child
	close (0);
	close (1);
	close (2);

	// Better set HOME variable as some scripts (e.g. AstroPy based ones) expect it to be set
	if (home && *home)
		setenv ("HOME", home, 1);

	if (sockwrite == -2)
	{
		close (filedeswrite[1]);
		dup2 (filedeswrite[0], 0);
		close (filedeswrite[0]);
	}

	close (filedes[0]);
	dup2 (filedes[1], 1);
	close (filedes[1]);

	close (filedeserr[0]);
	dup2 (filedeserr[1], 2);
	close (filedeserr[1]);

	// if required, pass environemnt values about connections..
	if (fillConnEnvVars)
	{
		connections_t *conns;
		connections_t::iterator citer;
		// fill centrald variables..
		conns = getMaster ()->getCentraldConns ();
		int i = 1;
		for (citer = conns->begin (); citer != conns->end (); citer++, i++)
		{
	  		std::ostringstream _os;
			_os << "centrald" << i;
			fillConnectionEnv (*citer, _os.str ().c_str ());
		}

		conns = getMaster () ->getConnections ();
		for (citer = conns->begin (); citer != conns->end (); citer++)
			fillConnectionEnv (*citer, (*citer)->getName ());
	}

	// close all sockets so when we crash, we don't get any dailing
	// sockets
	master->forkedInstance ();
	// start new group, so SIGTERM to group will kill all children
	setpgrp ();

	ret = newProcess ();
	if (ret)
		logStream (MESSAGE_ERROR) << "ConnFork::init " << exePath << " newProcess return : " <<
			ret << " " << strerror (errno) << sendLog;
	exit (0);
}

void ConnFork::stop ()
{
	if (childPid > 0)
		kill (-childPid, SIGSTOP);
}

void ConnFork::terminate ()
{
	if (childPid > 0)
		kill (-childPid, SIGKILL);
	childPid = -1;
}

int ConnFork::idle ()
{
	if (childPid > 0 && endTime > 0)
	{
		time_t now;
		time (&now);
		if (now > endTime)
		{
		  	endTime = now;
			logStream (MESSAGE_WARNING) << "killing " << exePath << " (" << childPid << "), as it reached timeout" << sendLog;
			terminate ();
			endConnection ();
		}
	}
	return 0;
}

void ConnFork::childReturned (pid_t in_child_pid)
{
	if (childPid == in_child_pid)
	{
		childEnd ();
		time (&endTime);
		childPid = -1;
		// endConnection will be called after read from pipe fails
	}
}

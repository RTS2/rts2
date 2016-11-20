/* 
 * Container holding multiple daemons.
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics AS CR  <kubanek@fzu.cz>
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

#include "multidev.h"

#include <signal.h>

using namespace rts2core;


void MultiDev::initMultidev (int debug)
{
	MultiDev::iterator iter;
	for (iter = begin (); iter != end (); iter++)
	{
		(*iter)->setMulti ();
		(*iter)->setDebug (debug);
		(*iter)->initDaemon ();
		(*iter)->beforeRun ();
	}
}

int MultiDev::run (int debug)
{
	initMultidev (debug);
	multiLoop ();
	return -1;
}

void MultiDev::multiLoop ()
{
	while (getMasterApp ()->getEndLoop () == false)
	{
		runLoop (10);
	}
}

void MultiDev::runLoop (float tmout)
{
	MultiDev::iterator iter;

	struct timespec read_tout;
	read_tout.tv_sec = int (tmout);
	read_tout.tv_nsec = (tmout - read_tout.tv_sec) * NSEC_SEC;

	nfds_t polls = 0;

	for (iter = begin (); iter != end (); iter++)
	{
		(*iter)->addPollSocks ();
		polls += (*iter)->npolls;
	}

	struct pollfd allpolls[polls + 1];
	int pollsa[size ()];
	int i = 0, j = 0;
	for (iter = begin (); iter != end (); iter++, j++)
	{
		memcpy (allpolls + i, (*iter)->fds, sizeof (struct pollfd) * (*iter)->npolls);
		pollsa[j] = (*iter)->npolls;
		i += (*iter)->npolls;
	}

	if (ppoll (allpolls, polls, &read_tout, NULL) > 0)
	{
		j = 0;
		int polloff = 0;
		for (iter = begin (); iter != end (); iter++, j++)
		{
			struct pollfd *oldfd = (*iter)->fds;
			(*iter)->fds = allpolls + polloff;
			(*iter)->pollSuccess ();
			(*iter)->fds = oldfd;
			polloff += pollsa[j];
		}
	}

	for (iter = begin (); iter != end (); iter++)
	{
		(*iter)->callIdle ();
	}
}

MultiBase::MultiBase (int argc, char **argv, const char *default_name):rts2core::Daemon (argc, argv)
{
	multi_name = default_name;

	addOption (OPT_NOAUTH, "noauth", 0, "allow unauthorized connections");
	addOption (OPT_NOTCHECKNULL, "notcheck", 0, "ignore if some recomended values are not set");
	addOption (OPT_LOCALHOST, "localhost", 1, "hostname, if it different from return of gethostname()");
	addOption (OPT_SERVER, "server", 1, "hostname (and possibly port number, separated by :) of central server");
	addOption ('d', NULL, 1, "multidev name (lock file suffix)");
}

void MultiBase::addDevice (Device *dev)
{
	md.push_back (dev);
}

int MultiBase::init ()
{
	int ret;

	ret = Daemon::init ();
	if (ret)
		return ret;

	ret = doDaemonize ();
	if (ret)
		exit (ret);

	std::string s = std::string (getLockPrefix ()) + multi_name;
	ret = checkLockFile (s.c_str ());
	if (ret)
		return ret;
	return lockFile ();
}

int MultiBase::run ()
{
	// init..
	int ret = init ();
	if (ret)
		return ret;
	return md.run ();
}

int MultiBase::processOption (int opt)
{
	switch (opt)
	{
		// those are handled in md inits
		case OPT_NOAUTH:
		case OPT_NOTCHECKNULL:
		case OPT_LOCALHOST:
		case OPT_SERVER:
			break;
		case 'd':
			multi_name = optarg;
			break;
		default:
			return Daemon::processOption (opt);
	}
	return 0;
}

/* 
 * Daemon class.
 * Copyright (C) 2005-2010 Petr Kubanek <petr@kubanek.net>
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

#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "daemon.h"

#ifndef LOCK_SH
#define   LOCK_SH   1    /* shared lock */
#endif

#ifndef LOCK_EX
#define   LOCK_EX   2    /* exclusive lock */
#endif

#ifndef LOCK_NB
#define   LOCK_NB   4    /* don't block when locking */
#endif

#ifndef LOCK_UN
#define   LOCK_UN   8    /* unlock */
#endif

using namespace rts2core;

void Daemon::addConnectionSock (int in_sock)
{
	Rts2Conn *conn = createConnection (in_sock);
	if (sendMetaInfo (conn))
	{
		delete conn;
		return;
	}
	addConnection (conn);
}

Daemon::Daemon (int _argc, char **_argv, int _init_state):rts2core::Block (_argc, _argv)
{
	lockPrefix = NULL;
	lock_fname = NULL;
	lock_file = 0;
	runAs = NULL;

	daemonize = DO_DAEMONIZE;

	doHupIdleLoop = false;

	state = _init_state;

	info_time = new ValueTime (RTS2_VALUE_INFOTIME, "time of last update", false);

	idleInfoInterval = -1;

	addOption ('i', NULL, 0, "run in interactive mode, don't loose console");
	addOption (OPT_LOCALPORT, "local-port", 1, "define local port on which we will listen to incoming requests");
	addOption (OPT_LOCKPREFIX, "lock-prefix", 1, "prefix for lock file");
	addOption (OPT_RUNAS, "run-as", 1, "run under specified user (and group, if it's provided after .)");
}

Daemon::~Daemon (void)
{
	if (listen_sock >= 0)
		close (listen_sock);
	if (lock_file)
		close (lock_file);
	delete info_time;
	closelog ();
}

int Daemon::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'i':
			daemonize = DONT_DAEMONIZE;
			break;
		case OPT_LOCALPORT:
			setPort (atoi (optarg));
			break;
		case OPT_LOCKPREFIX:
			setLockPrefix (optarg);
			break;
		case OPT_RUNAS:
			runAs = optarg;
			break;
		default:
			return rts2core::Block::processOption (in_opt);
	}
	return 0;
}

int Daemon::checkLockFile (const char *_lock_fname)
{
	int ret;
	lock_fname = _lock_fname;
	mode_t old_mask = umask (022);
	lock_file = open (lock_fname, O_RDWR | O_CREAT, 0666);
	umask (old_mask);
	if (lock_file == -1)
	{
		logStream (MESSAGE_ERROR) << "cannot create lock file " << lock_fname << ": "
			<< strerror (errno) << " - do you have correct permission? Try to run daemon as root (sudo,..)"
			<< sendLog;
		return -1;
	}
#ifdef HAVE_FLOCK
	ret = flock (lock_file, LOCK_EX | LOCK_NB);
#else
	ret = lockf (lock_file, F_TLOCK, 0);
#endif
	if (ret)
	{
		if (errno == EWOULDBLOCK)
		{
			logStream (MESSAGE_ERROR) << "lock file " << lock_fname << " owned by another process" << sendLog;
			return -1;
		}
		logStream (MESSAGE_DEBUG) << "cannot flock " << lock_fname << ": " << strerror (errno) << sendLog;
		return -1;
	}
	return 0;
}

int Daemon::checkNotNulls ()
{
	int failed = 0;
	for (Rts2CondValueVector::iterator iter = values.begin (); iter != values.end (); iter++)
	{
		if ((*iter)->getValue ()->getFlags () & RTS2_VALUE_NOTNULL)
			failed += (*iter)->getValue ()->checkNotNull ();
	}
	return failed;
}

int Daemon::doDaemonize ()
{
	if (daemonize != DO_DAEMONIZE)
		return 0;
	int ret;
#ifndef HAVE_FLOCK
	// close and release lock file, as we will lock it again in child - there isn't way how to pass closed file descriptor to child without flock function
	close (lock_file);
	lock_file = 0;
#endif
	ret = fork ();
	if (ret < 0)
	{
		logStream (MESSAGE_ERROR) << "Daemon::int daemonize fork " << strerror (errno) << sendLog;
		exit (2);
	}
	if (ret)
	{
		lock_file = 0;
		exit (0);
	}
	if (runAs)
		switchUser (runAs);

	close (0);
	close (1);
	close (2);
	int f = open ("/dev/null", O_RDWR);
	dup (f);
	dup (f);
	dup (f);
	daemonize = IS_DAEMONIZED;
	openlog (NULL, LOG_PID, LOG_DAEMON);
	return 0;
}

const char * Daemon::getLockPrefix ()
{
	if (lockPrefix == NULL)
		return LOCK_PREFIX;
	return lockPrefix;
}

int Daemon::lockFile ()
{
	if (!lock_file)
		return -1;
	FILE *fd = fdopen (lock_file, "w");
	if (fd == NULL)
		return -1;
	if (fprintf (fd, "%i\n", getpid ()) <= 0)
	{
	  	logStream (MESSAGE_ERROR) << "Cannot write PID to lock file!" << sendLog;
		return -1;
	}
	fflush (fd);
	return 0;
}

int Daemon::init ()
{
	int ret;
	try
	{
		ret = rts2core::Block::init ();
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Daemon::init Rts2block returns " << ret << sendLog;
			return ret;
		}
	}
	catch (Error &er)
	{
		std::cerr << er << std::endl;
		return -1;
	}

	listen_sock = socket (PF_INET, SOCK_STREAM, 0);
	if (listen_sock == -1)
	{
		logStream (MESSAGE_ERROR) << "Daemon::init create listen socket " <<
			strerror (errno) << sendLog;
		return -1;
	}
	const int so_reuseaddr = 1;
	setsockopt (listen_sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
		sizeof (so_reuseaddr));
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons (getPort ());
	server.sin_addr.s_addr = htonl (INADDR_ANY);
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "Daemon::init binding to port: " <<
		getPort () << sendLog;
	#endif						 /* DEBUG_EXTRA */
	ret = bind (listen_sock, (struct sockaddr *) &server, sizeof (server));
	if (ret == -1)
	{
		logStream (MESSAGE_ERROR) << "Daemon::init bind " <<
			strerror (errno) << sendLog;
		return -1;
	}
	socklen_t sock_size = sizeof (server);
	ret = getsockname (listen_sock, (struct sockaddr *) &server, &sock_size);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Daemon::init getsockname " <<
			strerror (errno) << sendLog;
		return -1;
	}
	setPort (ntohs (server.sin_port));
	ret = listen (listen_sock, 5);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "rts2core::Block::init cannot listen: " <<
			strerror (errno) << sendLog;
		close (listen_sock);
		listen_sock = -1;
		return -1;
	}
	return 0;
}

int Daemon::initValues ()
{
	return 0;
}

void Daemon::initDaemon ()
{
	int ret;
	ret = init ();
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "cannot init daemon, exiting" << sendLog;
		exit (ret);
	}
	ret = initValues ();
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "cannot init values in daemon, exiting" <<
			sendLog;
		exit (ret);
	}
}

void Daemon::setIdleInfoInterval (double interval)
{
	// activate infoall event
	if (interval > 0)
	{
		if (idleInfoInterval > 0)
			deleteTimers (EVENT_TIMER_INFOALL);
		addTimer (interval, new Rts2Event (EVENT_TIMER_INFOALL, this));
	}
	idleInfoInterval = interval;
}

int Daemon::run ()
{
	initDaemon ();
	beforeRun ();
	while (!getEndLoop ())
	{
		oneRunLoop ();
	}
	return 0;
}

int Daemon::idle ()
{
	if (doHupIdleLoop)
	{
		signaledHUP ();
		doHupIdleLoop = false;
	}

	return rts2core::Block::idle ();
}

void Daemon::setInfoTime (struct tm *_date)
{
	static char p_tz[100];
	std::string old_tz;
	if (getenv("TZ"))
		old_tz = std::string (getenv ("TZ"));

	putenv ((char*) "TZ=UTC");

	setInfoTime (mktime (_date));

	strcpy (p_tz, "TZ=");

	if (old_tz.length () > 0)
	{
		strncat (p_tz, old_tz.c_str (), 96);
	}
	putenv (p_tz);
}

void Daemon::postEvent (Rts2Event *event)
{
	switch (event->getType ())
	{
		case EVENT_TIMER_INFOALL:
			if (canCallInfoFromTimer ())
				infoAll ();
			// next timer..
			if (idleInfoInterval > 0)
			{
				addTimer (idleInfoInterval, event);
				return;
			}
			break;
	}
	rts2core::Block::postEvent (event);
}

void Daemon::forkedInstance ()
{
	if (listen_sock >= 0)
		close (listen_sock);
	rts2core::Block::forkedInstance ();
}

void Daemon::sendMessage (messageType_t in_messageType, const char *in_messageString)
{
	int prio;
	switch (daemonize)
	{
		case IS_DAEMONIZED:
		case DO_DAEMONIZE:
		case CENTRALD_OK:
			// if at least one centrald is running..
			if (someCentraldRunning ())
				break;
			// otherwise write it to syslog..
			switch (in_messageType)
			{
				case MESSAGE_CRITICAL:
				case MESSAGE_ERROR:
					prio = LOG_ERR;
					break;
				case MESSAGE_WARNING:
					prio = LOG_WARNING;
					break;
				case MESSAGE_INFO:
					prio = LOG_INFO;
					break;
				case MESSAGE_DEBUG:
					prio = LOG_DEBUG;
					break;
			}
			syslog (prio, "%s", in_messageString);
			if (daemonize == IS_DAEMONIZED)
				break;
		case DONT_DAEMONIZE:
			// print to stdout
			rts2core::Block::sendMessage (in_messageType, in_messageString);
			break;
	}
}

void Daemon::centraldConnRunning (Rts2Conn *conn)
{
	if (daemonize == IS_DAEMONIZED)
	{
		daemonize = CENTRALD_OK;
	}
}

void Daemon::centraldConnBroken (Rts2Conn *conn)
{
	if (daemonize == CENTRALD_OK)
	{
		daemonize = IS_DAEMONIZED;
		logStream (MESSAGE_WARNING) << "connection to centrald lost" << sendLog;
	}
}

void Daemon::addSelectSocks ()
{
	FD_SET (listen_sock, &read_set);
	rts2core::Block::addSelectSocks ();
}

void Daemon::selectSuccess ()
{
	int client;
	// accept connection on master
	if (FD_ISSET (listen_sock, &read_set))
	{
		struct sockaddr_in other_side;
		socklen_t addr_size = sizeof (struct sockaddr_in);
		client =
			accept (listen_sock, (struct sockaddr *) &other_side, &addr_size);
		if (client == -1)
		{
			logStream (MESSAGE_DEBUG) << "client accept: " << strerror (errno)
				<< " " << listen_sock << sendLog;
		}
		else
		{
			addConnectionSock (client);
		}
	}
	rts2core::Block::selectSuccess ();
}

void Daemon::addValue (Value * value, int queCondition)
{
	values.push_back (new Rts2CondValue (value, queCondition));
}

Value * Daemon::getOwnValue (const char *v_name)
{
	Rts2CondValue *c_val = getCondValue (v_name);
	if (c_val == NULL)
		return NULL;
	return c_val->getValue ();
}

Rts2CondValue * Daemon::getCondValue (const char *v_name)
{
	Rts2CondValueVector::iterator iter;
	for (iter = values.begin (); iter != values.end (); iter++)
	{
		Rts2CondValue *val = *iter;
		if (val->getValue ()->isValue (v_name))
			return val;
	}
	return NULL;
}

Rts2CondValue * Daemon::getCondValue (const Value *val)
{
	Rts2CondValueVector::iterator iter;
	for (iter = values.begin (); iter != values.end (); iter++)
	{
		Rts2CondValue *c_val = *iter;
		if (c_val->getValue () == val)
			return c_val;
	}
	return NULL;
}

Value * Daemon::duplicateValue (Value * old_value, bool withVal)
{
	// create new value, which will be passed to hook
	Value *dup_val = NULL;
	switch (old_value->getValueExtType ())
	{
		case 0:
			dup_val = newValue (old_value->getFlags (), old_value->getName (), old_value->getDescription ());
			// do some extra settings
			switch (old_value->getValueType ())
			{
				case RTS2_VALUE_SELECTION:
					((ValueSelection *) dup_val)->copySel ((ValueSelection *) old_value);
					break;
			}
			if (withVal)
				((ValueString *) dup_val)->setFromValue (old_value);
			break;
		case RTS2_VALUE_STAT:
			dup_val = new ValueDoubleStat (old_value->getName (), old_value->getDescription (), old_value->getWriteToFits ());
			break;
		case RTS2_VALUE_MMAX:
			dup_val = new ValueDoubleMinMax (old_value->getName (), old_value->getDescription (), old_value->getWriteToFits ());
			((ValueDoubleMinMax *) dup_val)->copyMinMax ((ValueDoubleMinMax *) old_value);
			break;
		case RTS2_VALUE_RECTANGLE:
			dup_val = new ValueRectangle (old_value->getName (), old_value->getDescription (), old_value->getWriteToFits (), old_value->getFlags ());
			break;
		case RTS2_VALUE_ARRAY:
			switch (old_value->getValueBaseType ())
			{
				case RTS2_VALUE_STRING:
					dup_val = new StringArray (old_value->getName (), old_value->getDescription (), old_value->getWriteToFits (), old_value->getFlags ());
					break;
				case RTS2_VALUE_DOUBLE:
					dup_val = new DoubleArray (old_value->getName (), old_value->getDescription (), old_value->getWriteToFits (), old_value->getFlags ());
					break;
				case RTS2_VALUE_TIME:
					dup_val = new TimeArray (old_value->getName (), old_value->getDescription (), old_value->getWriteToFits (), old_value->getFlags ());
					break;
				case RTS2_VALUE_INTEGER:
					dup_val = new IntegerArray (old_value->getName (), old_value->getDescription (), old_value->getWriteToFits (), old_value->getFlags ());
					break;
				case RTS2_VALUE_BOOL:
					dup_val = new BoolArray (old_value->getName (), old_value->getDescription (), old_value->getWriteToFits (), old_value->getFlags ());
					break;
				default:
					logStream (MESSAGE_ERROR) << "unknow array type: " << old_value->getValueBaseType () << sendLog;
					break;
			}
			if (dup_val)
				break;
		default:
			logStream (MESSAGE_ERROR) << "unknow value type: " << old_value->getValueExtType () << sendLog;
			return NULL;
	}
	if (withVal)
		dup_val->setFromValue (old_value);
	return dup_val;
}

void Daemon::addConstValue (Value * value)
{
	constValues.push_back (value);
}

void Daemon::addConstValue (const char *in_name, const char *in_desc, const char *in_value)
{
	ValueString *val = new ValueString (in_name, std::string (in_desc));
	val->setValueCharArr (in_value);
	addConstValue (val);
}

void Daemon::addConstValue (const char *in_name, const char *in_desc, std::string in_value)
{
	ValueString *val = new ValueString (in_name, std::string (in_desc));
	val->setValueString (in_value);
	addConstValue (val);
}

void Daemon::addConstValue (const char *in_name, const char *in_desc, double in_value)
{
	ValueDouble *val = new ValueDouble (in_name, std::string (in_desc));
	val->setValueDouble (in_value);
	addConstValue (val);
}

void Daemon::addConstValue (const char *in_name, const char *in_desc, int in_value)
{
	ValueInteger *val =
		new ValueInteger (in_name, std::string (in_desc));
	val->setValueInteger (in_value);
	addConstValue (val);
}

void Daemon::addConstValue (const char *in_name, const char *in_value)
{
	ValueString *val = new ValueString (in_name);
	val->setValueCharArr (in_value);
	addConstValue (val);
}

void Daemon::addConstValue (const char *in_name, double in_value)
{
	ValueDouble *val = new ValueDouble (in_name);
	val->setValueDouble (in_value);
	addConstValue (val);
}

void Daemon::addConstValue (const char *in_name, int in_value)
{
	ValueInteger *val = new ValueInteger (in_name);
	val->setValueInteger (in_value);
	addConstValue (val);
}

int Daemon::setValue (Value * old_value, Value * newValue)
{
	// if for some reason writable value makes it there, it means that it was not caught downstream, and it can be set
	if (old_value->isWritable ())
		return 0;
	// we don't know how to set values, so return -2
	return -2;
}

void Daemon::changeValue (Value * value, int nval)
{
	Rts2CondValue *cv = getCondValue (value);
	Value *nv = duplicateValue (value, false);
	nv->setValueInteger (nval);
	doSetValue (cv, '=', nv);
}

void Daemon::changeValue (Value * value, bool nval)
{
	Rts2CondValue *cv = getCondValue (value);
	Value *nv = duplicateValue (value, false);
	((ValueBool *) nv)->setValueBool (nval);
	doSetValue (cv, '=', nv);
}

void Daemon::changeValue (Value * value, double nval)
{
	Rts2CondValue *cv = getCondValue (value);
	Value *nv = duplicateValue (value, false);
	((ValueDoubleStat *) nv)->setValueDouble (nval);
	doSetValue (cv, '=', nv);
}

int Daemon::setCondValue (Rts2CondValue * old_value_cond, char op, Value * new_value)
{
	// que change if that's necessary
	if ((op != '=' || !old_value_cond->getValue ()->isEqual (new_value) || queValues.contains (old_value_cond->getValue ()))
		&& (queValueChange (old_value_cond, getState ()))
		)
	{
		queValues.push_back (new ValueQue (old_value_cond, op, new_value));
		return -1;
	}

	// do not set values already set to new value
	if (op == '=' && old_value_cond->getValue ()->isEqual (new_value))
		return 0;

	return doSetValue (old_value_cond, op, new_value);
}

int Daemon::doSetValue (Rts2CondValue * old_cond_value, char op, Value * new_value)
{
	int ret;

	Value *old_value = old_cond_value->getValue ();

	ret = new_value->doOpValue (op, old_value);
	if (ret)
	{
		// translate error to real error, not only qued error
		ret = -2;
		goto err;
	}
	// call hook
	ret = setValue (old_value, new_value);
	if (ret < 0)
		goto err;

	// set value after sucessfull return..
	if (ret == 0)
	{
		if (!old_value->isEqual (new_value))
		{
			old_value->setFromValue (new_value);
			valueChanged (old_value);
		}
	}

	// if the previous one was postponed, clear that flag..
	if (old_cond_value->loadedFromQue ())
	{
		old_cond_value->clearLoadedFromQue ();
		delete old_cond_value;
	}
	else
	{
		delete new_value;
	}

	sendValueAll (old_value);

	return 0;
err:
	logStream (MESSAGE_ERROR) << "Daemon::loadValues cannot set value " << new_value->getName () << sendLog;

	delete new_value;
	return ret;
}

void Daemon::valueChanged (Value *changed_value)
{
}

int Daemon::baseInfo ()
{
	return 0;
}

int Daemon::baseInfo (Rts2Conn * conn)
{
	int ret;
	ret = baseInfo ();
	if (ret)
	{
		conn->sendCommandEnd (DEVDEM_E_HW, "device not ready");
		return -1;
	}
	return sendBaseInfo (conn);
}

int Daemon::sendBaseInfo (Rts2Conn * conn)
{
	for (ValueVector::iterator iter = constValues.begin ();
		iter != constValues.end (); iter++)
	{
		Value *val = *iter;
		val->send (conn);
	}
	return 0;
}

int Daemon::info ()
{
	updateInfoTime ();
	return 0;
}

int Daemon::info (Rts2Conn * conn)
{
	int ret;
	ret = info ();
	if (ret)
	{
		conn->sendCommandEnd (DEVDEM_E_HW, "device not ready");
		return -1;
	}
	ret = sendInfo (conn);
	if (ret)
		conn->sendCommandEnd (DEVDEM_E_SYSTEM, "cannot send info");
	return ret;
}

int Daemon::infoAll ()
{
	int ret;
	ret = info ();
	if (ret)
		return -1;
	connections_t::iterator iter;
	for (iter = getConnections ()->begin (); iter != getConnections ()->end (); iter++)
		sendInfo (*iter);
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
		sendInfo (*iter);

	for (Rts2CondValueVector::iterator iter2 = values.begin (); iter2 != values.end (); iter2++)
	{
		Value *val = (*iter2)->getValue ();
		val->resetNeedSend ();
	}

	return 0;
}

void Daemon::constInfoAll ()
{
	connections_t::iterator iter;
	for (iter = getConnections ()->begin (); iter != getConnections ()->end (); iter++)
		sendBaseInfo (*iter);
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
		sendBaseInfo (*iter);
}

int Daemon::sendInfo (Rts2Conn * conn, bool forceSend)
{
	if (!isRunning (conn))
		return -1;
	for (Rts2CondValueVector::iterator iter = values.begin (); iter != values.end (); iter++)
	{
		Value *val = (*iter)->getValue ();
		if (val->needSend () || forceSend)
		{
			val->send (conn);
		}
	}
	if (info_time->needSend ())
		info_time->send (conn);
	return 0;
}

void Daemon::sendValueAll (Value * value)
{
	if (value->needSend ())
	{
		connections_t::iterator iter;
		for (iter = getConnections ()->begin (); iter != getConnections ()->end (); iter++)
			value->send (*iter);
		for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
			value->send (*iter);
		value->resetNeedSend ();
	}
}

void Daemon::sendProgressAll (double start, double end)
{
	connections_t::iterator iter;
	for (iter = getConnections ()->begin (); iter != getConnections ()->end (); iter++)
		(*iter)->sendProgress (start, end);
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
	  	(*iter)->sendProgress (start, end);
}

int Daemon::sendMetaInfo (Rts2Conn * conn)
{
	int ret;
	ret = info_time->sendMetaInfo (conn);
	if (ret < 0)
		return -1;
	for (ValueVector::iterator iter = constValues.begin (); iter != constValues.end (); iter++)
	{
		Value *val = *iter;
		ret = val->sendMetaInfo (conn);
		if (ret < 0)
			return -1;
	}
	for (Rts2CondValueVector::iterator iter = values.begin (); iter != values.end (); iter++)
	{
		Value *val = (*iter)->getValue ();
		ret = val->sendMetaInfo (conn);
		if (ret < 0)
			return -1;
	}
	return 0;
}

int Daemon::setValue (Rts2Conn * conn)
{
	char *v_name;
	char *op;
	int ret;
	if (conn->paramNextString (&v_name) || conn->paramNextString (&op))
		return -2;

	Rts2CondValue *old_value_cond = NULL;

	const char *ai = NULL;

	// search for [ - array index
	char *ca = strchr (v_name, '[');
	if (ca != NULL)
	{
		int l = strlen (v_name);
		*ca = '\0';
		ai = ca + 1;
		if (v_name[l - 1] != ']')
		{
			conn->sendCommandEnd (DEVDEM_E_SYSTEM, "missing ] for end of array index");
			return -1;
		}
		v_name[l - 1] = '\0';
	}
	// try to search for _ - compatibility with old version
	else
	{
		old_value_cond = getCondValue (v_name);
		// if indexed value does not exists..
		if (old_value_cond == NULL)
		{
			ca = strrchr (v_name, '_');
			if (ca != NULL)
			{
				// see if we can get value
				*ca = '\0';
				old_value_cond = getCondValue (v_name);
				// value not found, assume it is not an array index
				if (old_value_cond == NULL)
					*ca = '_';
				else
					ai = ca + 1;
			}
		}
	}

	if (old_value_cond == NULL)
		old_value_cond = getCondValue (v_name);

	if (!old_value_cond)
		return -2;
	Value *old_value = old_value_cond->getValue ();
	if (!old_value)
		return -2;
	if (!old_value->isWritable ())
	{
	  	conn->sendCommandEnd (DEVDEM_E_SYSTEM, "cannot set read-only value");
		return -1;
	}

	if (ai && (!(old_value->getFlags () & RTS2_VALUE_ARRAY)))
	{
		conn->sendCommandEnd (DEVDEM_E_SYSTEM, "trying to index non-array value");
		return -1;
	}

	// array needs to be allocated with values, as it will then only modify some indices in a new array
	Value *newValue = duplicateValue (old_value, (old_value->getFlags () & RTS2_VALUE_ARRAY));

	if (newValue == NULL)
		return -2;

	if (ai)
	{
		const char *endp;
		std::vector <int> indices = parseRange (ai, ((ValueArray *)old_value)->size (), endp);
		if (*endp)
		{
			conn->sendCommandEnd (DEVDEM_E_PARAMSVAL, endp);
			return -1;
		}
		ret = ((ValueArray *)newValue)->setValues (indices, conn);
	}
	else
	{
		ret = newValue->setValue (conn);
	}
	if (ret)
		goto err;

	ret = setCondValue (old_value_cond, *op, newValue);
	// value change was qued
	if (ret == -1)
	{
		std::ostringstream os;
		os << "value " << old_value_cond->getValue()->getName () << " change was queued";
		conn->sendCommandEnd (DEVDEM_I_QUED, os.str ().c_str ());
	}
	return ret;

err:
	delete newValue;
	return ret;
}

void Daemon::setState (int new_state, const char *description, Rts2Conn *commandedConn)
{
	if (state == new_state)
		return;
	stateChanged (new_state, state, description, commandedConn);
}

void Daemon::stateChanged (int new_state, int old_state, const char *description, Rts2Conn *commandedConn)
{
	state = new_state;
}

void Daemon::maskState (int state_mask, int new_state, const char *description, double start, double end, Rts2Conn *commandedConn)
{
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG)
		<< "Device::maskState state: state_mask: " << std::hex << state_mask
		<< " new_state: " << std::hex << new_state
		<< " desc: " << description
		<< " start: " << start
		<< " end: " << end
		<< sendLog;
	#endif
	int masked_state = state;
	// null from state all errors..
	masked_state &= ~(DEVICE_ERROR_MASK | state_mask);
	masked_state |= new_state;
	setState (masked_state, description, commandedConn);

	if (!(isnan (start) && isnan(end)))
		sendProgressAll (start, end);
}

void Daemon::signaledHUP ()
{
	// empty here, shall be supplied in descendants..
}

void Daemon::sigHUP (int sig)
{
	doHupIdleLoop = true;
}

void Daemon::switchUser (const char *usrgrp)
{
	char *user = (char *) usrgrp;
	char *grp = strchr (user, '.');
	if (grp)
	{
		*grp = '\0';
		grp++;
	}

	errno = 0;

	struct passwd *pw = getpwnam (user);
	if (pw == NULL)
	{
		std::cerr << "cannot find user with name " << usrgrp;
		if (errno)
			std::cerr << strerror (errno) << " (" << errno << ")";
		std::cerr << ", exiting" << std::endl;
		exit (3);
	}

	int grpid = pw->pw_gid;

	if (grp)
	{
		struct group *gr = getgrnam (grp);
		if (gr == NULL)
		{
			std::cerr << "cannot find group with name " << grp;
			if (errno)
				std::cerr << strerror (errno) << " (" << errno << ")";
			std::cerr << ", exiting" << std::endl;
			exit (5);
		}
		grpid = gr->gr_gid;
	}

	if (setgid (grpid))
	{
		std::cerr << "cannot switch group to uid " << grpid << " " << strerror (errno) << std::endl;
		exit (4);
	}

	if (setuid (pw->pw_uid))
	{
		std::cerr << "cannot switch user to uid " << pw->pw_uid << " " << strerror (errno) << std::endl;
		exit (4);
	}
}

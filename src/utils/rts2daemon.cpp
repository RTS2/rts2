/* 
 * Daemon class.
 * Copyright (C) 2005-2009 Petr Kubanek <petr@kubanek.net>
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
#include <syslog.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/file.h>

#include "rts2daemon.h"

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

void
Rts2Daemon::addConnectionSock (int in_sock)
{
	Rts2Conn *conn = createConnection (in_sock);
	if (sendMetaInfo (conn))
	{
		delete conn;
		return;
	}
	addConnection (conn);
}


Rts2Daemon::Rts2Daemon (int _argc, char **_argv, int _init_state):
Rts2Block (_argc, _argv)
{
	lockPrefix = NULL;
	lock_fname = NULL;
	lock_file = 0;

	daemonize = DO_DAEMONIZE;

	doHupIdleLoop = false;

	state = _init_state;

	info_time = new Rts2ValueTime (RTS2_VALUE_INFOTIME, "time when this informations were correct", false);

	idleInfoInterval = -1;
	nextIdleInfo = 0;

	addOption ('i', NULL, 0, "run in interactive mode, don't loose console");
	addOption (OPT_LOCALPORT, "local-port", 1,
		"define local port on which we will listen to incoming requests");
	addOption (OPT_LOCKPREFIX, "lock-prefix", 1,
		"prefix for lock file");
}


Rts2Daemon::~Rts2Daemon (void)
{
	savedValues.clear ();
	if (listen_sock >= 0)
		close (listen_sock);
	if (lock_file)
		close (lock_file);
	delete info_time;
	closelog ();
}


int
Rts2Daemon::processOption (int in_opt)
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
		default:
			return Rts2Block::processOption (in_opt);
	}
	return 0;
}


int
Rts2Daemon::checkLockFile (const char *_lock_fname)
{
	int ret;
	lock_fname = _lock_fname;
	mode_t old_mask = umask (022);
	lock_file = open (lock_fname, O_RDWR | O_CREAT, 0666);
	umask (old_mask);
	if (lock_file == -1)
	{
		logStream (MESSAGE_ERROR) << "cannot create lock file " << lock_fname
			<< strerror (errno) << " - do you have correct permission? Try to run daemon as root (sudo,..)"
			<< sendLog;
		return -1;
	}
#ifdef HAVE_FLOCK
	ret = flock (lock_file, LOCK_EX | LOCK_NB);
#else
	struct flock fl;

	fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
	fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
	fl.l_start  = 0;        /* Offset from l_whence         */
	fl.l_len    = 0;        /* length, 0 = to EOF           */
	fl.l_pid    = getpid(); /* our PID                      */

	ret = fcntl(lock_file, F_SETLKW, &fl);  /* F_GETLK, F_SETLK, F_SETLKW */
#endif
	if (ret)
	{
		if (errno == EWOULDBLOCK)
		{
			logStream (MESSAGE_ERROR) << "lock file " << lock_fname <<
				" owned by another process" << sendLog;
			return -1;
		}
		logStream (MESSAGE_DEBUG) << "cannot flock " << lock_fname << ": " <<
			strerror (errno) << sendLog;
		return -1;
	}
	return 0;
}


int
Rts2Daemon::doDeamonize ()
{
	if (daemonize != DO_DAEMONIZE)
		return 0;
	int ret;
	ret = fork ();
	if (ret < 0)
	{
		logStream (MESSAGE_ERROR)
			<< "Rts2Daemon::int daemonize fork " << strerror (errno) << sendLog;
		exit (2);
	}
	if (ret)
		exit (0);
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


const char *
Rts2Daemon::getLockPrefix ()
{
	if (lockPrefix == NULL)
		return LOCK_PREFIX;
	return lockPrefix;
}


int
Rts2Daemon::lockFile ()
{
	if (!lock_file)
		return -1;
	std::ofstream _os (lock_fname);
	_os << getpid () << std::endl;
	_os.flush ();
	_os.close ();
	return 0;
}


int
Rts2Daemon::init ()
{
	int ret;
	ret = Rts2Block::init ();
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Rts2Daemon::init Rts2block returns " <<
			ret << sendLog;
		return ret;
	}

	listen_sock = socket (PF_INET, SOCK_STREAM, 0);
	if (listen_sock == -1)
	{
		logStream (MESSAGE_ERROR) << "Rts2Daemon::init create listen socket " <<
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
	logStream (MESSAGE_DEBUG) << "Rts2Daemon::init binding to port: " <<
		getPort () << sendLog;
	#endif						 /* DEBUG_EXTRA */
	ret = bind (listen_sock, (struct sockaddr *) &server, sizeof (server));
	if (ret == -1)
	{
		logStream (MESSAGE_ERROR) << "Rts2Daemon::init bind " <<
			strerror (errno) << sendLog;
		return -1;
	}
	socklen_t sock_size = sizeof (server);
	ret = getsockname (listen_sock, (struct sockaddr *) &server, &sock_size);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Rts2Daemon::init getsockname " <<
			strerror (errno) << sendLog;
		return -1;
	}
	setPort (ntohs (server.sin_port));
	ret = listen (listen_sock, 5);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Rts2Block::init cannot listen: " <<
			strerror (errno) << sendLog;
		close (listen_sock);
		listen_sock = -1;
		return -1;
	}
	return 0;
}


int
Rts2Daemon::initValues ()
{
	return 0;
}


void
Rts2Daemon::initDaemon ()
{
	int ret;
	ret = init ();
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Cannot init daemon, exiting" << sendLog;
		exit (ret);
	}
	ret = initValues ();
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Cannot init values in daemon, exiting" <<
			sendLog;
		exit (ret);
	}
}


int
Rts2Daemon::run ()
{
	initDaemon ();
	while (!getEndLoop ())
	{
		oneRunLoop ();
	}
	return 0;
}


int
Rts2Daemon::idle ()
{
	time_t now = time (NULL);
	if (idleInfoInterval >= 0)
	{
		if (now >= nextIdleInfo)
		{
			infoAll ();
		}
		else
		{
			setTimeoutMin ((nextIdleInfo - now) * USEC_SEC);
		}
	}
	if (doHupIdleLoop)
	{
		signaledHUP ();
		doHupIdleLoop = false;
	}

	return Rts2Block::idle ();
}


void
Rts2Daemon::forkedInstance ()
{
	if (listen_sock >= 0)
		close (listen_sock);
	Rts2Block::forkedInstance ();
}


void
Rts2Daemon::sendMessage (messageType_t in_messageType, const char *in_messageString)
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
			Rts2Block::sendMessage (in_messageType, in_messageString);
			break;
	}
}


void
Rts2Daemon::centraldConnRunning (Rts2Conn *conn)
{
	if (daemonize == IS_DAEMONIZED)
	{
		daemonize = CENTRALD_OK;
	}
}


void
Rts2Daemon::centraldConnBroken (Rts2Conn *conn)
{
	if (daemonize == CENTRALD_OK)
	{
		daemonize = IS_DAEMONIZED;
		logStream (MESSAGE_WARNING) << "connection to centrald lost" << sendLog;
	}
}


void
Rts2Daemon::addSelectSocks ()
{
	FD_SET (listen_sock, &read_set);
	Rts2Block::addSelectSocks ();
}


void
Rts2Daemon::selectSuccess ()
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
	Rts2Block::selectSuccess ();
}


void
Rts2Daemon::saveValue (Rts2CondValue * val)
{
	Rts2Value *old_value = duplicateValue (val->getValue (), true);
	savedValues.push_back (old_value);
	val->setValueSave ();
}


void
Rts2Daemon::deleteSaveValue (Rts2CondValue * val)
{
	for (Rts2ValueVector::iterator iter = savedValues.begin (); iter != savedValues.end (); iter++)
	{
		Rts2Value *new_val = *iter;
		if (new_val->isValue (val->getValue ()->getName ().c_str ()))
		{
			savedValues.erase (iter);
			return;
		}
	}
}


void
Rts2Daemon::loadValues ()
{
	int ret;
	for (Rts2ValueVector::iterator iter = savedValues.begin ();
		iter != savedValues.end ();)
	{
		Rts2Value *new_val = *iter;
		Rts2CondValue *old_val = getCondValue (new_val->getName ().c_str ());
		if (old_val == NULL)
		{
			logStream (MESSAGE_ERROR) <<
				"Rts2Daemon::loadValues cannot get value " << new_val->
				getName () << sendLog;
		}
		// we don't need to save this one
		else if (old_val->ignoreLoad ())
		{
			// just reset it..
			old_val->clearIgnoreSave ();
		}
		// if there was error setting value
		else
		{
			ret = setCondValue (old_val, '=', new_val);

			if (ret == 0 || ret == -2)
			{
				old_val->clearValueSave ();
				// this will put to iter next value..
				iter = savedValues.erase (iter);
				continue;
			}
			else if (ret == -1)
			{
				// if change was qued
				iter = savedValues.erase (iter);
				continue;
			}
			else
			{
			  	old_val->setValueSaveAfterLoad ();
				old_val->loadFromQue ();
			}
		}
		iter++;
	}
}


void
Rts2Daemon::addValue (Rts2Value * value, int queCondition, bool save_value)
{
	values.push_back (new Rts2CondValue (value, queCondition, save_value));
}


Rts2Value *
Rts2Daemon::getValue (const char *v_name)
{
	Rts2CondValue *c_val = getCondValue (v_name);
	if (c_val == NULL)
		return NULL;
	return c_val->getValue ();
}


Rts2CondValue *
Rts2Daemon::getCondValue (const char *v_name)
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


Rts2CondValue *
Rts2Daemon::getCondValue (const Rts2Value *val)
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


Rts2Value *
Rts2Daemon::duplicateValue (Rts2Value * old_value, bool withVal)
{
	// create new value, which will be passed to hook
	Rts2Value *dup_val;
	switch (old_value->getValueExtType ())
	{
		case 0:
			dup_val = newValue (old_value->getFlags (),
				old_value->getName (),
				old_value->getDescription ());
			// do some extra settings
			switch (old_value->getValueType ())
			{
				case RTS2_VALUE_SELECTION:
					((Rts2ValueSelection *) dup_val)->copySel ((Rts2ValueSelection *) old_value);
					break;
			}
			if (withVal)
				((Rts2ValueString *) dup_val)->setValueCharArr (old_value->
					getValue ());
			break;
		case RTS2_VALUE_STAT:
			dup_val = new Rts2ValueDoubleStat (old_value->getName (),
				old_value->getDescription (),
				old_value->getWriteToFits ());
			break;
		case RTS2_VALUE_MMAX:
			dup_val = new Rts2ValueDoubleMinMax (old_value->getName (),
				old_value->getDescription (),
				old_value->getWriteToFits ());
			((Rts2ValueDoubleMinMax *) dup_val)->
				copyMinMax ((Rts2ValueDoubleMinMax *) old_value);
			break;
		case RTS2_VALUE_RECTANGLE:
			dup_val = new Rts2ValueRectangle (old_value->getName (),
				old_value->getDescription (),
				old_value->getWriteToFits (),
				old_value->getFlags ());
			break;
		case RTS2_VALUE_ARRAY:
			switch (old_value->getValueBaseType ())
			{
				case RTS2_VALUE_STRING:
					dup_val = new StringArray (old_value->getName (),
						old_value->getDescription (),
						old_value->getWriteToFits (),
						old_value->getFlags ());
					break;
				case RTS2_VALUE_DOUBLE:
					dup_val = new DoubleArray (old_value->getName (),
						old_value->getDescription (),
						old_value->getWriteToFits (),
						old_value->getFlags ());
					break;
				default:
					logStream (MESSAGE_ERROR) << "unknow array type: " << old_value->getValueBaseType () << sendLog;
					break;
			}

		default:
			logStream (MESSAGE_ERROR) << "unknow value type: " << old_value->
				getValueType () << sendLog;
			return NULL;
	}
	if (withVal)
		dup_val->setFromValue (old_value);
	return dup_val;
}


void
Rts2Daemon::addConstValue (Rts2Value * value)
{
	constValues.push_back (value);
}


void
Rts2Daemon::addBopValue (Rts2Value * value)
{
	bopValues.push_back (value);
	// create status mask and send it..
	checkBopStatus ();
}


void
Rts2Daemon::removeBopValue (Rts2Value * value)
{
	for (Rts2ValueVector::iterator iter = bopValues.begin ();
		iter != bopValues.end ();)
	{
		if (*iter == value)
			iter = bopValues.erase (iter);
		else
			iter++;
	}
	checkBopStatus ();
}


void
Rts2Daemon::checkBopStatus ()
{
	int new_state = getState ();
	for (Rts2ValueVector::iterator iter = bopValues.begin ();
		iter != bopValues.end (); iter++)
	{
		Rts2Value *val = *iter;
		new_state |= val->getBopMask ();
	}
	setState (new_state, "changed due to bop");
}


void
Rts2Daemon::addConstValue (const char *in_name, const char *in_desc, const char *in_value)
{
	Rts2ValueString *val = new Rts2ValueString (in_name, std::string (in_desc));
	val->setValueCharArr (in_value);
	addConstValue (val);
}


void
Rts2Daemon::addConstValue (const char *in_name, const char *in_desc, std::string in_value)
{
	Rts2ValueString *val = new Rts2ValueString (in_name, std::string (in_desc));
	val->setValueString (in_value);
	addConstValue (val);
}


void
Rts2Daemon::addConstValue (const char *in_name, const char *in_desc, double in_value)
{
	Rts2ValueDouble *val = new Rts2ValueDouble (in_name, std::string (in_desc));
	val->setValueDouble (in_value);
	addConstValue (val);
}


void
Rts2Daemon::addConstValue (const char *in_name, const char *in_desc, int in_value)
{
	Rts2ValueInteger *val =
		new Rts2ValueInteger (in_name, std::string (in_desc));
	val->setValueInteger (in_value);
	addConstValue (val);
}


void
Rts2Daemon::addConstValue (const char *in_name, const char *in_value)
{
	Rts2ValueString *val = new Rts2ValueString (in_name);
	val->setValueCharArr (in_value);
	addConstValue (val);
}


void
Rts2Daemon::addConstValue (const char *in_name, double in_value)
{
	Rts2ValueDouble *val = new Rts2ValueDouble (in_name);
	val->setValueDouble (in_value);
	addConstValue (val);
}


void
Rts2Daemon::addConstValue (const char *in_name, int in_value)
{
	Rts2ValueInteger *val = new Rts2ValueInteger (in_name);
	val->setValueInteger (in_value);
	addConstValue (val);
}


int
Rts2Daemon::setValue (Rts2Value * old_value, Rts2Value * newValue)
{
	// we don't know how to set values, so return -2
	return -2;
}


int
Rts2Daemon::setCondValue (Rts2CondValue * old_value_cond, char op, Rts2Value * new_value)
{
	// que change if that's necessary
	if ((op != '=' || !old_value_cond->getValue ()->isEqual (new_value) || queValues.contains (old_value_cond->getValue ()))
		&& (queValueChange (old_value_cond, getState ()))
		)
	{
		queValues.push_back (new Rts2ValueQue (old_value_cond, op, new_value));
		return -1;
	}

	return doSetValue (old_value_cond, op, new_value);
}


int
Rts2Daemon::doSetValue (Rts2CondValue * old_cond_value, char op, Rts2Value * new_value)
{
	int ret;

	Rts2Value *old_value = old_cond_value->getValue ();

	// save values before first change
	if (old_cond_value->needSaveValue ())
	{
		saveValue (old_cond_value);
	}

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

	// if in previous step we put ignore load, reset it now
	if (old_cond_value->ignoreLoad ())
	{
		saveValue (old_cond_value);
		old_cond_value->clearIgnoreSave ();
	}

	// if the previous one was postponed, clear that flag..
	if (old_cond_value->loadedFromQue ())
	{
		old_cond_value->clearLoadedFromQue ();
		deleteSaveValue (old_cond_value);
	}
	else
	{
		delete new_value;
	}

	sendValueAll (old_value);

	return 0;
	err:
	logStream (MESSAGE_ERROR)
		<< "Rts2Daemon::loadValues cannot set value "
		<< new_value->getName ()
		<< sendLog;

	delete new_value;
	return ret;
}


void
Rts2Daemon::valueChanged (Rts2Value *changed_value)
{
}


int
Rts2Daemon::baseInfo ()
{
	return 0;
}


int
Rts2Daemon::baseInfo (Rts2Conn * conn)
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


int
Rts2Daemon::sendBaseInfo (Rts2Conn * conn)
{
	for (Rts2ValueVector::iterator iter = constValues.begin ();
		iter != constValues.end (); iter++)
	{
		Rts2Value *val = *iter;
		int ret;
		ret = val->send (conn);
		if (ret)
			return ret;
	}
	return 0;
}


int
Rts2Daemon::info ()
{
	updateInfoTime ();
	return 0;
}


int
Rts2Daemon::info (Rts2Conn * conn)
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


int
Rts2Daemon::infoAll ()
{
	int ret;
	nextIdleInfo = time (NULL) + idleInfoInterval;
	ret = info ();
	if (ret)
		return -1;
	connections_t::iterator iter;
	for (iter = getConnections ()->begin (); iter != getConnections ()->end (); iter++)
		sendInfo (*iter);
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
		sendInfo (*iter);
	return 0;
}


void
Rts2Daemon::constInfoAll ()
{
	connections_t::iterator iter;
	for (iter = getConnections ()->begin (); iter != getConnections ()->end (); iter++)
		sendBaseInfo (*iter);
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
		sendBaseInfo (*iter);
}


int
Rts2Daemon::sendInfo (Rts2Conn * conn)
{
	if (!isRunning (conn))
		return -1;
	for (Rts2CondValueVector::iterator iter = values.begin ();
		iter != values.end (); iter++)
	{
		Rts2Value *val = (*iter)->getValue ();
		int ret;
		ret = val->send (conn);
		if (ret)
			return ret;
	}
	return info_time->send (conn);
}


void
Rts2Daemon::sendValueAll (Rts2Value * value)
{
	connections_t::iterator iter;
	for (iter = getConnections ()->begin (); iter != getConnections ()->end (); iter++)
		value->send (*iter);
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
		value->send (*iter);
}


void
Rts2Daemon::checkValueSave (Rts2Value *val)
{
	Rts2CondValue *cond_val = getCondValue (val);
	if (cond_val && cond_val->needSaveValue ())
		saveValue (cond_val);
}


int
Rts2Daemon::sendMetaInfo (Rts2Value * val)
{
	connections_t::iterator iter;
	for (iter = getConnections ()->begin (); iter != getConnections ()->end (); iter++)
		val->sendMetaInfo (*iter);
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
		val->sendMetaInfo (*iter);
	return 0;
}


int
Rts2Daemon::sendMetaInfo (Rts2Conn * conn)
{
	int ret;
	ret = info_time->sendMetaInfo (conn);
	if (ret < 0)
		return -1;
	for (Rts2ValueVector::iterator iter = constValues.begin (); iter != constValues.end (); iter++)
	{
		Rts2Value *val = *iter;
		ret = val->sendMetaInfo (conn);
		if (ret < 0)
			return -1;
	}
	for (Rts2CondValueVector::iterator iter = values.begin (); iter != values.end (); iter++)
	{
		Rts2Value *val = (*iter)->getValue ();
		ret = val->sendMetaInfo (conn);
		if (ret < 0)
			return -1;
	}
	return 0;
}


int
Rts2Daemon::setValue (Rts2Conn * conn, bool overwriteSaved)
{
	char *v_name;
	char *op;
	int ret;
	if (conn->paramNextString (&v_name) || conn->paramNextString (&op))
		return -2;
	Rts2CondValue *old_value_cond = getCondValue (v_name);
	if (!old_value_cond)
		return -2;
	Rts2Value *old_value = old_value_cond->getValue ();
	if (!old_value)
		return -2;
	Rts2Value *newValue;

	newValue = duplicateValue (old_value);

	if (newValue == NULL)
		return -2;

	ret = newValue->setValue (conn);
	if (ret)
		goto err;

	if (overwriteSaved)
	{
		old_value_cond->setIgnoreSave ();
		deleteSaveValue (old_value_cond);
	}

	ret = setCondValue (old_value_cond, *op, newValue);
	// value change was qued
	if (ret == -1)
	{
		conn->sendCommandEnd (DEVDEM_I_QUED, "value change was qued");
	}
	return ret;

	err:
	delete newValue;
	return ret;
}


void
Rts2Daemon::setState (int new_state, const char *description)
{
	if (state == new_state)
		return;
	stateChanged (new_state, state, description);
}


void
Rts2Daemon::stateChanged (int new_state, int old_state, const char *description)
{
	state = new_state;
}


void
Rts2Daemon::maskState (int state_mask, int new_state, const char *description)
{
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG)
		<< "Rts2Device::maskState state: state_mask: " << std::hex << state_mask
		<< " new_state: " << std::hex << new_state
		<< " desc: " << description
		<< sendLog;
	#endif
	int masked_state = state;
	// null from state all errors..
	masked_state &= ~(DEVICE_ERROR_MASK | state_mask);
	masked_state |= new_state;
	setState (masked_state, description);
}


void
Rts2Daemon::signaledHUP ()
{
	// empty here, shall be supplied in descendants..
}


void
Rts2Daemon::sigHUP (int sig)
{
	doHupIdleLoop = true;
}

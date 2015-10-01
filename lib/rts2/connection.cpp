/* 
 * Connection class.
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

#include "radecparser.h"

#include "connection.h"
#include "centralstate.h"
#include "block.h"
#include "command.h"

#include "valuestat.h"
#include "valueminmax.h"
#include "valuerectangle.h"
#include "valuearray.h"

#include <iostream>

#include <errno.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace rts2core;

ConnError::ConnError (Connection *conn, const char *_msg): Error (_msg)
{
	conn->connectionError (-1);
}

ConnError::ConnError (Connection *conn, const char *_msg, int _errn): Error ()
{
	setMsg (std::string ("connection error ") + _msg + ": " + strerror (_errn));
	conn->connectionError (-1);
}

Connection::Connection (Block * in_master):Object ()
{
	buf = new char[MAX_DATA + 1];
	buf_size = MAX_DATA;

	sock = -1;
	master = in_master;
	buf_top = buf;
	full_data_end = NULL;

	key = 0;
	centrald_num = -1;
	centrald_id = -1;
	conn_state = CONN_UNKNOW;
	type = NOT_DEFINED_SERVER;
	runningCommand = NULL;
	runningCommandStatus = RETURNING;
	serverState = new ServerState ();
	bopState = new ServerState ();
	otherDevice = NULL;
	otherType = -1;

	statusStart = NAN;
	statusExpectedEnd = NAN;

	connectionTimeout = 300;	 // 5 minutes timeout

	commandInProgress = false;
	info_time = NULL;
	last_info_time = 0;

	time (&lastGoodSend);
	lastData = lastGoodSend;
	lastSendReady = lastGoodSend - connectionTimeout;

	activeReadData = -1;
	dataConn = 0;

	sharedReadMemory = NULL;
}

Connection::Connection (int in_sock, Block * in_master):Object ()
{
	buf = new char[MAX_DATA + 1];
	buf_size = MAX_DATA;

	sock = in_sock;
	master = in_master;
	buf_top = buf;
	full_data_end = NULL;

	key = 0;
	centrald_num = -1;
	centrald_id = -1;
	conn_state = CONN_CONNECTED;
	type = NOT_DEFINED_SERVER;
	runningCommand = NULL;
	runningCommandStatus = RETURNING;
	serverState = new ServerState ();
	bopState = new ServerState ();
	otherDevice = NULL;
	otherType = -1;

	connectionTimeout = 300;	 // 5 minutes timeout (150 + 150)

	statusStart = NAN;
	statusExpectedEnd = NAN;

	commandInProgress = false;
	info_time = NULL;

	time (&lastGoodSend);
	lastData = lastGoodSend;
	lastSendReady = lastData - connectionTimeout;

	activeReadData = -1;
	dataConn = 0;

	sharedReadMemory = NULL;
}

Connection::~Connection (void)
{
	if (sock >= 0)
		close (sock);
	delete serverState;
	delete bopState;
	queClear ();
	delete[]buf;
	delete sharedReadMemory;
	delete otherDevice;
}

int Connection::add (fd_set * readset, fd_set * writeset, fd_set * expset)
{
	if (sock >= 0)
	{
		FD_SET (sock, readset);
		if (isConnState (CONN_INPROGRESS))
			FD_SET (sock, writeset);
	}
	return 0;
}

std::string Connection::getCameraChipState (int chipN)
{
	int chip_state = (getRealState () & (CAM_MASK_CHIP << (chipN * 4))) >> (chipN * 4);
	std::ostringstream _os;
	if (chip_state == 0)
	{
		_os << chipN << " idle";
	}
	else
	{
		if (chip_state & CAM_EXPOSING)
		{
			_os << chipN << " EXPOSING";
		}
		if (chip_state & CAM_EXPOSING_NOIM)
		{
			if (_os.str ().size ())
				_os << " | ";
			_os << chipN << " EXPOSING NOIMG";
		}
		if (chip_state & CAM_READING)
		{
			if (_os.str ().size ())
				_os << " | ";
			_os << chipN << " READING";
		}
		if (chip_state & CAM_FT)
		{
			if (_os.str ().size ())
				_os << " | ";
			_os << chipN << " FT";
		}
	}
	return _os.str ();
}

std::string Connection::getStateString ()
{
	std::ostringstream _os;
	int real_state = getRealState ();
	int chipN;
	switch (getOtherType ())
	{
		case DEVICE_TYPE_SERVERD:
			_os << CentralState (getState ()).getString ();
			break;
		case DEVICE_TYPE_MOUNT:
			switch (real_state & TEL_MASK_MOVING)
			{
				case TEL_OBSERVING:
					if (real_state & TEL_TRACKING)
						_os << "TRACKING";
					else
						_os << "not tracking";
					break;
				case TEL_MOVING:
					_os << "MOVING";
					break;
				case TEL_PARKED:
					_os << "PARKED";
					break;
				case TEL_PARKING:
					_os << "PARKING";
					break;
				default:
					_os << "unknow state " << real_state;
			}
			if (real_state & TEL_CORRECTING)
				_os << " | CORRECTING";
			if (real_state & TEL_WAIT_CUP)
				_os << " | WAIT_FOR_CUPOLA";
			if (real_state & TEL_NEED_STOP)
				_os << " | NEED_FLIP";
			break;
		case DEVICE_TYPE_CCD:
			chipN = getValueInteger ("chips");
			for (int i = 0; i < chipN; i++)
				_os << std::hex << getCameraChipState (i);
			if (real_state & CAM_HAS_IMAGE)
				_os << " | IMAGE_READY";
			switch (real_state & CAM_MASK_SHUTTER)
			{
				case CAM_SHUT_CLEARED:
					_os << " | SHUTTER_CLEARED";
					break;
				case CAM_SHUT_SET:
					_os << " | SHUTTER_SET";
					break;
				case CAM_SHUT_TRANS:
					_os << " | SHUTTER_TRANS";
					break;
				default:
					_os << " | shutter unknow";
			}
			break;
		case DEVICE_TYPE_DOME:
		case DEVICE_TYPE_CUPOLA:
			switch (real_state & DOME_DOME_MASK)
			{
				case DOME_CLOSED:
					_os << "CLOSED";
					break;
				case DOME_OPENING:
					_os << "OPENING";
					break;
				case DOME_OPENED:
					_os << "OPENED";
					break;
				case DOME_CLOSING:
					_os << "CLOSING";
					break;
				case DOME_WAIT_CLOSING:
					_os << "WAIT FOR CLOSE";
					break;
				default:
					_os << "UNKNOW";
			}
			if (getOtherType () == DEVICE_TYPE_CUPOLA)
			{
				if (real_state & DOME_CUP_MOVE)
					_os << " | CUPOLA_MOVING";
				else
					_os << " | cupola_idle";
				if (real_state & DOME_CUP_SYNC)
					_os << " | cupola_synced";
				else
					_os << " | CUPOLA_NOT_SYNCED";
			}
			break;
		case DEVICE_TYPE_WEATHER:
			_os << "weather " << real_state;
			break;
		case DEVICE_TYPE_ROTATOR:
			switch (real_state & ROT_MASK_ROTATING)
			{
				case ROT_IDLE:
					_os << "idle";
					break;
				case ROT_ROTATING:
					_os << "ROTATING";
					break;
				default:
					_os << "UNKNOWN";	
			}
			break;
		case DEVICE_TYPE_PHOT:
			if (real_state & PHOT_INTEGRATE)
				_os << "INTEGRATING";
			else
				_os << "idle";
			if (real_state & PHOT_FILTER_MOVE)
				_os << " | FILTER_MOVING";
			break;
		case DEVICE_TYPE_PLAN:
			_os << "plan (not supported) ";
			break;
		case DEVICE_TYPE_GRB:
			_os << "grbd " << real_state;
			break;
		case DEVICE_TYPE_FOCUS:
			if (real_state & FOC_FOCUSING)
				_os << "CHANGING";
			else
				_os << "idle";
			break;
		case DEVICE_TYPE_MIRROR:
			switch (real_state & MIRROR_MASK)
			{
				case MIRROR_MOVE:
					_os << "MOVING";
					break;
				case MIRROR_NOTMOVE:
					_os << "idle";
					break;
				case MIRROR_A:
					_os << "A";
					break;
				case MIRROR_A_B:
					_os << "MOVING_A_B";
					break;
				case MIRROR_B:
					_os << "B";
					break;
				case MIRROR_B_A:
					_os << "MOVING_B_A";
					break;
				default:
					_os << "unknow";
			}
			break;
		case DEVICE_TYPE_FW:
			if (real_state & FILTERD_MOVE)
				_os << "MOVING";
			else
				_os << "idle";
			break;
		case DEVICE_TYPE_AUGERSH:
			_os << "augershooter " << real_state;
			break;
		case DEVICE_TYPE_EXECUTOR:
			switch (real_state & EXEC_STATE_MASK)
			{
				case EXEC_IDLE:
					_os << "idle";
					break;
				case EXEC_MOVE:
					_os << "MOVING TO NEXT TARGET";
					break;
				case EXEC_ACQUIRE:
					_os << "ACQUIRING";
					break;
				case EXEC_ACQUIRE_WAIT:
					_os << "ACQUIRE WAIT FOR PROCESSING";
					break;
				case EXEC_OBSERVE:
					_os << "OBSERVING";
					break;
				case EXEC_LASTREAD:
					_os << "OBSERVING IN LAST READ";
					break;
				default:
					_os << "UNKNOW";
			}
			if (real_state & EXEC_END)
				_os << " | WILL ENDS";
			else
				_os << " | not ending";
			switch (real_state & EXEC_MASK_ACQ)
			{
				case EXEC_NOT_ACQ:
					break;
				case EXEC_ACQ_OK:
					_os << " | ACQUSITION OK";
					break;
				case EXEC_ACQ_FAILED:
					_os << " | ACQUSITION FAILED";
					break;
				default:
					_os << " | UNKNOW ACQUSTION " << (real_state & EXEC_MASK_ACQ);
			}
			switch (real_state & EXEC_MASK_SCRIPT)
			{
				case EXEC_SCRIPT_RUNNING:
					_os << " | SCRIPT RUNNING";
					break;
			}
			break;
		case DEVICE_TYPE_IMGPROC:
			if (real_state & IMGPROC_RUN)
				_os << "PROCESS RUNNING";
			else
				_os << "idle";
			break;
		case DEVICE_TYPE_SELECTOR:
			if (real_state & SENSOR_INPROGRESS)
				_os << "CHANGING";
			else
				_os << "idle";
			break;
		case DEVICE_TYPE_XMLRPC:
			switch (real_state & EXEC_MASK_SCRIPT)
			{
				case EXEC_SCRIPT_RUNNING:
					_os << " | SCRIPT RUNNING";
					break;
			}
			break;
		case DEVICE_TYPE_INDI:
			_os << "indi " << real_state;
			break;
		case DEVICE_TYPE_SENSOR:
			_os << "sensor " << real_state;
			break;
		case DEVICE_TYPE_LOGD:
			_os << "logd " << real_state;
			break;
		case DEVICE_TYPE_SCRIPTOR:
			_os << "scriptor " << real_state;
			break;
		case DEVICE_TYPE_BB:
			_os << "bb " << real_state;
			break;
		default:
			_os << "UNKNOW DEVICE " << getOtherType () << " " << real_state;
	}
	if (getState () & DEVICE_ERROR_KILL)
		_os << " | PRIORITY CHANGED";
	if (getState () & DEVICE_ERROR_HW)
		_os << " | HW ERROR";
	if (getState () & DEVICE_NOT_READY)
		_os << " | NOT READY";

	// report bad weather
	if (getState () & BAD_WEATHER)
	  	_os << " | BAD WEATHER";
	if (getState () & WR_RAIN)
		_os << " | rain";
	if (getState () & WR_WIND)
		_os << " | wind";
	if (getState () & WR_HUMIDITY)
		_os << " | humidity";
	if (getState () & WR_CLOUD)
		_os << " | cloud";

	// stop everything..
	if (getState () & STOP_EVERYTHING)
		_os << " | STOP";

	// block states and full BOP states
	if (getState () & BOP_EXPOSURE)
		_os << " | BLOCK EXPOSURE";
	else if (getFullBopState () & BOP_EXPOSURE)
		_os << " # BLOCK EXPOSING";

	if (getState () & BOP_READOUT)
		_os << " | BLOCK READOUT";
	else if (getFullBopState () & BOP_READOUT)
		_os << " # BLOCK READOUT";

	if (getState () & BOP_TEL_MOVE)
		_os << " | BLOCK TELESCOPE MOVEMENT";
	else if (getFullBopState () & BOP_TEL_MOVE)
		_os << " # BLOCK TELESCOPE MOVEMENT";
	
	if (getState () & BOP_WILL_EXPOSE)
	  	_os << " | WILL EXPOSE";
	else if (getFullBopState () & BOP_WILL_EXPOSE)
	  	_os << " # WILL EXPOSE";

	if (getState () & BOP_TRIG_EXPOSE)
	  	_os << " | WAIT TRIG EXPOSE";
	else if (getFullBopState () & BOP_TRIG_EXPOSE)
	  	_os << " # WAIT TRIG EXPOSE";

	return _os.str ();
}

void Connection::postEvent (Event * event)
{
	if (otherDevice)
		otherDevice->postEvent (new Event (event));
	Object::postEvent (event);
}

void Connection::postMaster (Event * event)
{
	master->postEvent (event);
}

int Connection::idle ()
{
	if (connectionTimeout > 0)
	{
		time_t now;
		time (&now);
		if (now > lastData + getConnTimeout ()
			&& now > lastSendReady + getConnTimeout () / 4)
		{
			#ifdef DEBUG_EXTRA
			int ret = sendMsg (PROTO_TECHNICAL " ready");
			std::cout << "Send T ready ret: " << ret <<
				" name: " << getName () << " type: " << type << std::endl;
			#else
			sendMsg (PROTO_TECHNICAL " ready");
			#endif				 /* DEBUG_EXTRA */
			time (&lastSendReady);
		}
		if (now > (lastData + getConnTimeout () * 2))
		{
			logStream (MESSAGE_DEBUG) << "Connection timeout: " << lastGoodSend
				<< " " << lastData << " " << now << " " << getName () << " " <<
				type << sendLog;
			connectionError (-1);
		}
	}
	if (otherDevice != NULL)
		otherDevice->idle ();
	return 0;
}

int Connection::authorizationOK ()
{
	logStream (MESSAGE_ERROR) << "authorization called on wrong connection" <<
		sendLog;
	return -1;
}

int Connection::authorizationFailed ()
{
	logStream (MESSAGE_ERROR) << "authorization failed on wrong connection" <<
		sendLog;
	return -1;
}

void Connection::checkBufferSize ()
{
	// increase buffer if it's too small
	if (((int) buf_size) == (buf_top - buf))
	{
		char *new_buf = new char[buf_size + MAX_DATA + 1];
		memcpy (new_buf, buf, buf_size);
		buf_top = new_buf + (buf_top - buf);
		buf_size += MAX_DATA;
		delete[]buf;
		buf = new_buf;
	}
}

int Connection::acceptConn ()
{
	int new_sock;
	struct sockaddr_in other_side;
	socklen_t addr_size = sizeof (struct sockaddr_in);
	new_sock = accept (sock, (struct sockaddr *) &other_side, &addr_size);
	if (new_sock == -1)
	{
		logStream (MESSAGE_ERROR) << "Connection::acceptConn data accept " <<
			strerror (errno) << sendLog;
		return -1;
	}
	else
	{
		close (sock);
		sock = new_sock;
		#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "Connection::acceptConn connection accepted"
			<< sendLog;
		#endif
		setConnState (CONN_CONNECTED);
		return 0;
	}
}

void Connection::setState (rts2_status_t in_value, char * msg)
{
	serverState->setValue (in_value);
	if (otherDevice)
		otherDevice->stateChanged (serverState);
	if (serverState->maskValueChanged (DEVICE_NOT_READY))
	{
		if ((serverState->getValue () & DEVICE_NOT_READY) == 0)
			master->deviceReady (this);
	}
	else if (serverState->maskValueChanged (DEVICE_STATUS_MASK)
		&& (serverState->getValue () & DEVICE_STATUS_MASK) == DEVICE_IDLE)
	{
		master->deviceIdle (this);
	}
}

void Connection::setBopState (rts2_status_t in_value)
{
	bopState->setValue (in_value);
	if (runningCommand && runningCommand->getStatusCallProgress () == CIP_RUN)
		sendCommand ();
}

void Connection::setOtherType (int other_device_type)
{
	delete otherDevice;
	otherDevice = master->createOtherType (this, other_device_type);
	otherType = other_device_type;
}

int Connection::getOtherType ()
{
	if (otherDevice)
		return otherType;
	return -1;
}

void Connection::updateStatusWait (Connection * conn)
{
	if (runningCommand && runningCommand->getStatusCallProgress () == CIP_WAIT)
		sendCommand ();
}

void Connection::masterStateChanged ()
{
	if (runningCommand && runningCommand->getStatusCallProgress () == CIP_RUN)
		sendCommand ();
}

void Connection::processLine ()
{
	// starting at command_start, we have complete line, which was
	// received
	int ret;

	// find command parameters end

	while (*command_buf_top && !isspace (*command_buf_top))
		command_buf_top++;
	// mark command end..
	if (*command_buf_top)
	{
		*command_buf_top = '\0';
		command_buf_top++;
	}
	// status combined with progress
	if (isCommand (PROTO_STATUS_PROGRESS))
	{
		ret = statusProgress ();
	}
	// status
	else if (isCommand (PROTO_STATUS))
	{
		ret = status ();
	}
	// bop status update
	else if (isCommand (PROTO_BOP_STATE))
	{
		ret = bopStatus ();
	}
	// message from application
	else if (isCommand (PROTO_MESSAGE))
	{
		ret = message ();
	}
	// technical - to keep connection working
	else if (isCommand (PROTO_TECHNICAL))
	{
		char *msg;
		if (paramNextString (&msg) || !paramEnd ())
		{
			ret = -2;
		}
		else if (!strcmp (msg, "ready"))
		{
			#ifdef DEBUG_EXTRA
			std::cout << "Send T OK" << std::endl;
			#endif
			sendMsg (PROTO_TECHNICAL " OK");
			ret = -1;
		}
		else if (!strcmp (msg, "OK"))
		{
			ret = -1;
		}
		else
		{
			ret = -2;
		}
	}
	// metainfo with values
	else if (isCommand (PROTO_METAINFO))
	{
		int m_type;
		char *m_name;
		char *m_descr;
		if (paramNextInteger (&m_type)
			|| paramNextString (&m_name)
			|| paramNextString (&m_descr) || !paramEnd ())
		{
		 	ret = -2;
		}
		else
		{
			ret = metaInfo (m_type, std::string (m_name), std::string (m_descr));
		}
	}
	else if (isCommand (PROTO_VALUE))
	{
		char *m_name;
		if (paramNextString (&m_name))
		{
			logStream (MESSAGE_DEBUG) << "Cannot get parameter for SET_VALUE on connection " << getCentraldId () << sendLog;
			ret = -1;
		}
		else
		{
			commandValue (m_name);
			ret = -1;
		}
	}
	else if (isCommand (PROTO_SELMETAINFO))
	{
		char *m_name;
		char *sel_name;
		if (paramNextString (&m_name))
		{
		  	ret = -2;
		}
		else if (paramEnd ())
		{
			ret = selMetaClear (m_name);
		}
		else if (paramNextString (&sel_name) || !paramEnd ())
		{
			ret = -2;
		}
		else
		{
			ret = selMetaInfo (m_name, sel_name);
		}
	}
	else if (isCommand (PROTO_SET_VALUE))
	{
		ret = master->setValue (this);
	}
	else if (isCommand (PROTO_BINARY))
	{
		int data_conn;
		// we expect binary data
		if (paramNextInteger (&data_conn))
		{
			// end connection - we cannot process this command
			activeReadData = -1;
			connectionError (-2);
			ret = -2;
		}
		else
		{
			DataChannels * chann = new DataChannels ();
			chann->initFromConnection (this);
			readChannels[data_conn] = chann;
			newDataConn (data_conn);
			ret = -1;
		}
	}
	else if (isCommand (PROTO_DATA))
	{
		if (paramNextInteger (&activeReadData) || paramNextInteger (&activeReadChannel)
			|| readChannels[activeReadData]->readChannel (activeReadChannel, this)
			|| !paramEnd ())
		{
			// end connection - bad binary data header
			activeReadData = -1;
			connectionError (-2);
			ret = -2;
		}
		else
		{
			ret = -1;
		}
	}
	else if (isCommand (PROTO_BINARY_KILLED))
	{
		int dC;
		if (paramNextInteger (&dC))
		{
			connectionError (-2);
			ret = -2;
		}
		else
		{
			std::map <int, DataChannels *>::iterator iter = readChannels.find (dC);
			if (iter != readChannels.end ())
			{
				if (otherDevice)
				{
					otherDevice->fullDataReceived (dC, iter->second);
				}
				delete iter->second;
				readChannels.erase (iter);
			}
			ret = -1;
		}
	}
	else if (isCommand (PROTO_SHARED))
	{
		int dC;
		int sharedMem;
		if (paramNextInteger (&dC) || paramNextInteger (&sharedMem))
		{
			connectionError (-2);
			ret = -2;
		}
		else
		{
			ret = -1;
			if (sharedReadMemory && sharedMem != sharedReadMemory->getShmId ())
			{
				// unmap existing IPC, map new one
				delete sharedReadMemory;
				sharedReadMemory = NULL;
			}

			if (sharedReadMemory == NULL)
			{
				sharedReadMemory = new DataSharedRead ();
				if (sharedReadMemory->attach (sharedMem))
				{
					connectionError (-2);
					ret = -2;
				}
			}
			if (ret == -1)
			{
				DataChannels * chann = new DataChannels ();
				chann->initSharedFromConnection (this, sharedReadMemory);
				readChannels[dC] = chann;
				newDataConn (dC);
			}
		}
	}
	else if (isCommand (PROTO_SHARED_FULL) || isCommand (PROTO_SHARED_KILLED))
	{
		int dC;
		if (paramNextInteger (&dC) || !paramEnd ())
		{
			connectionError (-2);
			ret = -2;
		}
		else
		{
			std::map <int, DataChannels *>::iterator iter = readChannels.find (dC);
			if (iter != readChannels.end ())
			{
				if (otherDevice)
				{
					otherDevice->fullDataReceived (dC, iter->second);
				}
				for (DataChannels::iterator dch_iter = iter->second->begin (); dch_iter != iter->second->end (); dch_iter++)
				{
					((DataSharedRead *) (*dch_iter))->removeActiveClient (getMaster ()->getSingleCentralConn ()->getCentraldId ());
				}
				delete iter->second;
				readChannels.erase (iter);
			}
			ret = -1;
		}
	}
	else if (isCommand (COMMAND_DATA_IN_FITS))
	{
		char *fn;
		if (paramNextString (&fn) || !paramEnd ())
		{
			connectionError (-2);
			ret = -2;
		}
		else
		{
			if (otherDevice)
				otherDevice->fitsData (fn);
			ret = 0;
		}
	}
	else if (isCommandReturn ())
	{
		ret = commandReturn ();
	}
	else
	{
		setCommandInProgress (true);
		ret = command ();
	}
	#ifdef DEBUG_ALL
	std::cerr << "Connection::processLine [" << getCentraldId ()
		<< "] command: " << getCommand () << " ret: " << ret << std::endl;
	#endif
	if (!ret)
		sendCommandEnd (DEVDEM_OK, "OK");
	else if (ret == -2)
	{
		logStream (MESSAGE_DEBUG) << "Connection::processLine [" << getCentraldId () << "] command: " << getCommand () << " ret: " << ret << sendLog;
		sendCommandEnd (DEVDEM_E_COMMAND, (std::string ("invalid parameters/invalid number of parameters - ") + getCommand ()).c_str ());
	}
}

void Connection::processBuffer ()
{
	if (full_data_end)
		return;
	full_data_end = buf_top;
	buf_top = buf;
	command_start = buf;
	while (*buf_top)
	{
		while (isspace (*buf_top) || (*buf_top && *buf_top == '\n'))
			buf_top++;
		command_start = buf_top;
		// find command end..
		while (*buf_top && *buf_top != '\n' && *buf_top != '\r')
			buf_top++;

		if (*buf_top == '\r' && *(buf_top + 1) == '\n')
		{
			*buf_top = '\0';
			buf_top++;
		}
		// weird error on when we get \r in one and \n in next read
		if (*buf_top == '\r' && !*(buf_top + 1))
		{
			// we get to 0, while will ends, and we get out, waiting for \n in next packet
			buf_top++;
			break;
		}

		if (*buf_top == '\n')
		{
			// mark end of line..
			*buf_top = '\0';
			buf_top++;

			command_buf_top = command_start;

			processLine ();
			// binary read just started
			if (activeReadData >= 0)
			{
				long readSize = full_data_end - buf_top;
				readSize = readChannels[activeReadData]->addData (activeReadChannel, buf_top, readSize);
				dataReceived ();
				// move binary data away
				memmove (buf_top, buf_top + readSize, (full_data_end - buf_top) - readSize + 1);
				full_data_end -= readSize;
			}
			command_start = buf_top;
		}
	}
	if (buf != command_start)
	{
		memmove (buf, command_start, (full_data_end - command_start) + 1);
		// move buffer to the end..
		buf_top -= command_start - buf;
	}
	full_data_end = NULL;
}

int Connection::receive (fd_set * readset)
{
	int data_size = 0;
	// connections market for deletion
	if (isConnState (CONN_DELETE))
		return -1;
	if ((sock >= 0) && FD_ISSET (sock, readset))
	{
		if (isConnState (CONN_CONNECTING))
		{
			return acceptConn ();
		}
		// we are receiving binary data
		if (activeReadData >= 0)
		{
			data_size = readChannels[activeReadData]->getData (activeReadChannel, sock);
			if (data_size == -1)
			{
				connectionError (data_size);
			}
			if (data_size == 0)
				return 0;
			dataReceived ();
			return data_size;
		}
		checkBufferSize ();
		data_size = read (sock, buf_top, buf_size - (buf_top - buf));
		// ignore EINTR
		if (data_size == -1 && errno == EINTR)
			return 0;
		if (data_size <= 0)
		{
			connectionError (data_size);
			return -1;
		}
		buf_top[data_size] = '\0';
		successfullRead ();
		#ifdef DEBUG_ALL
		std::cout << "Connection::receive name " << getName ()
			<< " reas: " << buf_top
			<< " full_buf: " << buf
			<< " size: " << data_size
			<< " commandInProgress " << commandInProgress
			<< " runningCommand " << runningCommand
			<< std::endl;
		#endif
		// move buf_top to end of readed data
		buf_top += data_size;
		data_size += buf_top - buf;
		processBuffer ();
	}
	return data_size;
}

int Connection::writable (fd_set * writeset)
{
	if (sock >=0 && FD_ISSET (sock, writeset) && isConnState (CONN_INPROGRESS))
	{
		int err = 0;
		int ret;
		socklen_t len = sizeof (err);

		ret = getsockopt (sock, SOL_SOCKET, SO_ERROR, &err, &len);
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Connection::idle getsockopt " <<
				strerror (errno) << sendLog;
			connectionError (-1);
		}
		else if (err)
		{
			logStream (MESSAGE_ERROR) << "Connection::idle getsockopt " <<
				strerror (errno) << sendLog;
			connectionError (-1);
		}
		else
		{
			connConnected ();
		}
	}
	return 0;
}

int Connection::getOurAddress (struct sockaddr_in *in_addr)
{
	// get our address and pass it to data conn
	socklen_t size;
	size = sizeof (struct sockaddr_in);

	return getsockname (sock, (struct sockaddr *) in_addr, &size);
}

void Connection::setAddress (struct in_addr *in_address)
{
	addr = *in_address;
}

void Connection::getAddress (char *addrBuf, int _buf_size)
{
	char *addr_s;
	addr_s = inet_ntoa (addr);
	strncpy (addrBuf, addr_s, _buf_size);
	addrBuf[_buf_size - 1] = '0';
}

void Connection::setCentraldId (int in_centrald_id)
{
	centrald_id = in_centrald_id;
}

void Connection::queCommand (Command * cmd, int notBop, Object * originator)
{
	cmd->setBopMask (notBop);
	cmd->setOriginator (originator);
	queCommand (cmd);
}

void Connection::queCommand (Command * cmd)
{
	cmd->setConnection (this);
	if (runningCommand
		|| isConnState (CONN_CONNECTING)
		|| isConnState (CONN_INPROGRESS)
		|| isConnState (CONN_AUTH_PENDING)
		|| isConnState (CONN_UNKNOW))
	{
		commandQue.push_back (cmd);
		return;
	}
	runningCommand = cmd;
	sendCommand ();
}

void Connection::queSend (Command * cmd)
{
	cmd->setConnection (this);
	if (isConnState (CONN_CONNECTING)
		|| isConnState (CONN_INPROGRESS)
		|| isConnState (CONN_UNKNOW))
	{
		commandQue.push_front (cmd);
		return;
	}
	if (runningCommand)
	{
		// when the actual command was sended or if the actual command
		// is returning, push us to fron of que to be executed as first
		// command
		if (runningCommandStatus == SEND || runningCommandStatus == RETURNING)
		{
			commandQue.push_front (cmd);
			return;
		}
		commandQue.push_front (runningCommand);
	}
	runningCommand = cmd;
	sendCommand ();
}

void Connection::commandReturn (Command * cmd, int in_status)
{
	if (otherDevice)
		otherDevice->commandReturn (cmd, in_status);
}

bool Connection::queEmptyForOriginator (Object *testOriginator)
{
	if (runningCommand && runningCommandStatus != RETURNING && runningCommand->isOriginator (testOriginator))
		return false;
	for (std::list <Command *>::iterator iter = commandQue.begin (); iter != commandQue.end (); iter++)
	{
		if ((*iter)->isOriginator (testOriginator))
			return false;
	}
	return true;
}

void Connection::queClear ()
{
	if (runningCommand && runningCommandStatus != SEND)
	{
		delete runningCommand;
		runningCommand = NULL;
	}
	std::list < Command * >::iterator que_iter;
	for (que_iter = commandQue.begin (); que_iter != commandQue.end (); que_iter++)
	{
		Command *cmd;
		cmd = (*que_iter);
		delete cmd;
	}
	commandQue.clear ();
}

void Connection::deleteConnection (Connection *conn)
{
	// look if connection isn't entered among originators or controll connections..
	if (runningCommand)
		runningCommand->deleteConnection (conn);
	for (std::list < Command * >::iterator iter = commandQue.begin (); iter != commandQue.end (); iter++)
	{
		(*iter)->deleteConnection (conn);
	}
	if (otherDevice)
		otherDevice->deleteConnection (conn);
}

// high-level commands, used to pass variables etc..
int Connection::command ()
{
	if (isCommand ("device"))
	{
		int p_centrald_num;
		int p_centraldId;
		char *p_name;
		char *p_host;
		int p_port;
		int p_device_type;
		if (paramNextInteger (&p_centrald_num)
		  	|| paramNextInteger (&p_centraldId)
			|| paramNextString (&p_name)
			|| paramNextString (&p_host)
			|| paramNextInteger (&p_port)
			|| paramNextInteger (&p_device_type)
			|| !paramEnd ())
			return -2;
		master->addAddress (getCentraldNum (), p_centrald_num, p_centraldId, p_name, p_host, p_port, p_device_type);
		setCommandInProgress (false);
		return -1;
	}
	else if (isCommand ("delete_device"))
	{
		int p_centrald_num;
		char *p_name;
		if (paramNextInteger (&p_centrald_num) || paramNextString (&p_name) || !paramEnd ())
			return -2;
		master->deleteAddress (p_centrald_num, p_name);
		return -1;
	}
	else if (isCommand ("client"))
	{
		int p_centraldId;
		char *p_login;
		char *p_name;
		if (paramNextInteger (&p_centraldId)
			|| paramNextString (&p_login)
			|| paramNextString (&p_name)
			|| !paramEnd ())
			return -2;
		master->addClient (p_centraldId, p_login, p_name);
		setCommandInProgress (false);
		return -1;
	}
	else if (isCommand ("delete_client"))
	{
		int p_centraldId;
		if (paramNextInteger (&p_centraldId) || !paramEnd ())
			return -2;
		master->deleteClient (p_centraldId);
		return -1;
	}
	else if (isCommand ("status_info"))
	{
		if (!paramEnd ())
			return -2;
		return master->statusInfo (this);
	}
	else if (isCommand (PROTO_PROGRESS))
	{
		if (paramNextDouble (&statusStart)
		  	|| paramNextDouble (&statusExpectedEnd)
			|| !paramEnd ())
			return -2;
		return master->progress (this, statusStart, statusExpectedEnd);
	}
	// don't respond to values with error - otherDevice does respond to
	// values, if there is no other device, we have to take resposibility
	// as it can fails (V without value), not with else
	if (isCommand (PROTO_VALUE))
		return -1;
	std::ostringstream ss;
	ss << "unknow command " << getCommand ();
	logStream (MESSAGE_DEBUG) << "Connection::command unknow command: getCommand " << getCommand () << " state: " << conn_state << " type: " << getType () << " name: " << getName () << sendLog;
	sendCommandEnd (-4, ss.str ().c_str ());
	return -4;
}

int Connection::statusProgress ()
{
	long long int value;
	char *msg = NULL;

	if (paramNextLongLong (&value) || paramNextDouble (&statusStart) || paramNextDouble (&statusExpectedEnd) || !(paramEnd() || (paramNextString (&msg) == 0 && paramEnd ())))
		return -2;
	//std::cerr << "Connection::statusProgress " << value << " " << statusStart << " " << statusExpectedEnd << std::endl;	
	setState (value, msg);
	master->progress (this, statusStart, statusExpectedEnd);
	return -1;
}

int Connection::status ()
{
	long long int value;
	char *msg = NULL;
	if (paramNextLongLong (&value) || !(paramEnd () || (paramNextString (&msg) == 0 && paramEnd ())))
		return -2;
	//std::cerr << "Connection::status " << value << std::endl;	
	setState (value, msg);
	return -1;
}

int Connection::bopStatus ()
{
	long long int masterStatus;
	long long int masterBopState;
	if (paramNextLongLong (&masterStatus) || paramNextLongLong (&masterBopState) || !paramEnd ())
		return -2;
	//std::cerr << "Connection::bopStatus " << masterStatus << " " << masterBopState << std::endl;	
	setBopState (masterBopState);
	setState (masterStatus, NULL);
	return -1;
}

int Connection::message ()
{
	struct timeval messageTime;
	char *messageOName;
	int messageType;

	if (paramNextTimeval (&messageTime)
		|| paramNextString (&messageOName) || paramNextInteger (&messageType))
		return -2;

	Message rts2Message = Message
		(messageTime, std::string (messageOName), messageType,
		std::string (paramNextWholeString ()));

	master->message (rts2Message);
	// message is always processed and do not need OK return
	return -1;
}

void Connection::sendCommand ()
{
	CommandDeviceStatus *statInfoCall;
	// we require some special state before command can be executed
	if (runningCommand->getBopMask ())
	{
		// we are waiting for some BOP mask and it have already occured
		if (runningCommand->getBopMask () & BOP_WHILE_STATE)
		{
			#ifdef DEBUG_ALL
			std::cout << "waiting for " << runningCommand->getText () << " "
				<< std::hex << runningCommand->getBopMask () << " "
				<< std::hex << getMaster()->getMasterStateFull ()
				<< " " << getFullBopState ()
				<< " " << runningCommand
				<< std::endl;
			#endif
			if (getFullBopState () & runningCommand->getBopMask () & BOP_MASK)
			{
				// just wait for finish
				if (runningCommand->getStatusCallProgress () == CIP_WAIT)
				{
					runningCommandStatus = WAITING;
					return;
				}
				#ifdef DEBUG_ALL
				logStream (MESSAGE_DEBUG) << "executing " << runningCommand->getText () << " " << runningCommand << sendLog;
				#endif
				runningCommand->send ();
				runningCommandStatus = SEND;
				runningCommand->setStatusCallProgress (CIP_WAIT);
			}
			else
			{
				// signal connection we are waiting for state changed
				runningCommand->setStatusCallProgress (CIP_RUN);
			}
			return;
		}
		switch (runningCommand->getStatusCallProgress ())
		{
			case CIP_NOT_CALLED:
				statInfoCall = new CommandDeviceStatus (getMaster (), this);
				// we can do that, as if we are running on same connection as is centrald, we are runningCommand, so we can send directly..
				statInfoCall->setConnection (this);
				statInfoCall->setStatusCallProgress (CIP_RETURN);
				statInfoCall->send ();
				runningCommand->setStatusCallProgress (CIP_WAIT);
				commandQue.push_front (runningCommand);
				runningCommand = statInfoCall;
				runningCommandStatus = SEND;
				break;
			case CIP_WAIT:
				// if the bock bit is still set..
				runningCommand->setStatusCallProgress (CIP_RUN);
			case CIP_RUN:
				if (getFullBopState () & runningCommand->getBopMask () & BOP_MASK)
				{
					runningCommandStatus = WAITING;
					break;
				}
				runningCommand->send ();
				runningCommandStatus = SEND;
				runningCommand->setStatusCallProgress (CIP_RETURN);
				break;
			case CIP_RETURN:
				// do nothing, it's status command, which return back, or command waiting for return
				break;
		}
	}
	else
	{
		runningCommand->send ();
		runningCommandStatus = SEND;
	}
}

int Connection::sendNextCommand ()
{
	// pop next command
	if (commandQue.size () > 0)
	{
		runningCommand = *(commandQue.begin ());
		commandQue.erase (commandQue.begin ());
		sendCommand ();
		return 0;
	}
	runningCommand = NULL;
	return -1;
}

int Connection::commandReturn ()
{
	int ret;
	int stat = atoi (getCommand ());
	// log retuns recieved without command
	if (!runningCommand)
	{
		logStream (MESSAGE_ERROR) << "Connection::commandReturn null on connection with '"
			<< getName ()
			<< "' (" << getCentraldId ()
			<< ") and status " << stat
			<<"!" << sendLog;
		return -1;
	}
	runningCommandStatus = RETURNING;
	commandReturn (runningCommand, stat);
	ret = runningCommand->commandReturn (stat, this);
	switch (ret)
	{
		case RTS2_COMMAND_REQUE:
			sendCommand ();
			break;
		case -1:
			delete runningCommand;
			sendNextCommand ();
			break;
	}
	return -1;
}

int Connection::sendMsg (const char *msg)
{
	int len;
	int ret;
	if (sock == -1)
		return -1;
	len = strlen (msg) + 1;
	char *mbuf = new char[len + 1];
	strcpy (mbuf, msg);
	strcat (mbuf, "\n");
	// ignore EINTR
	do
	{
		ret = write (sock, mbuf, len);
	} while (ret == -1 && errno == EINTR);

	if (ret != len)
	{
		syslog (LOG_ERR, "Cannot send msg: %s to sock %i with len %i, ret %i errno %i message %m",
			msg, sock, len, ret, errno);
		#ifdef DEBUG_EXTRA
		logStream (MESSAGE_ERROR)
			<< "Connection::send [" << getCentraldId () << ":" << conn_state << "] error "
			<< sock << " state: " << ret << " sending " << msg << ":" << strerror (errno)
			<< sendLog;
		#endif
		connectionError (ret);
		delete[] mbuf;
		return -1;
	}
	#ifdef DEBUG_ALL
	std::cout << "Connection::send " << getName ()
		<< " [" << getCentraldId () << ":" << sock << "] send " << ret << ": " << msg
		<< std::endl;
	#endif

	delete[] mbuf;
	successfullSend ();
	return 0;
}

int Connection::sendMsg (std::string msg)
{
	return sendMsg (msg.c_str ());
}

int Connection::sendMsg (std::ostringstream &_os)
{
	return sendMsg (_os.str ().c_str ());
}

int Connection::startBinaryData (int dataType, int channum, size_t *chansize)
{
	std::ostringstream _os;
	dataConn++;
	_os << PROTO_BINARY " " << dataConn << " " << dataType << " " << channum;
	for (int i = 0; i < channum; i++)
		_os << " " << chansize[i];
	int ret;
	ret = sendMsg (_os);
	if (ret == -1)
		return -1;
	writeChannels[dataConn] = new DataWrite (channum, chansize);
	return dataConn;
}

int Connection::sendBinaryData (int data_conn, int chan, char *data, size_t dataSize)
{
	char *binaryWriteTop, *binaryWriteBuff;
	binaryWriteTop = binaryWriteBuff = data;
	char *binaryEnd = data + dataSize;

	std::ostringstream _os;
	_os << PROTO_DATA " " << data_conn << " " << chan << " " << dataSize;
	int ret;
	ret = sendMsg (_os);
	if (ret)
		return ret;

	while (binaryWriteTop < binaryEnd)
	{
		if (dataSize > getWriteBinaryDataSize (data_conn))
		{
			logStream (MESSAGE_ERROR) << "Attemp to send too much data - "
				<< dataSize << "bytes, but there are only " << getWriteBinaryDataSize (data_conn) << " bytes remain to be send" << sendLog;
			dataSize = getWriteBinaryDataSize (data_conn);
		}
		ret = send (sock, binaryWriteTop, dataSize, 0);
		if (ret == -1)
		{
			if (errno != EINTR)
			{
				connectionError (ret);
				return -1;
			}
		}
		else
		{
			binaryWriteTop += ret;
			dataSize -= ret;
			std::map <int, DataAbstractWrite *>::iterator iter = writeChannels.find (data_conn);
			if (iter != writeChannels.end ())
			{
				((*iter).second)->dataWritten (chan, ret);
				if (((*iter).second)->getDataSize () <= 0)
				{
					delete ((*iter).second);
					writeChannels.erase (iter);
				}
			}
		}
	}
	return 0;
}

void Connection::endBinaryData (int data_conn)
{
	std::ostringstream _os;
	_os << PROTO_BINARY_KILLED " " << data_conn;
	delete writeChannels[data_conn];
	writeChannels.erase (data_conn);
	sendMsg (_os);
}

int Connection::startSharedData (DataSharedWrite *data, int channum, int *segnums)
{
	std::ostringstream _os;
	dataConn++;
	_os << PROTO_SHARED " " << dataConn << " " << data->getShmId () << " " << channum;
	for (int i = 0; i < channum; i++)
		_os << " " << segnums[i];
	int ret;
	ret = sendMsg (_os);
	if (ret == -1)
		return -1;
	writeChannels[dataConn] = new DataSharedWrite (data);
	return dataConn;
}

void Connection::endSharedData (int data_conn, bool complete)
{
	std::ostringstream _os;
	if (complete)
	{
		_os << PROTO_SHARED_FULL " ";
	}
	else
	{
		_os << PROTO_SHARED_KILLED " ";
		writeChannels[data_conn]->endChannels ();
	}
		
	_os << data_conn;
	delete writeChannels[data_conn];
	writeChannels.erase (data_conn);
	sendMsg (_os);
}

int Connection::fitsDataTransfer (const char *fn)
{
	std::ostringstream _os;
	chmod (fn, 0666);
	_os << COMMAND_DATA_IN_FITS " " << fn;
	sendMsg (_os);
	return 0;
}

void Connection::successfullSend ()
{
	time (&lastGoodSend);
}

void Connection::getSuccessSend (time_t * in_t)
{
	*in_t = lastGoodSend;
}

bool Connection::reachedSendTimeout ()
{
	time_t now;
	time (&now);
	return now > lastGoodSend + getConnTimeout ();
}

void Connection::successfullRead ()
{
	time (&lastData);
}

void Connection::connConnected ()
{
}

void Connection::connectionError (int last_data_size)
{
	activeReadData = -1;
	if (canDelete ())
		setConnState (CONN_DELETE);
	else
		setConnState (CONN_BROKEN);
	if (sock >= 0)
		close (sock);
	sock = -1;
	if (strlen (getName ()))
		master->deleteAddress (getCentraldNum (), getName ());
}

int Connection::sendMessage (Message & msg)
{
	return sendMsg (msg.toConn ());
}

int Connection::sendValue (std::string val_name, int value)
{
	std::ostringstream _os;
	_os << PROTO_VALUE " " << val_name << " " << value;
	return sendMsg (_os);
}

int Connection::sendValue (std::string val_name, int val1, double val2)
{
	std::ostringstream _os;
	_os.setf (std::ios_base::fixed, std::ios_base::floatfield);
	_os.precision (20);
	_os << PROTO_VALUE " " << val_name << " " << val1
		<< " " << val2;
	return sendMsg (_os);
}

int Connection::sendValue (std::string val_name, const char *value)
{
	if (getConnState () == CONN_INPROGRESS)
	{
		return -1;
	}
	std::ostringstream _os;
	_os << PROTO_VALUE " " << val_name << " \"" << value << "\"";
	return sendMsg (_os);
}

int Connection::sendValueRaw (std::string val_name, const char *value)
{
	if (getConnState () == CONN_INPROGRESS)
	{
		return -1;
	}
	std::ostringstream _os;
	_os << PROTO_VALUE " " << val_name << " " << value;
	return sendMsg (_os);
}

int Connection::sendValue (std::string val_name, double value)
{
	std::ostringstream _os;
	_os.setf (std::ios_base::fixed, std::ios_base::floatfield);
	_os.precision (20);
	_os << PROTO_VALUE " " << val_name << " " << value;
	return sendMsg (_os);
}

int Connection::sendValue (char *val_name, char *val1, int val2)
{
	std::ostringstream _os;
	_os << PROTO_VALUE " " << val_name << " \"" << val1 << "\" " << val2;
	return sendMsg (_os);
}

int Connection::sendValue (char *val_name, int val1, int val2, double val3, double val4, double val5, double val6)
{
	std::ostringstream _os;
	_os.setf (std::ios_base::fixed, std::ios_base::floatfield);
	_os.precision (20);
	_os << PROTO_VALUE " " << val_name << " "
		<< val1 << " " << val2 << " "
		<< val3 << " " << val4 << " "
		<< val5 << " " << val6;
	return sendMsg (_os);
}

int Connection::sendValueTime (std::string val_name, time_t * value)
{
	std::ostringstream _os;
	_os << PROTO_VALUE " " << val_name << " " << *value;
	return sendMsg (_os);
}

int Connection::sendProgress (double start, double end)
{
	std::ostringstream _os;
	_os << PROTO_PROGRESS << " " << std::fixed << start << " " << end;
	return sendMsg (_os);
}

int Connection::sendCommandEnd (int num, const char *in_msg)
{
	std::ostringstream _os;
	if (num == 0)
		_os << "+0";
	else
		_os << std::showpos << num;
	_os << " " << in_msg;
	sendMsg (_os);
	if (commandInProgress)
	{
		setCommandInProgress (false);
		processBuffer ();
	}
	if (num < 0)
		logStream (MESSAGE_ERROR) << "command end with error " << num << " description: " << in_msg << sendLog;
	else if (num > 0)
		logStream (MESSAGE_DEBUG) << "command end with note " << num << " description: " << in_msg << sendLog;
	return 0;
}

void Connection::setConnState (conn_state_t new_conn_state)
{
	if (new_conn_state == CONN_AUTH_OK || new_conn_state == CONN_CONNECTED)
	{
		if (!runningCommand)
			sendNextCommand ();
		// otherwise command will be send after command which trigerred
		// state change finished..
	}
	conn_state = new_conn_state;
	if (new_conn_state == CONN_AUTH_FAILED)
	{
		connectionError (-1);
	}
}

int Connection::isConnState (conn_state_t in_conn_state)
{
	return (conn_state == in_conn_state);
}

bool Connection::paramEnd ()
{
	while (isspace (*command_buf_top))
		command_buf_top++;
	return !*command_buf_top;
}

int Connection::paramNextString (char **str, const char *enddelim)
{
	while (isspace (*command_buf_top))
		command_buf_top++;
	if (!*command_buf_top)
		return -1;
	// start of string with spaces
	if (*command_buf_top == '"')
	{
		command_buf_top++;
		*str = command_buf_top;
		while (*command_buf_top && *command_buf_top != '"')
			command_buf_top++;
	}
	else
	{
		// eat next spaces
		*str = command_buf_top;
		while (!isspace (*command_buf_top)
		  && *command_buf_top
		  && (enddelim == NULL || strchr (enddelim, *command_buf_top) == NULL))
			command_buf_top++;
	}
	if (*command_buf_top)
	{
		*command_buf_top = '\0';
		command_buf_top++;
	}
	return 0;
}

char * Connection::paramNextWholeString ()
{
	while (isspace (*command_buf_top))
		command_buf_top++;
	return command_buf_top;
}

int Connection::paramNextStringNull (char **str)
{
	int ret;
	ret = paramNextString (str);
	if (ret)
		return !paramEnd ();
	return ret;
}

int Connection::paramNextInteger (int *num)
{
	char *str_num;
	char *num_end;
	if (paramNextString (&str_num, ","))
		return -1;
	*num = strtol (str_num, &num_end, 10);
	if (*num_end)
		return -1;
	return 0;
}

int Connection::paramNextLong (long int *num)
{
	char *str_num;
	char *num_end;
	if (paramNextString (&str_num, ","))
		return -1;
	*num = strtol (str_num, &num_end, 10);
	if (*num_end)
		return -1;
	return 0;
}

int Connection::paramNextLongLong (long long int *num)
{
	char *str_num;
	char *num_end;
	if (paramNextString (&str_num, ","))
		return -1;
	*num = strtoll (str_num, &num_end, 10);
	if (*num_end)
		return -1;
	return 0;
}

int Connection::paramNextSizeT (size_t * num)
{
	char *str_num;
	char *num_end;
	if (paramNextString (&str_num))
		return -1;
	*num = strtol (str_num, &num_end, 10);
	if (*num_end)
		return -1;
	return 0;
}

int Connection::paramNextSSizeT (ssize_t * num)
{
	char *str_num;
	char *num_end;
	if (paramNextString (&str_num))
		return -1;
	*num = strtol (str_num, &num_end, 10);
	if (*num_end)
		return -1;
	return 0;
}

int Connection::paramNextDouble (double *num)
{
	char *str_num;
	int ret;
	if (paramNextString (&str_num, ","))
		return -1;
	if (!strcmp (str_num, "nan"))
	{
		*num = NAN;
		return 0;
	}
	ret = sscanf (str_num, "%lf", num);
	if (ret != 1)
		return -1;
	return 0;
}

int Connection::paramNextDoubleTime (double *num)
{
	char *str_num;
	int ret;
	if (paramNextString (&str_num, ","))
		return -1;
	if (!strcmp (str_num, "nan"))
	{
		*num = NAN;
		return 0;
	}
	ret = sscanf (str_num, "%lf", num);
	if (ret != 1)
		return -1;
	if (str_num[0] == '+')
		*num += getNow ();
	return 0;
}

int Connection::paramNextFloat (float *num)
{
	char *str_num;
	int ret;
	if (paramNextString (&str_num, ","))
		return -1;
	ret = sscanf (str_num, "%f", num);
	if (ret != 1)
		return -1;
	return 0;
}

int Connection::paramNextHMS (double *num)
{
	char *str_num;
	if (paramNextString (&str_num))
		return -1;
	double mul;
	*num = parseDMS (str_num, &mul);
	if (errno != 0)
		return -1;
	// convert to degrees..
	if (fabs (mul) != 1)
		*num *= 15.0;
	return 0;
}

int Connection::paramNextDMS (double *num)
{
	char *str_num;
	if (paramNextString (&str_num))
		return -1;
	double mul;
	*num = parseDMS (str_num, &mul);
	if (errno != 0)
		return -1;
	return 0;
}

int Connection::paramNextTimeval (struct timeval *tv)
{
	int sec;
	int usec;
	if (paramNextInteger (&sec) || paramNextInteger (&usec))
		return -1;
	tv->tv_sec = sec;
	tv->tv_usec = usec;
	return 0;
}

void Connection::newDataConn (int data_conn)
{
	if (otherDevice)
		otherDevice->newDataConn (data_conn);
}

void Connection::dataReceived ()
{
	std::map <int, DataChannels *>::iterator iter = readChannels.find (activeReadData);
	// inform device that we read some data
	if (otherDevice)
		otherDevice->dataReceived ((iter->second)->at(activeReadChannel));
	if ((iter->second)->getRestSize () == 0)
	{
		if (otherDevice)
			otherDevice->fullDataReceived (iter->first, iter->second);
		delete iter->second;
		readChannels.erase (iter);
		activeReadData = -1;
	}
	else if ((iter->second)->getChunkSize (activeReadChannel) == 0)
	{
		activeReadData = -1;
	}
}

Value * Connection::getValue (const char *value_name)
{
	return values.getValue (value_name);
}

Value * Connection::getValueType (const char *value_name, int value_type)
{
	Value * ret = getValue (value_name);
	if (ret == NULL)
		throw Error (std::string ("cannot find value with name ") + value_name);
	if (ret->getValueType () != value_type)
		throw Error (std::string ("value ") + value_name + std::string ("has wrong type"));
	return ret;
}

ValueVector::iterator & Connection::getFailedValue (ValueVector::iterator &iter)
{
	for (; iter != values.end (); iter++)
	{
		if (((*iter)->getFlags () & RTS2_VALUE_ERRORMASK) != RTS2_VALUE_GOOD)
			return iter;
	}
	return iter;
}

void Connection::addValue (Value * value, const ValueVector::iterator eiter)
{
	if (value->isValue (RTS2_VALUE_INFOTIME))
		info_time = (ValueTime *) value;
	values.insert (eiter, value);
}

int Connection::metaInfo (int rts2Type, std::string m_name, std::string desc)
{
	// if value exists, update it
	Value *existing_value = values.getValue (m_name.c_str ());
	ValueVector::iterator eiter;
	if (existing_value)
	{
		if (existing_value->getValueType () == (rts2Type & RTS2_VALUE_MASK))
		{
			existing_value->setFlags (rts2Type);
			existing_value->setDescription (desc);
			return -1;
		}
		eiter = values.removeValue (m_name.c_str ());
	}
	else
	{
		eiter = values.end ();
	}

	Value *new_value;
	switch (rts2Type & RTS2_EXT_TYPE)
	{
		case 0:
			new_value = newValue (rts2Type, m_name, desc);
			if (new_value == NULL)
				return -2;
			break;
		case RTS2_VALUE_STAT:
			new_value = new ValueDoubleStat (m_name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
			break;
		case RTS2_VALUE_MMAX:
			new_value = new ValueDoubleMinMax (m_name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
			break;
		case RTS2_VALUE_RECTANGLE:
			new_value = new ValueRectangle (m_name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
			break;
		case RTS2_VALUE_ARRAY:
			switch (rts2Type & RTS2_BASE_TYPE)
			{
				case RTS2_VALUE_STRING:
					new_value = new StringArray (m_name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
					break;
				case RTS2_VALUE_DOUBLE:
					new_value = new DoubleArray (m_name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
					break;
				case RTS2_VALUE_TIME:
					new_value = new TimeArray (m_name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
					break;
				case RTS2_VALUE_INTEGER:
					new_value = new IntegerArray (m_name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
					break;
				case RTS2_VALUE_BOOL:
					new_value = new BoolArray (m_name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
					break;
				default:
					logStream (MESSAGE_ERROR) << "unsuported array type; " << rts2Type << sendLog;
					exit (10);
			}
			break;
		case RTS2_VALUE_TIMESERIE:
			new_value = new ValueDoubleTimeserie (m_name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
			break;
		default:
			logStream (MESSAGE_ERROR) << "unknow value type: " << rts2Type << sendLog;
			return -2;
	}
	addValue (new_value, eiter);
	return -1;
}

int Connection::selMetaClear (const char *value_name)
{
	Value *val = getValue (value_name);
	if (!val || val->getValueType () != RTS2_VALUE_SELECTION)
		return -2;
	((ValueSelection *) val)->clear ();
	return -1;
}

int Connection::selMetaInfo (const char *value_name, char *sel_name)
{
	Value *val = getValue (value_name);
	if (!val || val->getValueType () != RTS2_VALUE_SELECTION)
		return -2;
	((ValueSelection *) val)->addSelVal (sel_name);
	return -1;
}

const char * Connection::getValueChar (const char *value_name)
{
	Value *val;
	val = getValue (value_name);
	if (val)
		return val->getValue ();
	return NULL;
}

double Connection::getValueDouble (const char *value_name)
{
	Value *val;
	val = getValue (value_name);
	if (val)
		return val->getValueDouble ();
	return NAN;
}

int Connection::getValueInteger (const char *value_name)
{
	Value *val;
	val = getValue (value_name);
	if (val)
		return val->getValueInteger ();
	return -1;
}

const char * Connection::getValueSelection (const char *value_name)
{
	Value *val;
	val = getValue (value_name);
	if (val->getValueType () != RTS2_VALUE_SELECTION)
		return "UNK";
	return ((ValueSelection *) val)->getSelName ();
}

const char * Connection::getValueSelection (const char *value_name, int val_num)
{
	Value *val;
	val = getValue (value_name);
	if (val == NULL || val->getValueType () != RTS2_VALUE_SELECTION)
		return "UNK";
	return ((ValueSelection *) val)->getSelName (val_num);
}

int Connection::commandValue (const char *v_name)
{
	Value *value = getValue (v_name);
	if (value)
	{
		int ret;
		ret = value->setValue (this);
		// notice other type..
		if (getOtherDevClient ())
			getOtherDevClient ()->valueChanged (value);
		return ret;
	}
	logStream (MESSAGE_ERROR)
		<< "unknow value from connection '" << getName () << "' "
		<< v_name
		<< " connection state " << getConnState ()
		<< " value size " << values.size ()
		<< sendLog;
	return -2;
}

bool Connection::existWriteType (int w_type)
{
	for (ValueVector::iterator iter = values.begin ();
		iter != values.end (); iter++)
	{
		if ((*iter)->getValueWriteFlags () == w_type)
			return true;
	}
	return false;
}

double Connection::getProgress (double now)
{
	if (isnan (statusStart) || isnan (statusExpectedEnd))
		return NAN;
	if (now > statusExpectedEnd)
		return 100;
	return 100 * ((now - statusStart) / (statusExpectedEnd - statusStart));
}

DataAbstractRead* Connection::lastDataChannel (int chan)
{
	if (readChannels.size () == 0 || ((int) (--readChannels.end ())->second->size ()) <= chan)
		return NULL;
	return (--readChannels.end ())->second->at (chan);
}

double Connection::getInfoTime ()
{
	if (info_time)
		return info_time->getValueDouble ();
	return getNow ();
}

void Connection::getInfoTime (struct timeval &tv)
{
	if (info_time)
	{
		info_time->getValueTime (tv);
		return;
	}
	gettimeofday (&tv, NULL);
}

bool Connection::infoTimeChanged ()
{
	if (info_time)
	{
		if (last_info_time != info_time->getValueDouble ())
		{
			last_info_time = info_time->getValueDouble ();
			return true;
		}
	}
	return false;
}

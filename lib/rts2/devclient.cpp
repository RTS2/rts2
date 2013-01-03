/* 
 * Client classes.
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

#include <algorithm>

#include "block.h"
#include "command.h"
#include "devclient.h"

using namespace rts2core;

DevClient::DevClient (Connection * in_connection):Object ()
{
	connection = in_connection;
	processedBaseInfo = NOT_PROCESED;

	waiting = NOT_WAITING;

	failedCount = 0;
}

DevClient::~DevClient ()
{
	unblockWait ();
}

void DevClient::postEvent (Event * event)
{
	switch (event->getType ())
	{
		case EVENT_QUERY_WAIT:
			if (waiting == WAIT_NOT_POSSIBLE)
				*((int *) event->getArg ()) = *((int *) event->getArg ()) + 1;
			break;
		case EVENT_ENTER_WAIT:
			waiting = WAIT_MOVE;
			break;
		case EVENT_CLEAR_WAIT:
			clearWait ();
			break;
	}
	Object::postEvent (event);
}

void DevClient::newDataConn (int data_conn)
{
}

void DevClient::dataReceived (DataAbstractRead *data)
{
}

void DevClient::fullDataReceived (int data_conn, DataChannels *data)
{
	logStream (MESSAGE_WARNING) << "Data not handled " << getName () << " " << data_conn << sendLog;
}

void DevClient::stateChanged (ServerState * state)
{
	if (connection->getErrorState () == DEVICE_ERROR_HW)
		incFailedCount ();
}

void DevClient::died ()
{
}

void DevClient::blockWait ()
{
	waiting = WAIT_NOT_POSSIBLE;
}

void DevClient::unblockWait ()
{
	if (waiting == WAIT_NOT_POSSIBLE)
	{
		int numNonWaits = 0;
		waiting = NOT_WAITING;
		connection->getMaster ()->
			postEvent (new Event (EVENT_QUERY_WAIT, (void *) &numNonWaits));
		if (numNonWaits == 0)	 // still zero, enter wait
			connection->getMaster ()->
				postEvent (new Event (EVENT_ENTER_WAIT));
	}
}

void DevClient::unsetWait ()
{
	unblockWait ();
	clearWait ();
}

int DevClient::isWaitMove ()
{
	return (waiting == WAIT_MOVE);
}

void DevClient::clearWait ()
{
	waiting = NOT_WAITING;
}

void DevClient::setWaitMove ()
{
	if (waiting == NOT_WAITING)
		waiting = WAIT_MOVE;
}

int DevClient::getStatus ()
{
	return connection->getState ();
}

void DevClient::idle ()
{
}

DevClientCamera::DevClientCamera (Connection * _connection):DevClient (_connection)
{
	lastExpectImage = false;
}

void DevClientCamera::exposureStarted (bool expectImage)
{
}

void DevClientCamera::exposureEnd (bool expectImage)
{
}

void DevClientCamera::exposureFailed (int status)
{
}

void DevClientCamera::readoutEnd ()
{
}

void DevClientCamera::stateChanged (ServerState * state)
{
	if (state->maskValueChanged (CAM_MASK_EXPOSE))
	{
		switch (state->getValue () & CAM_MASK_EXPOSE)
		{
			case CAM_EXPOSING:
			case CAM_EXPOSING_NOIM:
				if (!(state->getValue () & DEVICE_SC_CURR))
					break;
				lastExpectImage = state->getValue () & CAM_EXPOSING;
				if (connection->getErrorState () == DEVICE_NO_ERROR)
					exposureStarted (lastExpectImage);
				else
					exposureFailed (connection->getErrorState ());
				break;
			case CAM_NOEXPOSURE:
				if (connection->getErrorState () == DEVICE_NO_ERROR)
					exposureEnd (lastExpectImage);
				else
					exposureFailed (connection->getErrorState ());
				break;
		}
	}
	// exposure mask does not changed, but FT changed..
	else if (state->maskValueChanged (CAM_MASK_FT))
	{
		switch (state->getValue () & CAM_MASK_FT)
		{
			case CAM_FT:
				exposureEnd (lastExpectImage);
				break;
			case CAM_NOFT:
				if (!(state->getValue () & DEVICE_SC_CURR))
					break;
				exposureStarted (lastExpectImage);
				break;
		}
	}
	if (state->maskValueChanged (CAM_MASK_READING) && (state->getValue () & CAM_MASK_READING) == CAM_NOTREADING)
	{
		if (connection->getErrorState () == DEVICE_NO_ERROR)
			readoutEnd ();
		else
			exposureFailed (connection->getErrorState ());
	}
	DevClient::stateChanged (state);
}


bool DevClientCamera::isIdle ()
{
	return ((connection->getState () & (CAM_MASK_EXPOSE | CAM_MASK_READING)) ==
		(CAM_NOEXPOSURE | CAM_NOTREADING));
}


bool DevClientCamera::isExposing ()
{
	return (connection->getState () & CAM_MASK_EXPOSE) == CAM_EXPOSING;
}


DevClientTelescope::DevClientTelescope (Connection * _connection):DevClient (_connection)
{
	moveWasCorrecting = false;
}


DevClientTelescope::~DevClientTelescope (void)
{
	moveFailed (DEVICE_ERROR_HW);
}

void DevClientTelescope::stateChanged (ServerState * state)
{
	if (state->maskValueChanged (TEL_MASK_CUP_MOVING))
	{
		switch (state->getValue () & TEL_MASK_CUP_MOVING)
		{
			case TEL_MOVING:
			case TEL_MOVING | TEL_WAIT_CUP:
			case TEL_PARKING:
				moveStart (state->getValue () & TEL_CORRECTING);
				break;
			case TEL_OBSERVING:
			case TEL_PARKED:
				if (connection->getErrorState () == DEVICE_NO_ERROR)
					moveEnd ();
				else
					moveFailed (connection->getErrorState ());
				break;
		}
	}
	if (state->maskValueChanged (DEVICE_ERROR_KILL) && (connection->getErrorState () & DEVICE_ERROR_KILL))
	{
		moveFailed (connection->getErrorState ());
	}
	DevClient::stateChanged (state);
}

void DevClientTelescope::moveStart (bool correcting)
{
	moveWasCorrecting = correcting;
}

void DevClientTelescope::moveEnd ()
{
	moveWasCorrecting = false;
}

void DevClientTelescope::postEvent (Event * event)
{
	bool qe;
	switch (event->getType ())
	{
		case EVENT_QUICK_ENABLE:
			qe = *((bool *) event->getArg ());
			connection->queCommand (new CommandChangeValue (this, std::string ("quick_enabled"), '=', qe ? std::string ("2") : std::string ("1")));
			break;
	}
	DevClient::postEvent (event);
}

DevClientDome::DevClientDome (Connection * _connection):DevClient (_connection)
{
}

DevClientCupola::DevClientCupola (Connection * _connection):DevClientDome (_connection)
{
}

void DevClientCupola::syncStarted ()
{
}

void DevClientCupola::syncEnded ()
{
}

void DevClientCupola::syncFailed (int status)
{
}
void DevClientCupola::notMoveFailed (int status)
{
}

void DevClientCupola::stateChanged (ServerState * state)
{
	switch (state->getValue () & DOME_CUP_MASK_SYNC)
	{
		case DOME_CUP_NOT_SYNC:
			if (connection->getErrorState ())
				syncFailed (state->getValue ());
			else
				syncStarted ();
			break;
		case DOME_CUP_SYNC:
			if (connection->getErrorState ())
				syncFailed (state->getValue ());
			else
				syncEnded ();
			break;
	}
	DevClientDome::stateChanged (state);
}


DevClientMirror::DevClientMirror (Connection * _connection):DevClient (_connection)
{
}

DevClientMirror::~DevClientMirror (void)
{
	moveFailed (DEVICE_ERROR_HW);
}

void DevClientMirror::stateChanged (ServerState * state)
{
	if (connection->getErrorState ())
	{
		moveFailed (connection->getErrorState ());
	}
	else
	{
		switch (state->getValue () & MIRROR_MASK)
		{
			case MIRROR_A:
				mirrorA ();
				break;
			case MIRROR_B:
				mirrorB ();
				break;
			case MIRROR_UNKNOW:
				// strange, but that can happen
				moveFailed (DEVICE_ERROR_HW);
				break;
		}
	}
	DevClient::stateChanged (state);
}

void DevClientMirror::mirrorA ()
{
}

void DevClientMirror::mirrorB ()
{
}

void DevClientMirror::moveFailed (int status)
{
}

DevClientPhot::DevClientPhot (Connection * _connection):DevClient (_connection)
{
	lastCount = -1;
	lastExp = -1.0;
	integrating = false;
}

DevClientPhot::~DevClientPhot ()
{
	integrationFailed (DEVICE_ERROR_HW);
}

void DevClientPhot::stateChanged (ServerState * state)
{
	if (state->maskValueChanged (PHOT_MASK_INTEGRATE))
	{
		switch (state->getValue () & PHOT_MASK_INTEGRATE)
		{
			case PHOT_NOINTEGRATE:
				if (connection->getErrorState () == DEVICE_NO_ERROR)
					integrationEnd ();
				else
					integrationFailed (connection->getErrorState ());
				break;
			case PHOT_INTEGRATE:
				integrationStart ();
				break;
		}
	}
	if (state->maskValueChanged (PHOT_MASK_FILTER))
	{
		switch (state->getValue () & PHOT_MASK_FILTER)
		{
			case PHOT_FILTER_MOVE:
				filterMoveStart ();
				break;
			case PHOT_FILTER_IDLE:
				if (connection->getErrorState () == DEVICE_NO_ERROR)
					filterMoveEnd ();
				else
					filterMoveFailed (connection->getErrorState ());
				break;
		}
	}
	DevClient::stateChanged (state);
}

void DevClientPhot::filterMoveStart ()
{
}

void DevClientPhot::filterMoveEnd ()
{
}

void DevClientPhot::filterMoveFailed (int status)
{
}

void DevClientPhot::integrationStart ()
{
	integrating = true;
}

void DevClientPhot::integrationEnd ()
{
	integrating = false;
}

void DevClientPhot::integrationFailed (int status)
{
	integrating = false;
}

void DevClientPhot::addCount (int count, float exp, bool is_ov)
{
	lastCount = count;
	lastExp = exp;
}


bool DevClientPhot::isIntegrating ()
{
	return integrating;
}

void DevClientPhot::valueChanged (Value * value)
{
	if (value->isValue ("count"))
	{
		Value *v_count = getConnection ()->getValue ("count");
		Value *v_exp = getConnection ()->getValue ("exposure");
		Value *v_is_ov = getConnection ()->getValue ("is_ov");
		if (v_count && v_exp && v_is_ov
			&& v_is_ov->getValueType () == RTS2_VALUE_BOOL)
		{
			addCount (v_count->getValueInteger (), v_exp->getValueFloat (),
				((ValueBool *) v_is_ov)->getValueBool ());
		}
	}
	DevClient::valueChanged (value);
}

DevClientFilter::DevClientFilter (Connection * _connection):DevClient (_connection)
{
}

DevClientFilter::~DevClientFilter ()
{
}

void DevClientFilter::filterMoveStart ()
{
}

void DevClientFilter::filterMoveEnd ()
{
}

void DevClientFilter::filterMoveFailed (int status)
{
}

void DevClientFilter::stateChanged (ServerState * state)
{
	if (state->maskValueChanged (FILTERD_MASK))
	{
		switch (state->getValue () & FILTERD_MASK)
		{
			case FILTERD_MOVE:
				filterMoveStart ();
				break;
			case FILTERD_IDLE:
				if (connection->getErrorState () == DEVICE_NO_ERROR)
					filterMoveEnd ();
				else
					filterMoveFailed (connection->getErrorState ());
				break;
		}
	}
}

DevClientAugerShooter::DevClientAugerShooter (Connection * _connection):DevClient (_connection)
{
}

DevClientFocus::DevClientFocus (Connection * _connection):DevClient (_connection)
{
}

DevClientFocus::~DevClientFocus (void)
{
	focusingFailed (DEVICE_ERROR_HW);
}

void DevClientFocus::focusingStart ()
{
}

void DevClientFocus::focusingEnd ()
{
}

void DevClientFocus::focusingFailed (int status)
{
	focusingEnd ();
}

void DevClientFocus::stateChanged (ServerState * state)
{
	switch (state->getValue () & FOC_MASK_FOCUSING)
	{
		case FOC_FOCUSING:
			focusingStart ();
			break;
		case FOC_SLEEPING:
			if (connection->getErrorState () == DEVICE_NO_ERROR)
				focusingEnd ();
			else
				focusingFailed (connection->getErrorState ());
			break;
	}
	DevClient::stateChanged (state);
}

DevClientExecutor::DevClientExecutor (Connection * _connection):DevClient (_connection)
{
}

void DevClientExecutor::lastReadout ()
{
}

void DevClientExecutor::stateChanged (ServerState * state)
{
	switch (state->getValue () & EXEC_STATE_MASK)
	{
		case EXEC_LASTREAD:
			lastReadout ();
			break;
	}
	DevClient::stateChanged (state);
}

DevClientSelector::DevClientSelector (Connection * _connection):DevClient (_connection)
{
}

DevClientImgproc::DevClientImgproc (Connection * _connection):DevClient (_connection)
{
}

DevClientGrb::DevClientGrb (Connection * _connection):DevClient (_connection)
{
}

DevClientBB::DevClientBB (Connection * _connection):DevClient (_connection)
{
}

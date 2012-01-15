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

#ifndef __RTS2_DEVCLIENT__
#define __RTS2_DEVCLIENT__

#include "object.h"
#include "block.h"
#include "value.h"

namespace rts2core
{
class Command;

/**
 * Defines default classes for handling device information.
 *
 * Descendants of rts2client can (by overloading method getConnection)
 * specify devices classes, which e.g. allow X11 printout of device
 * information etc..
 */
class DevClient:public Object
{
	public:
		DevClient (Connection * in_connection);
		virtual ~ DevClient (void);

		virtual void postEvent (Event * event);

		/**
		 * Called when new data connection is created.
		 *
		 * @param data_conn Index of new data connection.
		 */
		virtual void newDataConn (int data_conn);

		/**
		 * Called when some data were received.
		 *
		 * @param data  pointer to Rts2DataRead object which received data.
		 */
		virtual void dataReceived (DataAbstractRead *data);

		/**
		 * Called when full image is received.
		 *
		 * @param data_conn data connection ID. Can be < 0 for shared memory data.
		 * @param data      pointer to Rts2DataRead object holding full data.
		 */
		virtual void fullDataReceived (int data_conn, DataChannels *data);

		virtual void stateChanged (ServerState * state);

		Connection *getConnection () { return connection; }

		const char *getName () { return connection->getName (); }

		int getOtherType () { return connection->getOtherType (); }

		rts2core::Block *getMaster () { return connection->getMaster (); }

		void queCommand (Command * cmd) { connection->queCommand (cmd); }

		void queCommand (Command * cmd, int notBop) { connection->queCommand (cmd, notBop); }

		void commandReturn (Command * cmd, int status)
		{
			if (status)
				incFailedCount ();
			else
				clearFailedCount ();
		}

		int getStatus ();

		/**
		 * Callback for info command returned with OK status.
		 */

		virtual void infoOK () {}

		/**
		 * Callback for info command returned with failed status.
		 */
		virtual void infoFailed () {}

		virtual void idle ();

		virtual void valueChanged (rts2core::Value * value) {}

	protected:
		Connection * connection;
		enum { NOT_PROCESED, PROCESED } processedBaseInfo;
		virtual void died ();

		// if system is waiting for something..
		enum { NOT_WAITING, WAIT_MOVE, WAIT_NOT_POSSIBLE } waiting;
		virtual void blockWait ();
		virtual void unblockWait ();
		virtual void unsetWait ();
		virtual void clearWait ();
		virtual int isWaitMove ();
		virtual void setWaitMove ();

		void incFailedCount () { failedCount++; }
		int getFailedCount () { return failedCount; }
		void clearFailedCount () { failedCount = 0; }

	private:
		int failedCount;
};

/**
 * Classes for devices connections.
 *
 * Please note that we cannot use descendants of ConnClient,
 * since we can connect to device as server.
 */
class DevClientCamera:public DevClient
{
	public:
		DevClientCamera (Connection * in_connection);

		/**
		 * Called when exposure (dark of light) command finished without error.
		 */
		virtual void exposureCommandOK () {};

		/**
		 * ExposureFailed will get called even when we faild during readout
		 */
		virtual void exposureFailed (int status);
		virtual void stateChanged (ServerState * state);

		virtual void filterOK () {}
		virtual void filterFailed (int status) {}
		virtual void focuserOK () {}
		virtual void focuserFailed (int status) {}

		bool isIdle ();
		bool isExposing ();
	protected:
		virtual void exposureStarted ();
		virtual void exposureEnd ();
		virtual void readoutEnd ();
};

class DevClientTelescope:public DevClient
{
	protected:
		bool moveWasCorrecting;
		virtual void moveStart (bool correcting);
		virtual void moveEnd ();
	public:
		DevClientTelescope (Connection * in_connection);
		virtual ~ DevClientTelescope (void);
		/*! gets calledn when move finished without success */
		virtual void moveFailed (int status)
		{
			moveWasCorrecting = false;
		}
		virtual void stateChanged (ServerState * state);
		virtual void postEvent (Event * event);
};

class DevClientDome:public DevClient
{
	public:
		DevClientDome (Connection * in_connection);
};

class DevClientCupola:public DevClientDome
{
	protected:
		virtual void syncStarted ();
		virtual void syncEnded ();
	public:
		DevClientCupola (Connection * in_connection);
		virtual void syncFailed (int status);
		virtual void notMoveFailed (int status);
		virtual void stateChanged (ServerState * state);
};

class DevClientMirror:public DevClient
{
	protected:
		virtual void mirrorA ();
		virtual void mirrorB ();
	public:
		DevClientMirror (Connection * in_connection);
		virtual ~ DevClientMirror (void);
		virtual void moveFailed (int status);
		virtual void stateChanged (ServerState * state);
};

class DevClientFocus:public DevClient
{
	protected:
		virtual void focusingStart ();
		virtual void focusingEnd ();
	public:
		DevClientFocus (Connection * in_connection);
		virtual ~ DevClientFocus (void);
		virtual void focusingFailed (int status);
		virtual void stateChanged (ServerState * state);
};

class DevClientPhot:public DevClient
{
	protected:
		virtual void filterMoveStart ();
		virtual void filterMoveEnd ();
		virtual void integrationStart ();
		virtual void integrationEnd ();
		virtual void addCount (int count, float exp, bool is_ov);
		int lastCount;
		float lastExp;
		bool integrating;
	public:
		DevClientPhot (Connection * in_connection);
		virtual ~ DevClientPhot (void);
		virtual void integrationFailed (int status);
		virtual void filterMoveFailed (int status);
		virtual void stateChanged (ServerState * state);

		virtual void filterOK () {}

		bool isIntegrating ();

		virtual void valueChanged (rts2core::Value * value);
};

class DevClientFilter:public DevClient
{
	protected:
		virtual void filterMoveStart ();
		virtual void filterMoveEnd ();
	public:
		DevClientFilter (Connection * in_connection);
		virtual ~ DevClientFilter (void);
		virtual void filterMoveFailed (int status);
		virtual void stateChanged (ServerState * state);

		virtual void filterOK () {}
};

class DevClientAugerShooter:public DevClient
{
	public:
		DevClientAugerShooter (Connection * in_connection);
};

class DevClientExecutor:public DevClient
{
	protected:
		virtual void lastReadout ();
	public:
		DevClientExecutor (Connection * in_connection);
		virtual void stateChanged (ServerState * state);
};

class DevClientSelector:public DevClient
{
	public:
		DevClientSelector (Connection * in_connection);
};

class DevClientImgproc:public DevClient
{
	public:
		DevClientImgproc (Connection * in_connection);
};

class DevClientGrb:public DevClient
{
	public:
		DevClientGrb (Connection * in_connection);
};

}
#endif							 /* !__RTS2_DEVCLIENT__ */

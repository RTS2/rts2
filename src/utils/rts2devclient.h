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

#include "rts2object.h"
#include "rts2block.h"
#include "rts2value.h"

class Rts2ServerState;

class Rts2Block;

namespace rts2core
{
class Rts2Command;

/**
 * Defines default classes for handling device information.
 *
 * Descendants of rts2client can (by overloading method getConnection)
 * specify devices classes, which e.g. allow X11 printout of device
 * information etc..
 */
class Rts2DevClient:public Rts2Object
{
	private:
		int failedCount;
	protected:
		Rts2Conn * connection;
		enum { NOT_PROCESED, PROCESED } processedBaseInfo;
		virtual void died ();
		enum
		{						 // if we wait for something..
			NOT_WAITING, WAIT_MOVE, WAIT_NOT_POSSIBLE
		} waiting;
		virtual void blockWait ();
		virtual void unblockWait ();
		virtual void unsetWait ();
		virtual void clearWait ();
		virtual int isWaitMove ();
		virtual void setWaitMove ();

		void incFailedCount ()
		{
			failedCount++;
		}
		int getFailedCount ()
		{
			return failedCount;
		}
		void clearFailedCount ()
		{
			failedCount = 0;
		}

	public:
		Rts2DevClient (Rts2Conn * in_connection);
		virtual ~ Rts2DevClient (void);

		virtual void postEvent (Rts2Event * event);

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
		virtual void dataReceived (DataRead *data);

		/**
		 * Called when full image is received.
		 *
		 * @param data_conn data connection ID. Can be < 0 for shared memory data.
		 * @param data      pointer to Rts2DataRead object holding full data.
		 */
		virtual void fullDataReceived (int data_conn, DataChannels *data);

		virtual void stateChanged (Rts2ServerState * state);

		Rts2Conn *getConnection ()
		{
			return connection;
		}

		const char *getName ()
		{
			return connection->getName ();
		}

		int getOtherType ()
		{
			return connection->getOtherType ();
		}

		Rts2Block *getMaster ()
		{
			return connection->getMaster ();
		}

		void queCommand (Rts2Command * cmd)
		{
			connection->queCommand (cmd);
		}

		void queCommand (Rts2Command * cmd, int notBop)
		{
			connection->queCommand (cmd, notBop);
		}

		void commandReturn (Rts2Command * cmd, int status)
		{
			if (status)
				incFailedCount ();
			else
				clearFailedCount ();
		}

		int getStatus ();

		virtual void infoOK ()
		{
		}
		virtual void infoFailed ()
		{
		}

		virtual void idle ();

		virtual void valueChanged (Rts2Value * value)
		{

		}
};

/**
 * Classes for devices connections.
 *
 * Please note that we cannot use descendants of Rts2ConnClient,
 * since we can connect to device as server.
 */
class Rts2DevClientCamera:public Rts2DevClient
{
	protected:
		virtual void exposureStarted ();
		virtual void exposureEnd ();
		virtual void readoutEnd ();
	public:
		Rts2DevClientCamera (Rts2Conn * in_connection);

		/**
		 * ExposureFailed will get called even when we faild during readout
		 */
		virtual void exposureFailed (int status);
		virtual void stateChanged (Rts2ServerState * state);

		virtual void filterOK ()
		{
		}
		virtual void filterFailed (int status)
		{
		}
		virtual void focuserOK ()
		{
		}
		virtual void focuserFailed (int status)
		{
		}

		bool isIdle ();
		bool isExposing ();
};

class Rts2DevClientTelescope:public Rts2DevClient
{
	protected:
		bool moveWasCorrecting;
		virtual void moveStart (bool correcting);
		virtual void moveEnd ();
	public:
		Rts2DevClientTelescope (Rts2Conn * in_connection);
		virtual ~ Rts2DevClientTelescope (void);
		/*! gets calledn when move finished without success */
		virtual void moveFailed (int status)
		{
			moveWasCorrecting = false;
		}
		virtual void stateChanged (Rts2ServerState * state);
		virtual void postEvent (Rts2Event * event);
};

class Rts2DevClientDome:public Rts2DevClient
{
	public:
		Rts2DevClientDome (Rts2Conn * in_connection);
};

class Rts2DevClientCupola:public Rts2DevClientDome
{
	protected:
		virtual void syncStarted ();
		virtual void syncEnded ();
	public:
		Rts2DevClientCupola (Rts2Conn * in_connection);
		virtual void syncFailed (int status);
		virtual void notMoveFailed (int status);
		virtual void stateChanged (Rts2ServerState * state);
};

class Rts2DevClientMirror:public Rts2DevClient
{
	protected:
		virtual void mirrorA ();
		virtual void mirrorB ();
	public:
		Rts2DevClientMirror (Rts2Conn * in_connection);
		virtual ~ Rts2DevClientMirror (void);
		virtual void moveFailed (int status);
		virtual void stateChanged (Rts2ServerState * state);
};

class Rts2DevClientFocus:public Rts2DevClient
{
	protected:
		virtual void focusingStart ();
		virtual void focusingEnd ();
	public:
		Rts2DevClientFocus (Rts2Conn * in_connection);
		virtual ~ Rts2DevClientFocus (void);
		virtual void focusingFailed (int status);
		virtual void stateChanged (Rts2ServerState * state);
};

class Rts2DevClientPhot:public Rts2DevClient
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
		Rts2DevClientPhot (Rts2Conn * in_connection);
		virtual ~ Rts2DevClientPhot (void);
		virtual void integrationFailed (int status);
		virtual void filterMoveFailed (int status);
		virtual void stateChanged (Rts2ServerState * state);

		virtual void filterOK ()
		{
		}

		bool isIntegrating ();

		virtual void valueChanged (Rts2Value * value);
};

class Rts2DevClientFilter:public Rts2DevClient
{
	protected:
		virtual void filterMoveStart ();
		virtual void filterMoveEnd ();
	public:
		Rts2DevClientFilter (Rts2Conn * in_connection);
		virtual ~ Rts2DevClientFilter (void);
		virtual void filterMoveFailed (int status);
		virtual void stateChanged (Rts2ServerState * state);

		virtual void filterOK ()
		{
		}
};

class Rts2DevClientAugerShooter:public Rts2DevClient
{
	public:
		Rts2DevClientAugerShooter (Rts2Conn * in_connection);
};

class Rts2DevClientExecutor:public Rts2DevClient
{
	protected:
		virtual void lastReadout ();
	public:
		Rts2DevClientExecutor (Rts2Conn * in_connection);
		virtual void stateChanged (Rts2ServerState * state);
};

class Rts2DevClientSelector:public Rts2DevClient
{
	public:
		Rts2DevClientSelector (Rts2Conn * in_connection);
};

class Rts2DevClientImgproc:public Rts2DevClient
{
	public:
		Rts2DevClientImgproc (Rts2Conn * in_connection);
};

class Rts2DevClientGrb:public Rts2DevClient
{
	public:
		Rts2DevClientGrb (Rts2Conn * in_connection);
};

}
#endif							 /* !__RTS2_DEVCLIENT__ */

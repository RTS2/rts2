/* 
 * Driver for Aerotech asix controller.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#include "sensord.h"

#include <Windows.h>
#include <AerSys.h>

#define AX_SCALE1 50000
#define AX_SCALE2 10000
#define AX_SCALE3 -10000

// when we'll have more then 3 axes, please change this
#define MAX_REQUESTED 3

namespace rts2sensord
{

class A3200:public Sensor
{
	private:
		HAERCTRL hAerCtrl;
		AXISINDEX mAxis;

		Rts2ValueDoubleMinMax *ax1;
		Rts2ValueDoubleMinMax *ax2;
		Rts2ValueDoubleMinMax *ax3;

		Rts2ValueInteger *moveCount;

		LPCTSTR initFile;

		void logErr (char *proc, AERERR_CODE eRc);

		int moveEnable ();
		int home ();
		int moveAxis (AXISINDEX ax, LONG tar);

	protected:
		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
		virtual int processOption (int in_opt);

	public:
		A3200 (int in_argc, char **in_argv);
		virtual ~ A3200 (void);

		virtual int init ();
		virtual int info ();

		int commandAuthorized (Rts2Conn * conn);
};

};

using namespace rts2sensord;

void
A3200::logErr (char *proc, AERERR_CODE eRc)
{
	TCHAR szMsg[MAX_TEXT_LEN];

	AerErrGetMessage (eRc, szMsg, MAX_TEXT_LEN, false);
	logStream (MESSAGE_ERROR) << "Cannot initialize A3200 " << szMsg << sendLog;
}


int
A3200::moveEnable ()
{
	AERERR_CODE eRc;
	eRc = AerMoveMEnable (hAerCtrl, mAxis);
	if (eRc != AERERR_NOERR)
	{
		logErr ("home MoveMEnable", eRc);
		return -1;
	}
	eRc = AerMoveMWaitDone (hAerCtrl, mAxis, 100, 0);
	if (eRc != AERERR_NOERR)
	{
		logErr ("home MoveMWait for Enable", eRc);
		return -1;
	}
	return 0;
}


int
A3200::home ()
{
	AERERR_CODE eRc;
	int ret = moveEnable ();
	if (ret)
		return ret;
	logStream (MESSAGE_DEBUG) << "All axis enabled, homing" << sendLog;
	// first home Z axis..
	eRc = AerMoveHome (hAerCtrl, AXISINDEX_3);
	if (eRc != AERERR_NOERR)
	{
		logErr ("home Z axis", eRc);
		return -1;
	}
	eRc = AerMoveWaitDone (hAerCtrl, AXISINDEX_3, 10000, 0);
	if (eRc != AERERR_NOERR)
	{
		logErr ("home wait for Z axis", eRc);
		return -1;
	}
	// now home all axis
	eRc = AerMoveMHome (hAerCtrl, mAxis);
	if (eRc != AERERR_NOERR)
	{
		logErr ("home MHome", eRc);
		return -1;
	}
	eRc = AerMoveMWaitDone (hAerCtrl, mAxis, 10000, 0);
	if (eRc != AERERR_NOERR)
	{
		logErr ("home MoveMWait for Home", eRc);
		return -1;
	}
	logStream (MESSAGE_DEBUG) << "All axis homed properly" << sendLog;
	return 0;
}


int
A3200::moveAxis (AXISINDEX ax, LONG tar)
{
	AERERR_CODE eRc;
	blockExposure ();
	if (moveCount->getValueInteger () == 0)
	{
		int ret;
		ret = home ();
		if (ret)
			return ret;
	}
	moveCount->inc ();
	eRc = AerMoveAbsolute (hAerCtrl, ax, tar, 1000000);
	if (eRc != AERERR_NOERR)
	{
		logErr ("moveAxis MoveAbsolute", eRc);
		clearExposure ();
		return -1;
	}
	eRc = AerMoveWaitDone (hAerCtrl, ax, 10000, 0);
	if (eRc != AERERR_NOERR)
	{
		logErr ("MoveMWait for Home", eRc);
		clearExposure ();
		return -1;
	}
	clearExposure ();
	return 0;
}


int
A3200::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == ax1)
	{
		return moveAxis (AXISINDEX_1, new_value->getValueDouble () * AX_SCALE1);
	}
	if (old_value == ax2)
	{
		return moveAxis (AXISINDEX_2, new_value->getValueDouble () * AX_SCALE2);
	}
	if (old_value == ax3)
	{
		return moveAxis (AXISINDEX_3, new_value->getValueDouble () * AX_SCALE3);
	}
	return Sensor::setValue (old_value, new_value);
}


A3200::A3200 (int in_argc, char **in_argv)
:Sensor (in_argc, in_argv)
{
	hAerCtrl = NULL;

	mAxis = AXISMASK_1 | AXISMASK_2 | AXISMASK_3;

	createValue (ax1, "AX1", "first axis", true, RTS2_VALUE_WRITABLE);
	createValue (ax2, "AX2", "second axis", true, RTS2_VALUE_WRITABLE);
	createValue (ax3, "AX3", "third axis", true, RTS2_VALUE_WRITABLE);

	createValue (moveCount, "moveCount", "number of axis movements", false);
	moveCount->setValueInteger (0);

	addOption ('f', NULL, 1, "Init file");
}


A3200::~A3200 (void)
{
	AerSysStop (hAerCtrl);
}


int
A3200::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			initFile = optarg;
			return 0;
	}
	return Sensor::processOption (in_opt);
}


int
A3200::init ()
{
	AERERR_CODE eRc = AERERR_NOERR;
	int ret;
	ret = Sensor::init ();
	if (ret)
		return ret;

	eRc =
		AerSysInitialize (0, initFile, 1, &hAerCtrl, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL);

	if (eRc != AERERR_NOERR)
	{
		logErr ("init AerSysInitialize", eRc);
		return -1;
	}
	ret = moveEnable ();
	return ret;
}


int
A3200::info ()
{
	DWORD dwUnits;
	DWORD dwDriveStatus[MAX_REQUESTED];
	DWORD dwAxisStatus[MAX_REQUESTED];
	DWORD dwFault[MAX_REQUESTED];
	DOUBLE dPosition[MAX_REQUESTED];
	DOUBLE dPositionCmd[MAX_REQUESTED];
	DOUBLE dVelocityAvg[MAX_REQUESTED];

	AERERR_CODE eRc;

	dwUnits = 0;				 // we want counts as units for the pos/vels

	eRc = AerStatusGetAxisInfoEx (hAerCtrl, mAxis, dwUnits,
		dwDriveStatus,
		dwAxisStatus,
		dwFault,
		dPosition, dPositionCmd, dVelocityAvg);
	if (eRc != AERERR_NOERR)
	{
		logErr ("info AerStatusGetAxisInfoEx", eRc);
		return -1;
	}
	ax1->setValueDouble (dPosition[0] / AX_SCALE1);
	ax2->setValueDouble (dPosition[1] / AX_SCALE2);
	ax3->setValueDouble (dPosition[2] / AX_SCALE3);
	return Sensor::info ();
}


int
A3200::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("home"))
	{
		if (!conn->paramEnd ())
			return -2;
		return home ();
	}
	return Sensor::commandAuthorized (conn);
}


int
main (int argc, char **argv)
{
	A3200 device = A3200 (argc, argv);
	return device.run ();
}
